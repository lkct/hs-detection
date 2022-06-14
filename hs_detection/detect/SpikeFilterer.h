#ifndef SPIKEFILTERER_H
#define SPIKEFILTERER_H

#include "Spike.h"
#include "SpikeQueue.h"

namespace HSDetection
{
    class SpikeFilterer
    {
    public: // TODO: need a input??? or always first
        Spike operator()(SpikeQueue *queue, SpikeQueue::const_iterator itSpike);
    };

} // namespace HSDetection

#endif
