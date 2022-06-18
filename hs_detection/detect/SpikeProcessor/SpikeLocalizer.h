#ifndef SPIKELOCALIZER_H
#define SPIKELOCALIZER_H

#include "SpikeProcessor.h"
#include "../ProbeLayout.h"

namespace HSDetection
{
    class SpikeLocalizer : public SpikeProcessor
    {
    private:
        ProbeLayout *pLayout; // passed in, should not release here

    public:
        SpikeLocalizer(ProbeLayout *pLayout);
        ~SpikeLocalizer();

    public:
        using SpikeProcessor::operator(); // allow call on iterator
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
