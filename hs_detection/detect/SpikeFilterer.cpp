#include "SpikeFilterer.h"
#include "Detection.h"
#include <utility>
#include "SpikeHandler.h"

using namespace std;

namespace HSDetection
{
    void SpikeFilterer::operator()(SpikeQueue *queue)
    {
        SpikeQueue::const_iterator itMax = queue->begin();
        int maxAmp = itMax->amplitude;
        int spikeChannel = queue->begin()->channel;
        int frameBound = queue->begin()->frame + Detection::noise_duration; // TODO: useless???

        for (SpikeQueue::const_iterator it = queue->begin(); // TODO: use std::max_ele
             it->frame <= frameBound && it != queue->end();
             ++it)
        {
            if (SpikeHandler::areNeighbors(it->channel, spikeChannel) && it->amplitude >= maxAmp)
            {
                itMax = it;
                maxAmp = it->amplitude;
            }
        }

        Spike maxSpike = move(*itMax);
        queue->erase(itMax);
        spikeChannel = maxSpike.channel;

        // NOTE: if empty, == end() will immediately hold
        for (SpikeQueue::const_iterator it = queue->begin(); it != queue->end();
             (SpikeHandler::areNeighbors(it->channel, spikeChannel) && it->amplitude < maxAmp)
                 ? (it = queue->erase(it))
                 : (++it))
        {
        }

        queue->push_front(move(maxSpike));
    }

} // namespace HSDetection
