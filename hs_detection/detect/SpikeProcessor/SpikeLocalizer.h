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
        TraceWrapper *pRef;      // passed in, should not release here
        RollingArray *pBaseline; // passed in, should not release here

        IntChannel numCoMCenters; // TODO: name???
        IntFrame noiseDuration;
        IntFrame spikePeakDuration;

    public:
        SpikeLocalizer(ProbeLayout *pLayout, TraceWrapper *pTrace,
                       TraceWrapper *pRef, RollingArray *pBaseline,
                       IntChannel numCoMCenters, IntFrame noiseDuration, IntFrame spikePeakDuration);
        ~SpikeLocalizer();

        using SpikeProcessor::operator(); // allow call on iterator
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
