/* helix-mp3-pico: the Helix fixed-point MP3 decoder for the RP2040.
 * The API is mp3dec.h, this adds the sizes for static buffers.
 *
 * Give the decoder its memory before MP3InitDecoder():
 *   static unsigned char pool[HELIX_MP3_POOL_BYTES] __attribute__((aligned(4)));
 *   MP3SetPool(pool, sizeof(pool));
 *   HMP3Decoder dec = MP3InitDecoder();   // 0 if the pool is too small
 * The pool is one per program: MP3SetPool() again before another decoder. */
#ifndef HELIX_MP3_PICO_H
#define HELIX_MP3_PICO_H

#include "mp3dec.h"

#define HELIX_MP3_PICO_VERSION "1.0.0"

/* The RPSL's Object Code Notice and where the source is, to show with your
 * other copyright notices when you ship a build (see README.md). */
#define HELIX_MP3_PICO_NOTICE \
  "Helix DNA Client technology included. Copyright (c) RealNetworks, Inc., 1995-2002. All rights reserved. " \
  "Source code of the Helix MP3 decoder used here is available under the RealNetworks Public Source License " \
  "at https://github.com/roge-rm/helix-mp3-pico"

/* Enough for MP3PoolBytes() on the RP2040 (25864 bytes) and on 64-bit hosts
 * (25904 bytes), with the split polyphase table. */
#define HELIX_MP3_POOL_BYTES 25904

/* Samples (int16) one MP3Decode() can write: 1152 per channel, interleaved. */
#define HELIX_MP3_PCM_SAMPLES (MAX_NCHAN * MAX_NGRAN * MAX_NSAMP)

#endif
