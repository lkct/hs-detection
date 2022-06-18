#ifndef SPIKEPROCESSOR_H
#define SPIKEPROCESSOR_H

#include "../SpikeQueue.h"

namespace HSDetection
{
    class SpikeProcessor
    {
    public:
        SpikeProcessor() {}
        virtual ~SpikeProcessor() {}

        void operator()(SpikeQueue::iterator itSpike) { (*this)(&*itSpike); };
        virtual void operator()(Spike *pSpike) = 0;
    };

} // namespace HSDetection

#endif
