/* decode <in.mp3> <out.raw>
 * Decodes an MP3 frame by frame with a static pool and writes the PCM
 * (interleaved int16, native byte order). With no file name it decodes the
 * example's sine_mp3 array. Prints the pool size, the first frame's format
 * and the frame count. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp3dec.h"
#include "helix_mp3_pico.h"
#include "../examples/DecodeArray/sine_mp3.h"

static unsigned char pool[HELIX_MP3_POOL_BYTES] __attribute__((aligned(4)));
static short pcm[MAX_NCHAN * MAX_NGRAN * MAX_NSAMP];

int main(int argc, char **argv)
{
	unsigned char *data = (unsigned char *)sine_mp3;
	long size = (long)sizeof(sine_mp3);
	if (argc < 3) {
		fprintf(stderr, "usage: decode <in.mp3|-> <out.raw>\n");
		return 2;
	}
	if (strcmp(argv[1], "-")) {
		FILE *f = fopen(argv[1], "rb");
		if (!f) { perror(argv[1]); return 1; }
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		rewind(f);
		data = malloc(size);
		if (fread(data, 1, size, f) != (size_t)size) { perror(argv[1]); return 1; }
		fclose(f);
	}
	FILE *out = fopen(argv[2], "wb");
	if (!out) { perror(argv[2]); return 1; }

	printf("pool: MP3PoolBytes() %d, HELIX_MP3_POOL_BYTES %d\n", MP3PoolBytes(), HELIX_MP3_POOL_BYTES);
	if (MP3PoolBytes() > (int)sizeof(pool)) {
		printf("FAIL: pool too small\n");
		return 1;
	}
	MP3SetPool(pool, sizeof(pool));
	HMP3Decoder dec = MP3InitDecoder();
	if (!dec) { printf("FAIL: MP3InitDecoder\n"); return 1; }

	unsigned char *p = data;
	int left = (int)size;
	long frames = 0, samples = 0, errors = 0;
	while (left > 0) {
		int off = MP3FindSyncWord(p, left);
		if (off < 0) break;
		p += off;
		left -= off;
		int err = MP3Decode(dec, &p, &left, pcm, 0);
		if (err == ERR_MP3_INDATA_UNDERFLOW) break;
		if (err == ERR_MP3_MAINDATA_UNDERFLOW) continue; /* bit reservoir filling */
		if (err) {
			errors++;
			if (left > 0) { p++; left--; } /* skip the bad sync */
			continue;
		}
		MP3FrameInfo fi;
		MP3GetLastFrameInfo(dec, &fi);
		if (frames == 0)
			printf("first frame: %d Hz, %d ch, %d bps, MPEG%s layer %d, %d samples\n", fi.samprate, fi.nChans, fi.bitrate,
			       fi.version == 0 ? "1" : fi.version == 1 ? "2" : "2.5", fi.layer, fi.outputSamps);
		fwrite(pcm, sizeof(short), fi.outputSamps, out);
		frames++;
		samples += fi.outputSamps;
	}
	fclose(out);
	MP3FreeDecoder(dec);
	printf("frames %ld, samples %ld, errors %ld\n", frames, samples, errors);
	return frames > 0 ? 0 : 1;
}
