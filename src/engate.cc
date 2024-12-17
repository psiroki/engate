#include "engate.h"

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "libyuv.h"
#include "opus.h"
#include "vpx/vp8cx.h"
#include "vpx/vpx_encoder.h"
#include "mkvmuxer/mkvmuxer.h"
#include "mkvmuxer/mkvwriter.h"

class EngateImpl {
  vpx_codec_ctx_t vpx_codec;
  OpusEncoder *opus_encoder;
  mkvmuxer::MkvWriter *writer;
  mkvmuxer::Segment *muxer;
  int pts;
  uint64_t audio_pts;
  int video_track;
  int audio_track;
  int width, height;
  int sample_rate;
  vpx_image_t *vpx_frame;
  vpx_codec_enc_cfg_t cfg;
  unsigned deadline;


  int add_native_video_frame(vpx_image_t *frame) {
    // Pass the frame to libvpx encoder
    int res;
    if (res = vpx_codec_encode(&vpx_codec, frame, pts, 1, 0, deadline)) {
      fprintf(stderr, "Error encoding frame: %d\n", res);
      return -1;
    }

    if (frame) ++pts;

    const vpx_codec_cx_pkt_t *pkt;
    vpx_codec_iter_t iter = nullptr;
    while ((pkt = vpx_codec_get_cx_data(&vpx_codec, &iter)) != nullptr) {
      if (pkt->kind == VPX_CODEC_CX_FRAME_PKT) {
        bool keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;
        fprintf(stderr, "Frame pts: %d/%d%s\n", (int) pkt->data.frame.pts, pts, keyframe ? " (K)" : "");
        int64_t pts_ns = pkt->data.frame.pts * 1000000000LL * cfg.g_timebase.num / cfg.g_timebase.den;
        if (!muxer->AddFrame(reinterpret_cast<const uint8_t*>(pkt->data.frame.buf),
            pkt->data.frame.sz, video_track, pts_ns, keyframe)) {
          return -1;
        }
      } else {
        fprintf(stderr, "Unknown pkt: %d\n", pkt->kind);
      }
    }

    return 0;
  }
public:
  inline EngateImpl(): deadline(VPX_DL_GOOD_QUALITY) { }

  inline void set_deadline(unsigned newDeadline) {
    deadline = newDeadline;
  }

  int init(const char *output_file, int w, int h, int fps, int audio_rate, int channels, int bitrate) {
    width = w;
    height = h;
    pts = 0;
    audio_pts = 0;

    // Video Encoder (libvpx)
    if (vpx_codec_enc_config_default(vpx_codec_vp9_cx(), &cfg, 0)) return -1;
    cfg.g_w = width;
    cfg.g_h = height;
    cfg.g_timebase.num = 1;
    cfg.g_timebase.den = fps;
    cfg.rc_target_bitrate = bitrate;
    if (vpx_codec_enc_init(&vpx_codec, vpx_codec_vp9_cx(), &cfg, 0)) return -2;
    vpx_frame = vpx_img_alloc(nullptr, VPX_IMG_FMT_I420, width, height, 16);

    // Audio Encoder (libopus)
    int opus_error;
    sample_rate = audio_rate;
    opus_encoder = opus_encoder_create(sample_rate, channels, OPUS_APPLICATION_AUDIO, &opus_error);
    if (opus_error != OPUS_OK) {
      fprintf(stderr, "Opus error: %d\n", opus_error);
      return -3;
    }

    // WebM Muxer (libwebm)
    writer = new mkvmuxer::MkvWriter();
    if (!writer->Open(output_file)) return -4;

    muxer = new mkvmuxer::Segment();
    if (!muxer->Init(writer)) return -5;

    // Add video and audio tracks
    video_track = muxer->AddVideoTrack(width, height, 0);
    audio_track = muxer->AddAudioTrack(sample_rate, channels, 0);
    if (!video_track || !audio_track) return -6;

    mkvmuxer::VideoTrack *vt = (mkvmuxer::VideoTrack*) muxer->GetTrackByNumber(video_track);
    vt->set_codec_id(mkvmuxer::Tracks::kVp9CodecId);

    mkvmuxer::AudioTrack *at = (mkvmuxer::AudioTrack*) muxer->GetTrackByNumber(audio_track);
    at->set_codec_id(mkvmuxer::Tracks::kOpusCodecId);

    return 0;
  }

  int add_video_frame(const uint8_t *argb_buffer, int pitch) {
    if (!pitch) pitch = width * 4;
    // Convert RGB to YUV420 (use libyuv for optimized conversion on ARM)
    int res = libyuv::ARGBToI420(argb_buffer, pitch,
        vpx_frame->planes[0], vpx_frame->stride[0],
        vpx_frame->planes[1], vpx_frame->stride[1],
        vpx_frame->planes[2], vpx_frame->stride[2],
        width, height);

    if (res != 0) {
      fprintf(stderr, "ARGBToI420 conversion failed: %d\n", res);
      return res;
    }

    return add_native_video_frame(vpx_frame);
  }

  int get_audio_frame_size() {
    return 20*sample_rate / 1000;
  }
  
  int add_audio_samples(const int16_t *pcm_samples, int sample_count) {
    if (sample_count != get_audio_frame_size())
      return -1;
    uint8_t encoded_audio[4096];
    int encoded_bytes = opus_encode(opus_encoder, pcm_samples, sample_count, encoded_audio, sizeof(encoded_audio));
    if (encoded_bytes < 0) return -2;

    if (!muxer->AddFrame(encoded_audio, encoded_bytes, audio_track, audio_pts, false)) return -3;

    audio_pts += (sample_count * 1000000000LL) / sample_rate; // Timestamp in nanoseconds
    return 0;
  }

  int finalize() {
    add_native_video_frame(nullptr);
    if (!muxer->Finalize()) return -1;
    writer->Close();
    delete muxer;
    delete writer;
    vpx_img_free(vpx_frame);
    opus_encoder_destroy(opus_encoder);
    vpx_codec_destroy(&vpx_codec);
    return 0;
  }
};

extern "C" {

void engateInitSettings(EngateSettings *settings) {
  memset(settings, 0, sizeof(settings));
  settings->audioSampleRate = 48000;
  settings->channels = 2;
  settings->deadline = VPX_DL_GOOD_QUALITY;
}

int engateCreate(const EngateSettings *settings, Engate *result) {
  EngateImpl *engate = new EngateImpl();
  *result = engate;
  engate->set_deadline(settings->deadline);
  return engate->init(settings->filename, settings->width, settings->height, settings->fps,
      settings->audioSampleRate, settings->channels, settings->videoBitrate);
}

int engateAddVideoFrame(Engate engate, const void *pixels, int pitch) {
  EngateImpl *impl = reinterpret_cast<EngateImpl*>(engate);
  return impl->add_video_frame(reinterpret_cast<const uint8_t*>(pixels), pitch);
}

int engateGetAudioFrameSize(Engate engate) {
  EngateImpl *impl = reinterpret_cast<EngateImpl*>(engate);
  return impl->get_audio_frame_size();
}

int engateAddAudioFrame(Engate engate, const void *samples) {
  EngateImpl *impl = reinterpret_cast<EngateImpl*>(engate);
  return impl->add_audio_samples(reinterpret_cast<const int16_t*>(samples), impl->get_audio_frame_size());
}

int engateClose(Engate *engate) {
  EngateImpl *impl = reinterpret_cast<EngateImpl*>(*engate);
  int res = impl->finalize();
  if (res == 0) {
    delete impl;
    *engate = nullptr;
  }
  return res;
}

}
