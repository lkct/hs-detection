#ifndef SPIKEFILTERER_H
#define SPIKEFILTERER_H

#include "QueueProcessor.h"

namespace HSDetection
{
    class SpikeFilterer : public QueueProcessor
    {
    public:
        void operator()(SpikeQueue *pQueue);
    };

} // namespace HSDetection

#endif
