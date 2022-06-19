#include "SpikeFilterer.h"

using namespace std;

namespace HSDetection
{
    SpikeFilterer::SpikeFilterer(ProbeLayout *pLayout, int noiseDuration)
        : pLayout(pLayout), framesFilter(noiseDuration + 1) {}

    SpikeFilterer::~SpikeFilterer() {}

    void SpikeFilterer::operator()(SpikeQueue *pQueue)
    {
        SpikeQueue::iterator itMax = pQueue->begin();
        int maxAmp = itMax->amplitude;

        int spikeChannel = itMax->channel; // channel of first in queue
        int frameBound = itMax->frame + framesFilter;

        for (SpikeQueue::iterator it = pQueue->begin();
             it->frame < frameBound && it != pQueue->end();
             ++it)
        {
            // TODO: get the max of last time/channel?
            if (pLayout->areNeighbors(it->channel, spikeChannel) && it->amplitude >= maxAmp)
            {
                itMax = it;
                maxAmp = it->amplitude;
            }
        }

        spikeChannel = itMax->channel; // channel of max

        pQueue->push_front(move(*itMax));
        pQueue->erase(itMax);

        pQueue->remove_if(
            [this, spikeChannel, maxAmp](const Spike &spike)
            { return pLayout->areNeighbors(spike.channel, spikeChannel) && spike.amplitude < maxAmp; });
    }

} // namespace HSDetection
