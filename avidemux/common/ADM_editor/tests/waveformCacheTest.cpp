#include "ADM_waveformCache.h"

#include <cassert>
#include <cstdio>
#include <iostream>

int main()
{
    ADM_WaveformSnapshot snapshot;
    snapshot.durationUs = 2000000;

    ADM_WaveformSourceSnapshot source;
    source.fileName = "/media/test.mkv";
    source.fileSize = 1234567;
    source.modifiedTime = 42;
    snapshot.sources.push_back(source);

    ADM_WaveformSegmentSnapshot segment = {};
    segment.reference = 0;
    segment.referenceStartUs = 100000;
    segment.timelineStartUs = 0;
    segment.durationUs = 2000000;
    snapshot.segments.push_back(segment);

    ADM_WaveformTrackSnapshot track;
    track.activeIndex = 0;
    track.poolIndex = 0;
    track.internalTrackIndex = 1;
    track.outputChannels = 2;
    track.outputFrequency = 48000;
    snapshot.tracks.push_back(track);

    const std::string key = ADM_waveformCacheKey(snapshot, 4096);
    assert(key.size() == 16);
    assert(key == ADM_waveformCacheKey(snapshot, 4096));

    snapshot.segments[0].referenceStartUs++;
    assert(key != ADM_waveformCacheKey(snapshot, 4096));
    snapshot.segments[0].referenceStartUs--;

    ADM_WaveformCacheData data;
    data.durationUs = snapshot.durationUs;
    data.tracks.resize(1);
    data.tracks[0].resize(2);
    data.tracks[0][0].push_back(0.25f);
    data.tracks[0][0].push_back(0.75f);
    data.tracks[0][1].push_back(0.50f);
    data.tracks[0][1].push_back(1.00f);

    const std::string path = "/tmp/avidemux-waveform-cache-test.bin";
    std::remove(path.c_str());
    assert(ADM_writeWaveformCache(path, key, data));

    ADM_WaveformCacheData loaded;
    assert(ADM_readWaveformCache(path, key, &loaded));
    assert(loaded.durationUs == data.durationUs);
    assert(loaded.tracks.size() == 1);
    assert(loaded.tracks[0].size() == 2);
    assert(loaded.tracks[0][0].size() == 2);
    assert(loaded.tracks[0][1][1] == 1.00f);

    ADM_WaveformCacheData wrong;
    assert(!ADM_readWaveformCache(path, "wrong-key", &wrong));
    std::remove(path.c_str());

    std::cout << "waveform cache roundtrip OK\n";
    return 0;
}
