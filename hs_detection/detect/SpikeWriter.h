#ifndef SPIKEWRITER_H
#define SPIKEWRITER_H

#include "Spike.h"

namespace HSDetection
{
    class SpikeWriter
    {
    public:
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
