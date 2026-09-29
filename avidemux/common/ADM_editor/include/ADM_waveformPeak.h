#pragma once

#include <stdint.h>
#include <vector>

class ADM_WaveformPeakAccumulator
{
public:
    ADM_WaveformPeakAccumulator(uint32_t sampleRate,
                                uint32_t channels,
                                uint64_t durationUs,
                                uint32_t targetBins = 4096);

    bool valid(void) const;
    void reset(void);

    // PCM is interleaved by channel. frameCount is the number of sample frames,
    // not the total number of float values. startTimeUs is the DTS/PTS of the
    // first frame in the packet on the editor timeline.
    void addInterleaved(const float *pcm, uint32_t frameCount, uint64_t startTimeUs);

    uint32_t sampleRate(void) const { return _sampleRate; }
    uint32_t channels(void) const { return _channels; }
    uint32_t binCount(void) const { return _binCount; }
    uint64_t durationUs(void) const { return _durationUs; }

    const std::vector<std::vector<float> > &channelPeaks(void) const { return _peaks; }

private:
    uint32_t _sampleRate;
    uint32_t _channels;
    uint32_t _binCount;
    uint64_t _durationUs;
    uint64_t _totalFrames;
    std::vector<std::vector<float> > _peaks;

    uint32_t frameToBin(uint64_t frameIndex) const;
};
