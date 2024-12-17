#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "engate.h"

#include "stb_image.h"

// Generate a slice of sound.
// `buffer`: Pointer to the int16_t interleaved stereo buffer.
// `sample_count`: Number of stereo samples to generate.
// `sample_offset`: Offset of the first sample (useful for continuity).
void generate_audio(int16_t* buffer, size_t sample_count, size_t sample_offset) {
  const int sample_rate = 44100; // 44.1 kHz
  const double base_freq = 440.0; // Base frequency: A4 (musical note)
  const double lfo_freq = 3.0;    // Low-frequency oscillator for modulation (0.5 Hz)
  const double panning_freq = 2.0; // Panning oscillation (0.25 Hz)
  double minorThird = pow(2.0, 3.0/12.0);
  double perfectFifth = pow(2.0, 7.0/12.0);
  for (size_t i = 0; i < sample_count; ++i) {
    size_t current_sample = sample_offset + i;

    // Time in seconds for the current sample.
    double t = static_cast<double>(current_sample) / sample_rate;

    // Generate the main sine wave tone.
    double tone = 0.0;
    for (int i = 1; i < 4; i <<= 1) {
      tone += (sin(2.0 * M_PI * base_freq * t * i) +
          sin(2.0 * M_PI * base_freq * minorThird * t * i) +
          sin(2.0 * M_PI * base_freq * perfectFifth * t * i)) / i;
    }

    // Apply LFO modulation to the tone (vibrato effect).
    double lfo = 0.2 * sin(2.0 * M_PI * lfo_freq * t);
    tone = sin(2.0 * M_PI * (base_freq + lfo) * t);

    // Generate a stereo panning effect.
    double pan = 0.5 * (1.0 + sin(2.0 * M_PI * panning_freq * t));

    // Scale the tone to int16_t range.
    int16_t left_sample = static_cast<int16_t>(tone * (1.0 - pan) * 32767);
    int16_t right_sample = static_cast<int16_t>(tone * pan * 32767);

    // Interleave stereo samples.
    buffer[2 * i] = left_sample;     // Left channel
    buffer[2 * i + 1] = right_sample; // Right channel
  }
}

int main(int argc, char **argv) {
  int width, height, channels;
  uint32_t *data = reinterpret_cast<uint32_t*>(stbi_load("sportscar.png", &width, &height, &channels, STBI_rgb_alpha));
  const int expSize = 1024;
  if (width != expSize || height != expSize) {
    fprintf(stderr, "Expected a %dx%d texture, but got %dx%d", expSize, expSize, width, height);
  }
  int fps = 30;
  const int sampleRate = 48000;
  EngateSettings settings;
  engateInitSettings(&settings);
  settings.width = 640;
  settings.height = 480;
  settings.fps = fps;
  settings.audioSampleRate = sampleRate;
  settings.videoBitrate = 2000;
  settings.filename = "demo.webm";
  settings.deadline = 33000;
  Engate engate(nullptr);
  int res = engateCreate(&settings, &engate);
  if (res < 0) return fprintf(stderr, "Could not init: %d\n", res);
  uint32_t pixels[640*480];
  float spatialScale = M_PI / 256.0f;
  float timeScale = M_PI / 30.0f;
  int16_t slice[2*engateGetAudioFrameSize(engate)];
  uint32_t sliceSamples = sizeof(slice) / sizeof(*slice) / 2;
  uint32_t samplesPerFrame = sampleRate/fps;
  uint32_t sampleOffset = 0;
  uint32_t sampleTime = 0;
  for (int i = 0; i < 120; ++i) {
    printf("%d..\n", i);
    int dx = width - ((width - 640) >> 2) - 640;
    int dy = height - ((height - 480) >> 2) - 480;
    float t = i * timeScale;
    for (int y = 0; y < 480; ++y) {
      uint32_t *target = pixels + y * 640;
      for (int x = 0; x < 640; ++x) {
        int u = x + dx + ((sinf(t + x * spatialScale)+sinf(t + y * spatialScale)) * 32.0f);
        int v = y + dy + ((cosf(t + x * spatialScale)+cosf(t + y * spatialScale)) * 32.0f);
        uint32_t c = data[(v & (expSize - 1)) * expSize + (u & (expSize - 1))];
        target[x] = (c >> 16 & 0xFF) | (c & 0xFF00) | (c << 16 & 0xFF0000) | 0xFF000000;
      }
    }
    res = engateAddVideoFrame(engate, pixels, 640*4);
    if (res < 0) return fprintf(stderr, "Could not add video frame: %d\n", res);
    sampleTime += samplesPerFrame;
    while (sampleOffset < sampleTime) {
      generate_audio(slice, sliceSamples, sampleOffset);
      res = engateAddAudioFrame(engate, slice);
      if (res < 0) return fprintf(stderr, "Could not add audio frame: %d\n", res);
      sampleOffset += sliceSamples;
    }
  }
  engateClose(&engate);
  stbi_image_free(data);
  return 0;
}
