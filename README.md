# helix-mp3-pico

A port of the RealNetworks Helix fixed-point MP3 decoder, packaged for the RP2040 (Raspberry Pi Pico) with Arduino-Pico, PlatformIO or the plain pico-sdk. It isn't an official Helix release and isn't affiliated with RealNetworks.

It decodes MPEG-1, MPEG-2 and MPEG-2.5 layer 3, mono or stereo, CBR or VBR, to 16-bit PCM. Layers 1 and 2 aren't supported.

I wrote it up as a library from the copy I use in [midiTracker](https://github.com/roge-rm/midiTracker), which plays MP3s from an SD card on core 1.

## What's different

From upstream Helix and from [arduino-libhelix](https://github.com/pschatzmann/arduino-libhelix):

- No heap. You give the decoder one block of memory with `MP3SetPool()`, and `MP3PoolBytes()` says how big it has to be. A static array works.
- The Cortex-M0+ has no 32x32 to 64-bit multiply, so a stock build calls a library routine for every one. Here the high word is built from four 16-bit products in inline asm.
- The polyphase filter runs as dot products of 16-bit pieces with 32-bit sums (`MP3_POLY_SPLIT`, on by default on the M0+), from a table built in the pool by `MP3InitDecoder()`. That code runs from RAM.
- The output is the same to the bit as the plain Helix C code. The host test checks this.
- There's no C++ wrapper. The API is Helix's own `mp3dec.h`.

Each changed file has dated notes at the top, and VENDORED.txt lists the changes.

## Install

PlatformIO, in `platformio.ini`:

```ini
lib_deps = https://github.com/roge-rm/helix-mp3-pico.git
```

Arduino IDE: download the repo as a zip and use Sketch > Include Library > Add .ZIP Library.

pico-sdk: `add_subdirectory(helix-mp3-pico)` after `pico_sdk_init()`, then `target_link_libraries(your_app helix_mp3_pico)`.

It's faster built with `-O2`. Arduino-Pico builds with `-Os` by default, where GCC doesn't inline the multiply. In PlatformIO:

```ini
build_unflags = -Os
build_flags = -O2
```

## Use

```c
#include <helix_mp3_pico.h>

static unsigned char pool[HELIX_MP3_POOL_BYTES] __attribute__((aligned(4)));
static short pcm[HELIX_MP3_PCM_SAMPLES];

MP3SetPool(pool, sizeof(pool));
HMP3Decoder dec = MP3InitDecoder();   // 0 if the pool is too small

// buf holds MP3 data, left is how many bytes of it there are
int off = MP3FindSyncWord(buf, left); // -1 if there's no frame in buf
p = buf + off;
left -= off;
int err = MP3Decode(dec, &p, &left, pcm, 0);
if (err == 0) {
    MP3FrameInfo fi;
    MP3GetLastFrameInfo(dec, &fi);
    // fi.outputSamps samples in pcm, fi.nChans channels at fi.samprate Hz
}
```

- `HELIX_MP3_POOL_BYTES` is 25904, enough for the RP2040 (25864) and 64-bit hosts. The pool is one per program: call `MP3SetPool()` again before you start another decoder. `MP3FreeDecoder()` only forgets the pool, it's yours to reuse.
- `MP3Decode()` decodes one frame and moves `p` and `left` past it. Give it at least one whole frame (up to 1441 bytes). When reading from a file, move what's left to the start of your buffer and top it up before each frame.
- The PCM is signed 16-bit, interleaved L R L R for stereo. A frame is 1152 samples per channel for MPEG-1 and 576 for MPEG-2 and 2.5.
- `ERR_MP3_INDATA_UNDERFLOW` means it needs more data. `ERR_MP3_MAINDATA_UNDERFLOW` is normal for the first frame or two after a start or seek, while the bit reservoir fills; skip it. For other errors skip a byte and find the next sync.
- Helix trusts the stream. If your files might not be MP3s, check the frame header yourself before decoding, as midiTracker does.

examples/DecodeArray decodes a short MP3 held in flash and prints each frame's format, decode time and peak level.

## Size and speed

From the DecodeArray build (Arduino-Pico, `-Os`): about 28 KB of flash, 772 bytes of code and 640 bytes of tables in RAM, plus the pool and your buffers (4608 bytes of PCM for one stereo frame).

From midiTracker's device profiling at 200 MHz with `-O2`: a 320 kbps stereo file takes about 8.8 ms to decode each 26.1 ms frame, about a third of one core. With the stock Helix C code the same player took 24 to 25 ms a frame on my test files, about 3 times as long.

## Test

```sh
make -C test
```

This builds the decoder twice on the host, once the way the RP2040 runs it (`MP3_POLY_SPLIT` and the 16-bit multiply in C) and once with the plain Helix C code, decodes the same MP3s with both and checks the PCM is the same to the bit and isn't silent. It also checks `MP3PoolBytes()` fits in `HELIX_MP3_POOL_BYTES`. The MP3s (CBR, VBR, joint stereo, mono, MPEG-2, MPEG-2.5) are made with ffmpeg in build/test; without ffmpeg it only uses the example's MP3. Then it feeds random bytes and bit-flipped MP3s to both builds under ASan and UBSan, 3000 runs with a fixed seed.

## Licence

The decoder is covered by the RealNetworks Public Source License 1.0 (RPSL.txt), or the RealNetworks Community Source License (RCSL.txt) if you hold one. LICENSE.txt has RealNetworks' notice. The source of the covered code, with my changes, is in this repo.

If you ship firmware built with this, the RPSL asks you to:

- say in the code and in your docs that the source of the covered code is available under the RPSL, and where (this repo),
- show its Object Code Notice where your other copyright notices go: "Helix DNA Client technology included. Copyright (c) RealNetworks, Inc., 1995-2002. All rights reserved.",
- publish the source of any changes you make to these files, with dated notes in each changed file.

`HELIX_MP3_PICO_NOTICE` in helix_mp3_pico.h has the notice and the link. Section 4 of the RPSL also has terms for programs that combine this code with other code, so read it before you ship.

## Credits

- The Helix MP3 decoder by RealNetworks (Jon Recker and Ken Cooke).
- The sources came from Phil Schatzmann's [arduino-libhelix](https://github.com/pschatzmann/arduino-libhelix), commit f6e55a3.
- Used in [midiTracker](https://github.com/roge-rm/midiTracker).
