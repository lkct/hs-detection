#ifndef SPIKELOCALIZER_H
#define SPIKELOCALIZER_H

#include "Spike.h"

namespace HSDetection
{
    class SpikeLocalizer
    {
    public:
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
