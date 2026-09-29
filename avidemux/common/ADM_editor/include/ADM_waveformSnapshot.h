#pragma once

#include <stdint.h>
#include <string>
#include <vector>

class ADM_Composer;

enum ADM_WaveformTrackSourceType
{
    ADM_WAVEFORM_TRACK_INTERNAL = 0,
    ADM_WAVEFORM_TRACK_EXTERNAL = 1
};

struct ADM_WaveformSourceSnapshot
{
    std::string fileName;
};

struct ADM_WaveformSegmentSnapshot
{
    uint32_t reference;
    uint64_t referenceStartUs;
    uint64_t timelineStartUs;
    uint64_t durationUs;
};

struct ADM_WaveformTrackSnapshot
{
    ADM_WaveformTrackSourceType sourceType;
    int activeIndex;
    int poolIndex;
    int internalTrackIndex;
    std::string externalFileName;
    uint32_t outputChannels;
    uint32_t outputFrequency;
};

struct ADM_WaveformSnapshot
{
    uint64_t durationUs;
    std::vector<ADM_WaveformSourceSnapshot> sources;
    std::vector<ADM_WaveformSegmentSnapshot> segments;
    std::vector<ADM_WaveformTrackSnapshot> tracks;

    ADM_WaveformSnapshot() : durationUs(0) {}
    void clear()
    {
        durationUs = 0;
        sources.clear();
        segments.clear();
        tracks.clear();
    }
};

bool ADM_buildWaveformSnapshot(ADM_Composer *composer, ADM_WaveformSnapshot *snapshot);
