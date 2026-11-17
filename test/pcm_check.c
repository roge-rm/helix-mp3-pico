/* pcm_check <a.raw> <b.raw>
 * Says ok when the two int16 files are the same to the bit and a isn't
 * silent, else where they first differ. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static short *load(const char *name, long *n)
{
	FILE *f = fopen(name, "rb");
	if (!f) { perror(name); exit(1); }
	fseek(f, 0, SEEK_END);
	*n = ftell(f) / 2;
	rewind(f);
	short *s = malloc(*n * 2 + 2);
	if (fread(s, 2, *n, f) != (size_t)*n) { perror(name); exit(1); }
	fclose(f);
	return s;
}

int main(int argc, char **argv)
{
	long na, nb, i;
	if (argc < 3) { fprintf(stderr, "usage: pcm_check a.raw b.raw\n"); return 2; }
	short *a = load(argv[1], &na), *b = load(argv[2], &nb);
	if (na != nb) { printf("FAIL: %ld samples against %ld\n", na, nb); return 1; }
	double sum = 0;
	for (i = 0; i < na; i++) {
		if (a[i] != b[i]) { printf("FAIL: sample %ld is %d against %d\n", i, a[i], b[i]); return 1; }
		sum += (double)a[i] * a[i];
	}
	double rms = na ? sqrt(sum / na) : 0;
	if (rms < 100) { printf("FAIL: silent (rms %.1f)\n", rms); return 1; }
	printf("ok: %ld samples the same, rms %.0f\n", na, rms);
	return 0;
}
