#!/usr/bin/env bash
set -euo pipefail

OUT="${1:-windows-waveform-multitrack-test.mkv}"

ffmpeg -hide_banner -loglevel error -y \
  -f lavfi -i 'testsrc2=size=640x360:rate=30:duration=12' \
  -f lavfi -i 'sine=frequency=440:sample_rate=48000:duration=12' \
  -f lavfi -i 'sine=frequency=880:sample_rate=48000:duration=12' \
  -filter_complex '[1:a]volume=0.35[a1];[2:a]volume=0.8[a2]' \
  -map 0:v -map '[a1]' -map '[a2]' \
  -c:v libx264 -pix_fmt yuv420p -preset veryfast \
  -c:a aac \
  -metadata:s:a:0 title='Track 1 - 440 Hz' \
  -metadata:s:a:1 title='Track 2 - 880 Hz' \
  "$OUT"

ffprobe -v error \
  -show_entries stream=index,codec_type,channels,sample_rate:stream_tags=title \
  -of compact "$OUT"
