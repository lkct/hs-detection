#ifndef SPIKELOCALIZER_H
#define SPIKELOCALIZER_H

#include "Point.h"
#include "Spike.h"

namespace HSDetection
{
    class SpikeLocalizer
    {
    public:
        Point operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
