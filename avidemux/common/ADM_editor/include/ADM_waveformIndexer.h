#pragma once

#include "ADM_waveformCache.h"

#include <stdint.h>
#include <string>

typedef bool (*ADM_WaveformCancelFn)(void *opaque);
typedef void (*ADM_WaveformProgressFn)(void *opaque,
                                       uint32_t trackIndex,
                                       const std::vector<std::vector<float> > &channelPeaks);

bool ADM_generateWaveform(const ADM_WaveformSnapshot &snapshot,
                          uint32_t targetBins,
                          ADM_WaveformCacheData *output,
                          std::string *errorMessage = NULL,
                          ADM_WaveformCancelFn cancel = NULL,
                          void *cancelOpaque = NULL,
                          ADM_WaveformProgressFn progress = NULL,
                          void *progressOpaque = NULL);
