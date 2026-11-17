// Decodes the MP3 in sine_mp3.h frame by frame and prints each frame's
// format, time to decode and peak level over serial. No heap: the decoder
// gets a static pool.
#include <helix_mp3_pico.h>
#include "sine_mp3.h"

static uint8_t pool[HELIX_MP3_POOL_BYTES] __attribute__((aligned(4)));
static int16_t pcm[HELIX_MP3_PCM_SAMPLES];  // one frame, interleaved L R L R

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000) {}
  Serial.println(HELIX_MP3_PICO_NOTICE);

  Serial.printf("pool %d bytes, decoder needs %d\n", (int)sizeof(pool), MP3PoolBytes());
  MP3SetPool(pool, sizeof(pool));
  HMP3Decoder dec = MP3InitDecoder();
  if (!dec) {
    Serial.println("MP3InitDecoder failed, pool too small");
    return;
  }

  unsigned char *p = (unsigned char *)sine_mp3;  // Helix doesn't write to it
  int left = sizeof(sine_mp3);
  int frames = 0;
  while (left > 0) {
    int off = MP3FindSyncWord(p, left);
    if (off < 0) break;
    p += off;
    left -= off;
    uint32_t t = micros();
    int err = MP3Decode(dec, &p, &left, pcm, 0);
    t = micros() - t;
    if (err == ERR_MP3_INDATA_UNDERFLOW) break;    // a cut-off last frame
    if (err == ERR_MP3_MAINDATA_UNDERFLOW) continue;  // filling the bit reservoir
    if (err) {
      Serial.printf("error %d, skipping a byte\n", err);
      p++;
      left--;
      continue;
    }
    MP3FrameInfo fi;
    MP3GetLastFrameInfo(dec, &fi);
    int peak = 0;
    for (int i = 0; i < fi.outputSamps; i++) peak = max(peak, abs((int)pcm[i]));
    Serial.printf("frame %d: %d Hz, %d ch, %d kbps, %d samples, %lu us, peak %d\n", frames, fi.samprate, fi.nChans,
                  fi.bitrate / 1000, fi.outputSamps, (unsigned long)t, peak);
    frames++;
  }
  MP3FreeDecoder(dec);
  Serial.printf("%d frames\n", frames);
}

void loop() {}
