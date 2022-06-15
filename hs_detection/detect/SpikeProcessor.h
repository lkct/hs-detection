#ifndef SPIKEPROCESSOR_H
#define SPIKEPROCESSOR_H

#include "SpikeQueue.h"

namespace HSDetection
{
    class SpikeProcessor
    {
    public:
        virtual void operator()(Spike *pSpike) = 0;

        void operator()(SpikeQueue::iterator itSpike) { (*this)(&*itSpike); };
    };

} // namespace HSDetection

#endif
