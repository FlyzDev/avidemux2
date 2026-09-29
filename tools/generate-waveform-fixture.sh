#!/usr/bin/env bash
set -euo pipefail

OUT="${1:-windows-waveform-multitrack-test.mkv}"

ffmpeg -hide_banner -loglevel error -y \
  -f lavfi -i 'testsrc2=size=640x360:rate=30:duration=12' \
  -f lavfi -i 'sine=frequency=440:sample_rate=48000:duration=12' \
  -f lavfi -i 'sine=frequency=660:sample_rate=48000:duration=12' \
  -f lavfi -i 'sine=frequency=880:sample_rate=48000:duration=12' \
  -f lavfi -i 'sine=frequency=1100:sample_rate=48000:duration=12' \
  -filter_complex '[1:a]volume=0.25[t1l];[2:a]volume=0.55[t1r];[t1l][t1r]join=inputs=2:channel_layout=stereo:map=0.0-FL|1.0-FR[t1];[3:a]volume=0.40[t2l];[4:a]volume=0.85[t2r];[t2l][t2r]join=inputs=2:channel_layout=stereo:map=0.0-FL|1.0-FR[t2]' \
  -map 0:v -map '[t1]' -map '[t2]' \
  -c:v libx264 -pix_fmt yuv420p -preset veryfast \
  -c:a aac \
  -metadata:s:a:0 title='Track 1 - L440 R660' \
  -metadata:s:a:1 title='Track 2 - L880 R1100' \
  "$OUT"

ffprobe -v error \
  -show_entries stream=index,codec_type,channels,channel_layout,sample_rate:stream_tags=title \
  -of compact "$OUT"
