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
        const ProbeLayout *pLayout;    // passed in, should not release here
        const TraceWrapper *pTrace;    // passed in, should not release here
        const TraceWrapper *pRef;      // passed in, should not release here
        const RollingArray *pBaseline; // passed in, should not release here

        IntFrame jitterTol;
        IntFrame peakDur;

    public:
        SpikeLocalizer(const ProbeLayout *pLayout, const TraceWrapper *pTrace,
                       const TraceWrapper *pRef, const RollingArray *pBaseline,
                       IntFrame jitterTol, IntFrame peakDur);
        ~SpikeLocalizer();

        using SpikeProcessor::operator(); // allow call on iterator
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
