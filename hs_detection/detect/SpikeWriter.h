#ifndef SPIKEWRITER_H
#define SPIKEWRITER_H

#include "SpikeProcessor.h"

namespace HSDetection
{
    class SpikeWriter : SpikeProcessor
    {
    public:
        using SpikeProcessor::operator();
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
