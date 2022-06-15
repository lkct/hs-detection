#ifndef QUEUEPROCESSOR_H
#define QUEUEPROCESSOR_H

#include "SpikeQueue.h"

namespace HSDetection
{
    class QueueProcessor
    {
    public:
        virtual void operator()(SpikeQueue *pQueue) = 0;
    };

} // namespace HSDetection

#endif
