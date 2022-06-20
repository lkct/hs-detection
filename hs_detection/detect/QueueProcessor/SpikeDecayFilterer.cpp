#include <algorithm>

#include "SpikeDecayFilterer.h"

using namespace std;

namespace HSDetection
{
    SpikeDecayFilterer::SpikeDecayFilterer(ProbeLayout *pLayout, int noiseDuration, float noiseAmpPercent)
        : pLayout(pLayout), noiseDuration(noiseDuration), noiseAmpPercent(noiseAmpPercent) {}

    SpikeDecayFilterer::~SpikeDecayFilterer() {}

    void SpikeDecayFilterer::operator()(SpikeQueue *pQueue)
    {
        SpikeQueue::iterator itMax = pQueue->begin();
        int maxAmp = itMax->amplitude;

        int spikeChannel = itMax->channel; // channel of first in queue
        int frameBound = itMax->frame + noiseDuration + 1;

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
        frameBound = itMax->frame + noiseDuration + 1;

        Spike maxSpike = move(*itMax);
        pQueue->erase(itMax);

        if (!pQueue->empty())
        {
            filterOuterNeighbors(pQueue, maxSpike);
        }

        pQueue->push_front(move(maxSpike));

        // TODO: with frameBound only in decay???
        pQueue->remove_if([this, frameBound, spikeChannel, maxAmp](const Spike &spike)
                          { return spike.frame < frameBound &&
                                   pLayout->areInnerNeighbors(spike.channel, spikeChannel) &&
                                   spike.amplitude < maxAmp; });
    }

    void SpikeDecayFilterer::filterOuterNeighbors(SpikeQueue *pQueue, Spike maxSpike)
    {
        int frameBound = maxSpike.frame + noiseDuration + 1;

        // pQueue->remove_if(
        //         [this, pQueue, &maxSpike, frameBound](const Spike &currSpike)
        //         { return currSpike.frame < frameBound &&
        //                  pLayout->areNeighbors(maxSpike.channel, currSpike.channel) &&
        //                  !pLayout->areInnerNeighbors(maxSpike.channel, currSpike.frame) &&
        //                  filteredOuterSpike(pQueue, currSpike, maxSpike); });

        vector<Spike> outerSpikes;

        copy_if(pQueue->begin(), pQueue->end(), back_inserter(outerSpikes),
                [this, pQueue, &maxSpike, frameBound](const Spike &currSpike)
                { return currSpike.frame < frameBound &&
                         pLayout->areOuterNeighbors(maxSpike.channel, currSpike.channel) &&
                         filteredOuterSpike(pQueue, currSpike, maxSpike); });

        for_each(outerSpikes.begin(), outerSpikes.end(),
                 [pQueue](const Spike &currSpike)
                 { pQueue->remove_if([&currSpike](const Spike &spike)
                                     { return spike.channel == currSpike.channel && spike.frame == currSpike.frame; }); });
    }

    bool SpikeDecayFilterer::filteredOuterSpike(SpikeQueue *pQueue, Spike outerSpike, Spike maxSpike)
    {
        for (int sharedInnerNeighbor : pLayout->getInnerNeighbors(outerSpike.channel))
        {
            if (pLayout->areInnerNeighbors(maxSpike.channel, sharedInnerNeighbor))
            {
                SpikeQueue::iterator innerSpike = find_if(pQueue->begin(), pQueue->end(),
                                                          [sharedInnerNeighbor](const Spike &spike)
                                                          { return spike.channel == sharedInnerNeighbor; });
                if (innerSpike != pQueue->end()) // exist spike
                {
                    return outerSpike.amplitude < innerSpike->amplitude * noiseAmpPercent && // decayed
                           innerSpike->frame - noiseDuration <= outerSpike.frame;            // not too early
                }
            }
        }

        float outerDist = pLayout->getChannelDistance(outerSpike.channel, maxSpike.channel);
        for (int currInnerChannel : pLayout->getInnerNeighbors(outerSpike.channel))
        {
            if (pLayout->getChannelDistance(currInnerChannel, maxSpike.channel) < outerDist)
            {
                SpikeQueue::iterator innerSpike = find_if(pQueue->begin(), pQueue->end(),
                                                          [currInnerChannel](const Spike &spike)
                                                          { return spike.channel == currInnerChannel; });
                if (innerSpike != pQueue->end() &&                                    // exist spike
                    outerSpike.amplitude < innerSpike->amplitude * noiseAmpPercent && // and decayed
                    innerSpike->frame - noiseDuration <= outerSpike.frame)            // and not too early
                {
                    return filteredOuterSpike(pQueue, *innerSpike, maxSpike);
                }
            }
        }
        return false;
    }

} // namespace HSDetection
