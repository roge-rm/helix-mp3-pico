# Host test: decodes MP3s with the library built the RP2040 way (split
# polyphase, 16-bit multiplies, in C) and with the plain Helix C paths, and
# checks the PCM is the same to the bit. MP3s are made with ffmpeg in
# build/test; without ffmpeg only the example's sine_mp3 array is used.
# Then fuzz runs both builds under ASan and UBSan on random and bit-flipped
# data (shifts and signed overflow left out, the fixed-point code wraps).
#   make -C test          build and run
#   make -C test clean
ROOT := $(abspath $(CURDIR)/..)
OUT := $(ROOT)/build/test
SRC := $(wildcard $(ROOT)/src/*.c)
HDR := $(wildcard $(ROOT)/src/*.h) $(ROOT)/examples/DecodeArray/sine_mp3.h
CFLAGS := -O2 -g -w -I$(ROOT)/src

.PHONY: check clean
SAN := -O1 -g -w -fsanitize=address,undefined -fno-sanitize=shift,signed-integer-overflow \
	-fno-sanitize-recover=all -fno-omit-frame-pointer -I$(ROOT)/src

check: $(OUT)/decode_ref $(OUT)/decode_pico $(OUT)/pcm_check $(OUT)/fuzz_ref $(OUT)/fuzz_pico
	$(CURDIR)/run.sh $(OUT)

# The plain Helix paths: 64-bit multiplies, polyphase from polyCoef.
$(OUT)/decode_ref: decode.c $(SRC) $(HDR)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -o $@ decode.c $(SRC)

# What the RP2040 runs, in C: MP3_POLY_SPLIT and the 16-bit MP3_MULHS.
$(OUT)/decode_pico: decode.c $(SRC) $(HDR)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -DMP3_POLY_SPLIT -DMP3_MULHS_C -o $@ decode.c $(SRC)

$(OUT)/fuzz_ref: fuzz.c $(SRC) $(HDR)
	@mkdir -p $(OUT)
	$(CC) $(SAN) -o $@ fuzz.c $(SRC)

$(OUT)/fuzz_pico: fuzz.c $(SRC) $(HDR)
	@mkdir -p $(OUT)
	$(CC) $(SAN) -DMP3_POLY_SPLIT -DMP3_MULHS_C -o $@ fuzz.c $(SRC)

$(OUT)/pcm_check: pcm_check.c
	@mkdir -p $(OUT)
	$(CC) -O2 -o $@ pcm_check.c -lm

clean:
	rm -rf $(OUT)
