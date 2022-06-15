#ifndef QUEUEPROCESSOR_H
#define QUEUEPROCESSOR_H

#include "../SpikeQueue.h"

namespace HSDetection
{
    class QueueProcessor
    {
    public:
        QueueProcessor() {}
        virtual ~QueueProcessor() {}

        virtual void operator()(SpikeQueue *pQueue) = 0;
    };

} // namespace HSDetection

#endif
