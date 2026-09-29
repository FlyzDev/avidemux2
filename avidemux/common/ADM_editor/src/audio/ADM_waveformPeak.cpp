#include "ADM_waveformPeak.h"

#include <algorithm>
#include <cmath>
#include <limits>

ADM_WaveformPeakAccumulator::ADM_WaveformPeakAccumulator(uint32_t sampleRate,
                                                         uint32_t channels,
                                                         uint64_t durationUs,
                                                         uint32_t targetBins)
    : _sampleRate(sampleRate),
      _channels(channels),
      _binCount(0),
      _durationUs(durationUs),
      _totalFrames(0)
{
    if (!_sampleRate || !_channels || !_durationUs || !targetBins)
        return;

    long double totalFrames = (static_cast<long double>(_durationUs) * _sampleRate) / 1000000.0L;
    if (totalFrames < 1.0L)
        totalFrames = 1.0L;
    if (totalFrames > static_cast<long double>(std::numeric_limits<uint64_t>::max()))
        totalFrames = static_cast<long double>(std::numeric_limits<uint64_t>::max());

    _totalFrames = static_cast<uint64_t>(totalFrames + 0.5L);
    _binCount = static_cast<uint32_t>(std::min<uint64_t>(targetBins, _totalFrames));
    if (!_binCount)
        _binCount = 1;

    _peaks.assign(_channels, std::vector<float>(_binCount, 0.0f));
}

bool ADM_WaveformPeakAccumulator::valid(void) const
{
    return _sampleRate && _channels && _durationUs && _totalFrames && _binCount &&
           _peaks.size() == _channels;
}

void ADM_WaveformPeakAccumulator::reset(void)
{
    for (size_t channel = 0; channel < _peaks.size(); ++channel)
        std::fill(_peaks[channel].begin(), _peaks[channel].end(), 0.0f);
}

uint32_t ADM_WaveformPeakAccumulator::frameToBin(uint64_t frameIndex) const
{
    if (!_binCount || !_totalFrames)
        return 0;
    if (frameIndex >= _totalFrames)
        return _binCount - 1;

    const long double scaled = (static_cast<long double>(frameIndex) * _binCount) /
                               static_cast<long double>(_totalFrames);
    uint64_t bin = static_cast<uint64_t>(scaled);
    if (bin >= _binCount)
        bin = _binCount - 1;
    return static_cast<uint32_t>(bin);
}

void ADM_WaveformPeakAccumulator::addInterleaved(const float *pcm,
                                                  uint32_t frameCount,
                                                  uint64_t startTimeUs)
{
    if (!valid() || !pcm || !frameCount)
        return;

    const long double firstFrameLd = (static_cast<long double>(startTimeUs) * _sampleRate) / 1000000.0L;
    uint64_t firstFrame = 0;
    if (firstFrameLd > 0.0L)
    {
        if (firstFrameLd >= static_cast<long double>(std::numeric_limits<uint64_t>::max()))
            firstFrame = std::numeric_limits<uint64_t>::max();
        else
            firstFrame = static_cast<uint64_t>(firstFrameLd + 0.5L);
    }

    for (uint32_t frame = 0; frame < frameCount; ++frame)
    {
        if (firstFrame > std::numeric_limits<uint64_t>::max() - frame)
            break;
        const uint64_t timelineFrame = firstFrame + frame;
        if (timelineFrame >= _totalFrames)
            break;

        const uint32_t bin = frameToBin(timelineFrame);
        const size_t base = static_cast<size_t>(frame) * _channels;
        for (uint32_t channel = 0; channel < _channels; ++channel)
        {
            const float sample = pcm[base + channel];
            if (!std::isfinite(sample))
                continue;
            const float amplitude = std::fabs(sample);
            if (amplitude > _peaks[channel][bin])
                _peaks[channel][bin] = amplitude;
        }
    }
}
