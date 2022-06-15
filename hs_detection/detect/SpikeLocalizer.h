#ifndef SPIKELOCALIZER_H
#define SPIKELOCALIZER_H

#include "SpikeProcessor.h"

namespace HSDetection
{
    class SpikeLocalizer : public SpikeProcessor
    {
    public:
        using SpikeProcessor::operator();
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
