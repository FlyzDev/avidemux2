#pragma once

#include "ADM_waveformSnapshot.h"

#include <stdint.h>
#include <string>
#include <vector>

struct ADM_WaveformCacheData
{
    uint64_t durationUs;
    std::vector<std::vector<std::vector<float> > > tracks;

    ADM_WaveformCacheData() : durationUs(0) {}
    void clear()
    {
        durationUs = 0;
        tracks.clear();
    }
};

std::string ADM_waveformCacheKey(const ADM_WaveformSnapshot &snapshot,
                                 uint32_t targetBins,
                                 uint32_t schemaVersion = 1);

bool ADM_writeWaveformCache(const std::string &path,
                            const std::string &cacheKey,
                            const ADM_WaveformCacheData &data);

bool ADM_readWaveformCache(const std::string &path,
                           const std::string &expectedCacheKey,
                           ADM_WaveformCacheData *data);
