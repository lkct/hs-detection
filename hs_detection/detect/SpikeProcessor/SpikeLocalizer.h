#ifndef SPIKELOCALIZER_H
#define SPIKELOCALIZER_H

#include "SpikeProcessor.h"
#include "../ProbeLayout.h"
#include "../TraceWrapper.h"
#include "../RollingArray.h"

namespace HSDetection
{
    class SpikeLocalizer : public SpikeProcessor
    {
    private:
        ProbeLayout *pLayout; // passed in, should not release here

        TraceWrapper *pTrace;    // passed in, should not release here
        TraceWrapper *pAGlobal;  // passed in, should not release here
        RollingArray *pBaseline; // passed in, should not release here

        int numCoMCenters; // TODO: name???
        int noiseDuration;
        int spikePeakDuration;

    public:
        SpikeLocalizer(ProbeLayout *pLayout, TraceWrapper *pTrace,
                       TraceWrapper *pAGlobal, RollingArray *pBaseline,
                       int numCoMCenters, int noiseDuration, int spikePeakDuration);
        ~SpikeLocalizer();

        using SpikeProcessor::operator(); // allow call on iterator
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
