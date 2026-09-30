# Imported marker navigation regression fixture

Generate a 3-minute 29.97 fps H.264 source whose frame PTS values do not line up with every whole-second timeline timestamp:

    ffmpeg -y -f lavfi -i testsrc2=size=320x180:rate=30000/1001 -t 180 \
      -c:v libx264 -preset ultrafast -g 300 -keyint_min 300 -sc_threshold 0 \
      -pix_fmt yuv420p -an avidemux-marker-repro-180s-2997.mp4

Open the generated MP4 in Avidemux, import `marker-navigation-5.xml`, then navigate Next through all five markers and Previous back through all five markers.

Expected:

- no `Error seeking to ... ms` modal;
- navigation lands on the marker timestamp or the nearest decodable frame;
- 15 s / 45 s / 75 s / 105 s / 150 s can be traversed repeatedly in both directions;
- normal A/B marker navigation is unchanged.

This fixture reproduced the old exact-PTS failure on Windows before the robust imported-marker seek fix.
