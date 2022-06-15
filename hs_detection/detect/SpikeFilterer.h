#ifndef SPIKEFILTERER_H
#define SPIKEFILTERER_H

#include "Spike.h"
#include "SpikeQueue.h"

namespace HSDetection
{
    class SpikeFilterer
    {
    public: // TODO: always first???
        void operator()(SpikeQueue *queue);
    };

} // namespace HSDetection

#endif
