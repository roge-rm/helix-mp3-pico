/* fuzz [file.mp3 ...]
 * Feeds random bytes and bit-flipped copies of real MP3s (the example's
 * sine_mp3 array and any files named) through MP3FindSyncWord() and
 * MP3Decode(). Built with -fsanitize=address,undefined, so a bad read or
 * write stops it. Fixed seed, so a failure repeats; FUZZ_SEED and
 * FUZZ_ITERS in the environment change the seed and the run count (3000). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp3dec.h"
#include "helix_mp3_pico.h"
#include "../examples/DecodeArray/sine_mp3.h"


static unsigned char pool[HELIX_MP3_POOL_BYTES] __attribute__((aligned(4)));
static short pcm[HELIX_MP3_PCM_SAMPLES];
static unsigned long long rng = 0x2545f4914f6cdd1dULL;

static unsigned rnd(void)
{
	rng ^= rng << 13;
	rng ^= rng >> 7;
	rng ^= rng << 17;
	return (unsigned)(rng >> 16);
}

/* Decodes the buffer like a player: find a sync, decode, skip a byte on an
 * error. The copy is malloc'd at its exact size so ASan sees overreads. */
static long run(const unsigned char *src, int size)
{
	unsigned char *data = malloc(size > 0 ? size : 1);
	memcpy(data, src, size);
	MP3SetPool(pool, sizeof(pool));
	HMP3Decoder dec = MP3InitDecoder();
	if (!dec) {
		printf("FAIL: MP3InitDecoder\n");
		exit(1);
	}
	unsigned char *p = data;
	int left = size;
	long frames = 0;
	for (int guard = 0; left > 0 && guard < 100000; guard++) {
		int off = MP3FindSyncWord(p, left);
		if (off < 0)
			break;
		p += off;
		left -= off;
		unsigned char *before = p;
		int err = MP3Decode(dec, &p, &left, pcm, 0);
		if (left < 0 || p < data || p > data + size) {
			printf("FAIL: MP3Decode left %d, pointer %ld\n", left, (long)(p - data));
			exit(1);
		}
		if (err == ERR_MP3_INDATA_UNDERFLOW)
			break;
		if (err == 0)
			frames++;
		else if (p == before && left > 0) {
			p++;
			left--;
		}
	}
	MP3FreeDecoder(dec);
	free(data);
	return frames;
}

int main(int argc, char **argv)
{
	int iters = getenv("FUZZ_ITERS") ? atoi(getenv("FUZZ_ITERS")) : 3000;
	if (getenv("FUZZ_SEED"))
		rng += strtoull(getenv("FUZZ_SEED"), 0, 0) * 0x9e3779b97f4a7c15ULL;
	int nsrc = argc;
	unsigned char **src = calloc(nsrc, sizeof(*src));
	int *len = calloc(nsrc, sizeof(*len));
	src[0] = (unsigned char *)sine_mp3;
	len[0] = (int)sizeof(sine_mp3);
	for (int i = 1; i < argc; i++) {
		FILE *f = fopen(argv[i], "rb");
		if (!f) { perror(argv[i]); return 1; }
		fseek(f, 0, SEEK_END);
		len[i] = (int)ftell(f);
		rewind(f);
		src[i] = malloc(len[i]);
		if (fread(src[i], 1, len[i], f) != (size_t)len[i]) { perror(argv[i]); return 1; }
		fclose(f);
	}
	int maxlen = 4096;
	for (int i = 0; i < nsrc; i++)
		if (len[i] > maxlen)
			maxlen = len[i];
	unsigned char *buf = malloc(maxlen);
	long frames = 0;
	for (int it = 0; it < iters; it++) {
		int n;
		if (it % 4 == 0) {
			/* random bytes, sometimes with sync words sprinkled in */
			n = 1 + rnd() % 4096;
			for (int i = 0; i < n; i++)
				buf[i] = rnd();
			if (it % 8 == 0)
				for (int i = 0; i + 1 < n; i += 1 + rnd() % 64) {
					buf[i] = 0xff;
					buf[i + 1] |= 0xe0;
				}
		} else {
			/* a real MP3 with bits flipped, sometimes cut short */
			int s = rnd() % nsrc;
			n = len[s];
			memcpy(buf, src[s], n);
			int flips = 1 + rnd() % 64;
			for (int i = 0; i < flips; i++)
				buf[rnd() % n] ^= 1 << (rnd() % 8);
			if (rnd() % 4 == 0)
				n = 1 + rnd() % n;
		}
		frames += run(buf, n);
	}
	printf("fuzz: %d runs, %ld frames decoded, no crash\n", iters, frames);
	for (int i = 1; i < nsrc; i++)
		free(src[i]);
	free(src);
	free(len);
	free(buf);
	return 0;
}
