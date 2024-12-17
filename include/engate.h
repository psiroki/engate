#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void *Engate;

typedef struct EngateSettings {
  /// Where to save the video
  const char *filename;
  /// Video frame width in pixels
  int width;
  /// Video frame height in pixels
  int height;
  /// Video frames per second
  int fps;
  /// Video bitrate in kilobits per second
  int videoBitrate;
  /// Audio sample rate (defaults to 48000)
  int audioSampleRate;
  /// Number of audio channels (defaults to 2)
  int channels;
  /// Frame encoding deadline in microseconds, affects encoding speed and quality
  unsigned deadline;
} EngateSettings;

void engateInitSettings(EngateSettings *settings);
int engateCreate(const EngateSettings *settings, Engate *result);
int engateAddVideoFrame(Engate engate, const void *pixels, int pitch);
int engateGetAudioFrameSize(Engate engate);
int engateAddAudioFrame(Engate engate, const void *samples);
int engateClose(Engate *engate);

#ifdef __cplusplus
}
#endif

