#ifndef SPIKELOCALIZER_H
#define SPIKELOCALIZER_H

#include "SpikeProcessor.h"
#include "../ProbeLayout.h"
#include "../VoltTrace.h"
#include "../RollingArray.h"

namespace HSDetection
{
    class SpikeLocalizer : public SpikeProcessor
    {
    private:
        ProbeLayout *pLayout; // passed in, should not release here
        int noiseDuration;
        int spikePeakDuration;
        int numCoMCenters;
        VoltTrace *pTrace;
        VoltTrace *pAGlobal;
        RollingArray *pBaseline;

    public:
        SpikeLocalizer(ProbeLayout *pLayout, int noiseDuration, int spikePeakDuration, int numCoMCenters,
                       VoltTrace *pTrace, VoltTrace *pAGlobal, RollingArray *pBaseline);
        ~SpikeLocalizer();

    public:
        using SpikeProcessor::operator(); // allow call on iterator
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
