#include "ADM_waveformPeak.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

static bool closeEnough(float a, float b)
{
    return std::fabs(a - b) < 0.0001f;
}

int main()
{
    ADM_WaveformPeakAccumulator acc(8, 2, 1000000, 4);
    assert(acc.valid());
    assert(acc.binCount() == 4);
    assert(acc.channelPeaks().size() == 2);

    const float pcm[] = {
         0.10f, -0.20f,
         0.40f,  0.30f,
        -0.25f,  0.80f,
         0.60f, -0.50f,
         0.20f,  0.10f,
        -0.90f,  0.70f,
         0.35f, -0.45f,
         0.55f,  0.65f
    };
    acc.addInterleaved(pcm, 8, 0);

    const std::vector<std::vector<float> > &p = acc.channelPeaks();
    assert(closeEnough(p[0][0], 0.40f));
    assert(closeEnough(p[1][0], 0.30f));
    assert(closeEnough(p[0][1], 0.60f));
    assert(closeEnough(p[1][1], 0.80f));
    assert(closeEnough(p[0][2], 0.90f));
    assert(closeEnough(p[1][2], 0.70f));
    assert(closeEnough(p[0][3], 0.55f));
    assert(closeEnough(p[1][3], 0.65f));

    const float update[] = {
        0.95f, 0.15f,
        std::numeric_limits<float>::quiet_NaN(), -0.99f
    };
    acc.addInterleaved(update, 2, 500000);
    assert(closeEnough(p[0][2], 0.95f));
    assert(closeEnough(p[1][2], 0.99f));

    acc.reset();
    for (size_t c = 0; c < p.size(); ++c)
        for (size_t b = 0; b < p[c].size(); ++b)
            assert(closeEnough(p[c][b], 0.0f));

    std::cout << "waveform peak accumulator OK\n";
    return 0;
}
