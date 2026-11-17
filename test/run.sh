#!/bin/bash
# run.sh <build dir>: see Makefile.
set -u
OUT=$1
DIR=$OUT/mp3
rm -rf "$DIR"
mkdir -p "$DIR"
FAILS=0
check() { # check <name> <file or ->
    local name=$1 in=$2
    echo "== $name"
    "$OUT/decode_ref" "$in" "$DIR/$name.ref.raw" | sed 's/^/  ref:  /'
    "$OUT/decode_pico" "$in" "$DIR/$name.pico.raw" | sed 's/^/  pico: /'
    local res
    res=$("$OUT/pcm_check" "$DIR/$name.pico.raw" "$DIR/$name.ref.raw")
    echo "  $res"
    case "$res" in ok*) ;; *) FAILS=$((FAILS + 1)) ;; esac
}
check sine_mp3 -
if command -v ffmpeg > /dev/null && ffmpeg -hide_banner -encoders 2>/dev/null | grep -q libmp3lame; then
    ff() { ffmpeg -nostdin -loglevel error -y "$@"; }
    SIG="0.3*sin(2*PI*440*t)+0.15*sin(2*PI*(200+400*t)*t)+0.05*(random(0)-0.5)|0.25*sin(2*PI*660*t+1)+0.2*sin(2*PI*3000*t)+0.05*(random(1)-0.5)"
    gen() { # gen <name> <rate> <seconds> <ffmpeg options...>
        local name=$1 rate=$2 d=$3; shift 3
        ff -f lavfi -i "aevalsrc='${SIG}':s=$rate:d=$d" "$@" "$DIR/$name.mp3"
        check "$name" "$DIR/$name.mp3"
    }
    gen cbr128 44100 4 -c:a libmp3lame -b:a 128k
    gen cbr320 44100 4 -c:a libmp3lame -b:a 320k
    gen vbr 48000 4 -c:a libmp3lame -q:a 2
    gen joint 44100 3 -c:a libmp3lame -b:a 96k -joint_stereo 1
    gen mono 32000 3 -ac 1 -c:a libmp3lame -q:a 5
    gen mpeg2 22050 3 -c:a libmp3lame -b:a 64k
    gen mpeg2mono 16000 3 -ac 1 -c:a libmp3lame -b:a 24k
    gen mpeg25 11025 3 -c:a libmp3lame -b:a 32k
    gen mpeg25mono 8000 3 -ac 1 -c:a libmp3lame -b:a 16k
else
    echo "no ffmpeg with libmp3lame, only sine_mp3 checked"
fi
FUZZ=$(ls "$DIR"/{mpeg2,mpeg25,mpeg25mono}.mp3 2>/dev/null)
for b in ref pico; do
    echo "== fuzz $b"
    if UBSAN_OPTIONS=print_stacktrace=1 "$OUT/fuzz_$b" $FUZZ > "$DIR/fuzz_$b.log" 2>&1; then
        tail -1 "$DIR/fuzz_$b.log" | sed 's/^/  /'
    else
        head -20 "$DIR/fuzz_$b.log" | sed 's/^/  /'
        FAILS=$((FAILS + 1))
    fi
done
echo "$FAILS failed"
[ "$FAILS" = 0 ]
