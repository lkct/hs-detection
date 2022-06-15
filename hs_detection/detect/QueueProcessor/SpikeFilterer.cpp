#include "SpikeFilterer.h"
#include "../Detection.h"
#include <utility>
#include "../Utils.h"

using namespace std;

namespace HSDetection
{
    void SpikeFilterer::operator()(SpikeQueue *pQueue)
    {
        SpikeQueue::const_iterator itMax = pQueue->begin();
        int maxAmp = itMax->amplitude;
        int spikeChannel = pQueue->begin()->channel;
        int frameBound = pQueue->begin()->frame + Detection::noise_duration; // TODO: useless???

        for (SpikeQueue::const_iterator it = pQueue->begin(); // TODO: use std::max_ele
             it->frame <= frameBound && it != pQueue->end();
             ++it)
        {
            if (Utils::areNeighbors(it->channel, spikeChannel) && it->amplitude >= maxAmp)
            {
                itMax = it;
                maxAmp = it->amplitude;
            }
        }

        Spike maxSpike = move(*itMax);
        pQueue->erase(itMax);
        spikeChannel = maxSpike.channel;

        // NOTE: if empty, == end() will immediately hold
        for (SpikeQueue::const_iterator it = pQueue->begin(); it != pQueue->end();
             (Utils::areNeighbors(it->channel, spikeChannel) && it->amplitude < maxAmp)
                 ? (it = pQueue->erase(it))
                 : (++it))
        {
        }

        pQueue->push_front(move(maxSpike));
    }

} // namespace HSDetection
