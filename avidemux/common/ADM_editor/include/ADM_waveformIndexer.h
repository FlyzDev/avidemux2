#pragma once

#include "ADM_waveformCache.h"

#include <stdint.h>
#include <string>

typedef bool (*ADM_WaveformCancelFn)(void *opaque);

bool ADM_generateWaveform(const ADM_WaveformSnapshot &snapshot,
                          uint32_t targetBins,
                          ADM_WaveformCacheData *output,
                          std::string *errorMessage = NULL,
                          ADM_WaveformCancelFn cancel = NULL,
                          void *cancelOpaque = NULL);
