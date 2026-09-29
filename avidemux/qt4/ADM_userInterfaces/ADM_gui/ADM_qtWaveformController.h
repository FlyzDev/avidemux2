#pragma once

class ADM_Composer;
class ADM_mwWaveform;

namespace ADM_QtWaveformController
{
    void schedule(ADM_Composer *composer, ADM_mwWaveform *waveform);
    void cancel(void);
}
