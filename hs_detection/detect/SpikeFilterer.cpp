#include "SpikeFilterer.h"
#include "FilterSpikes.h"
#include "Detection.h"

namespace HSDetection
{
    Spike SpikeFilterer::operator()(SpikeQueue *queue, SpikeQueue::const_iterator itSpike)
    {
        SpikeQueue::const_iterator itMax = itSpike;
        int maxAmp = itSpike->amplitude;
        int spikeChannel = itSpike->channel;
        int frameBound = itSpike->frame + Detection::noise_duration; // TODO: useless???

        for (SpikeQueue::const_iterator it = queue->begin();
             it->frame <= frameBound && it != queue->end();
             ++it)
        {
            if (FilterSpikes::areNeighbors(it->channel, spikeChannel) && it->amplitude >= maxAmp)
            {
                itMax = it;
                maxAmp = it->amplitude;
            }
        }

        Spike maxSpike = *itMax;
        spikeChannel = maxSpike.channel;
        queue->erase(itMax);

        // NOTE: if empty, == end() will immediately hold
        for (SpikeQueue::const_iterator it = queue->begin(); it != queue->end();
             (FilterSpikes::areNeighbors(it->channel, spikeChannel) && it->amplitude < maxAmp)
                 ? (it = queue->erase(it))
                 : (++it))
        {
        }

        return maxSpike;
    }

} // namespace HSDetection
