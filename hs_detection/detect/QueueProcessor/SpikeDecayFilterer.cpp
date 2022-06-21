#include <algorithm>

#include "SpikeDecayFilterer.h"

using namespace std;

namespace HSDetection
{
    SpikeDecayFilterer::SpikeDecayFilterer(ProbeLayout *pLayout, IntFrame noiseDuration, float noiseAmpPercent)
        : pLayout(pLayout), noiseDuration(noiseDuration), noiseAmpPercent(noiseAmpPercent) {}

    SpikeDecayFilterer::~SpikeDecayFilterer() {}

    void SpikeDecayFilterer::operator()(SpikeQueue *pQueue)
    {
        IntFrame frameBound = pQueue->begin()->frame + noiseDuration + 1;
        IntChannel maxChannel = pQueue->begin()->channel;
        IntVolt maxAmp = pQueue->begin()->amplitude;

        { // TODO:
            // pQueue->remove_if(
            //         [this, pQueue, &maxSpike, frameBound](const Spike &spike)
            //         { return spike.frame < frameBound &&
            //                  pLayout->areNeighbors(maxSpike.channel, spike.channel) &&
            //                  !pLayout->areInnerNeighbors(maxSpike.channel, spike.frame) &&
            //                  shouldFilterOuter(pQueue, spike, maxSpike); });

            vector<Spike> outerSpikes;

            copy_if(pQueue->begin(), pQueue->end(), back_inserter(outerSpikes),
                    [this, pQueue, frameBound, maxChannel](const Spike &spike)
                    { return spike.frame < frameBound &&
                             pLayout->areOuterNeighbors(spike.channel, maxChannel) &&
                             shouldFilterOuter(pQueue, spike); });

            pQueue->remove_if([&outerSpikes](const Spike &spike)
                              { return binary_search( // outerSpikes is sorted by nature of queue
                                    outerSpikes.begin(), outerSpikes.end(), spike,
                                    [](const Spike &lhs, const Spike &rhs)
                                    { return lhs.frame < rhs.frame ||
                                             (lhs.frame == rhs.frame && lhs.channel < rhs.channel); }); });
        }

        // TODO: with frameBound only in decay???
        pQueue->remove_if([this, frameBound, maxChannel, maxAmp](const Spike &spike)
                          { return spike.frame < frameBound &&
                                   pLayout->areInnerNeighbors(spike.channel, maxChannel) &&
                                   spike.amplitude < maxAmp; });
    }

    bool SpikeDecayFilterer::shouldFilterOuter(SpikeQueue *pQueue, const Spike &outerSpike) const
    {
        IntChannel maxChannel = pQueue->begin()->channel;
        IntChannel outerChannel = outerSpike.channel;
        FloatGeom outerDist = pLayout->getChannelDistance(outerChannel, maxChannel);

        // TODO: merge loop?
        for (IntChannel innerOfOuter : pLayout->getInnerNeighbors(outerChannel))
        {
            if (pLayout->areInnerNeighbors(innerOfOuter, maxChannel))
            {
                SpikeQueue::const_iterator itSpikeOnInner = find_if(
                    pQueue->begin(), pQueue->end(),
                    [innerOfOuter](const Spike &spike)
                    { return spike.channel == innerOfOuter; });

                if (itSpikeOnInner != pQueue->end()) // exist spike on innerOfOuter
                {
                    return outerSpike.amplitude < itSpikeOnInner->amplitude * noiseAmpPercent && // and outer decayed
                           itSpikeOnInner->frame - noiseDuration <= outerSpike.frame;            // and outer not too early
                }
            }
        }

        for (IntChannel innerOfOuter : pLayout->getInnerNeighbors(outerChannel))
        {
            if (pLayout->getChannelDistance(innerOfOuter, maxChannel) < outerDist)
            {
                SpikeQueue::const_iterator itSpikeOnInner = find_if(
                    pQueue->begin(), pQueue->end(),
                    [innerOfOuter](const Spike &spike)
                    { return spike.channel == innerOfOuter; });

                if (itSpikeOnInner != pQueue->end() &&                                    // exist spike on innerOfOuter
                    outerSpike.amplitude < itSpikeOnInner->amplitude * noiseAmpPercent && // and outer decayed
                    itSpikeOnInner->frame - noiseDuration <= outerSpike.frame)            // and outer not too early
                {
                    return shouldFilterOuter(pQueue, *itSpikeOnInner);
                }
            }
        }

        return false; // no corresponding spike from inner
    }

} // namespace HSDetection
