#include <algorithm>

#include "SpikeFilterer.h"
#include "../Utils.h"

using namespace std;

namespace HSDetection
{
    SpikeFilterer::SpikeFilterer(int noiseDuration) : framesFilter(noiseDuration + 1) {}

    SpikeFilterer::~SpikeFilterer() {}

    void SpikeFilterer::operator()(SpikeQueue *pQueue)
    {
        SpikeQueue::const_iterator itMax = pQueue->begin();
        int maxAmp = itMax->amplitude;
        int spikeChannel = itMax->channel;
        int frameBound = itMax->frame + framesFilter;

        // maybe not to use std::max_element is better in this case
        for (SpikeQueue::const_iterator it = pQueue->begin();
             it->frame < frameBound && it != pQueue->end();
             ++it)
        {
            // TODO: get the max of last time/channel?
            if (Utils::areNeighbors(it->channel, spikeChannel) && it->amplitude >= maxAmp)
            {
                itMax = it;
                maxAmp = it->amplitude;
            }
        }

        Spike maxSpike = move(*itMax);
        // int maxAmp = maxSpike.amplitude;
        spikeChannel = maxSpike.channel;
        pQueue->erase(itMax);

        pQueue->erase(
            remove_if(pQueue->begin(), pQueue->end(),
                      [spikeChannel, maxAmp](const Spike &spike)
                      { return Utils::areNeighbors(spike.channel, spikeChannel) && spike.amplitude < maxAmp; }),
            pQueue->end());

        pQueue->push_front(move(maxSpike));
    }

} // namespace HSDetection
