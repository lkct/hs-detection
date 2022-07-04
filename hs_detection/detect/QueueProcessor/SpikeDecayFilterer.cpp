#include <algorithm>
#include <utility>

#include "SpikeDecayFilterer.h"

using namespace std;

namespace HSDetection
{
    SpikeDecayFilterer::SpikeDecayFilterer(const ProbeLayout *pLayout, IntFrame jitterTol, FloatRatio decayRatio)
        : pLayout(pLayout), jitterTol(jitterTol), decayRatio(decayRatio) {}

    SpikeDecayFilterer::~SpikeDecayFilterer() {}

    void SpikeDecayFilterer::operator()(SpikeQueue *pQueue)
    {
        Spike maxSpike = move(*pQueue->begin());
        pQueue->erase(pQueue->begin());

        IntFrame frameBound = maxSpike.frame + jitterTol;
        IntChannel maxChannel = maxSpike.channel;
        IntVolt maxAmp = maxSpike.amplitude;

        { // filter outer neighbors
            vector<Spike> outerSpikes;

            copy_if(pQueue->begin(), pQueue->end(), back_inserter(outerSpikes),
                    [this, pQueue, frameBound, maxChannel](const Spike &spike)
                    { return spike.frame <= frameBound &&
                             pLayout->areOuterNeighbors(spike.channel, maxChannel) &&
                             shouldFilterOuter(pQueue, spike); });

            pQueue->remove_if([&outerSpikes](const Spike &spike)
                              { return binary_search( // outerSpikes is sorted by nature of queue
                                    outerSpikes.begin(), outerSpikes.end(), spike,
                                    [](const Spike &lhs, const Spike &rhs)
                                    { return lhs.frame < rhs.frame ||
                                             (lhs.frame == rhs.frame && lhs.channel < rhs.channel); }); });
        }

        pQueue->remove_if([this, frameBound, maxChannel, maxAmp](const Spike &spike)
                          { return spike.frame <= frameBound &&
                                   pLayout->areInnerNeighbors(spike.channel, maxChannel) &&
                                   spike.amplitude <= maxAmp; });
        pQueue->push_front(move(maxSpike));
    }

    bool SpikeDecayFilterer::shouldFilterOuter(SpikeQueue *pQueue, const Spike &outerSpike) const
    {
        IntChannel maxChannel = pQueue->begin()->channel;
        IntChannel outerChannel = outerSpike.channel;
        FloatGeom outerDist = pLayout->getChannelDistance(outerChannel, maxChannel);

        for (IntChannel innerOfOuter : pLayout->getInnerNeighbors(outerChannel))
        {
            if (pLayout->getChannelDistance(innerOfOuter, maxChannel) < outerDist)
            {
                SpikeQueue::const_iterator itSpikeOnInner = find_if(
                    pQueue->begin(), pQueue->end(),
                    [innerOfOuter](const Spike &spike)
                    { return spike.channel == innerOfOuter; });
                if (itSpikeOnInner == pQueue->end()) // spike on innerOfOuter channel not found
                {
                    continue;
                }

                bool isDecay = outerSpike.amplitude < itSpikeOnInner->amplitude * decayRatio && // outer decayed
                               itSpikeOnInner->frame - jitterTol <= outerSpike.frame;           // and outer time correct

                if (pLayout->areInnerNeighbors(innerOfOuter, maxChannel))
                {
                    return isDecay;
                }
                // else: outer neighbor that is closer
                if (isDecay)
                {
                    return shouldFilterOuter(pQueue, *itSpikeOnInner);
                }
            }
        }

        return false; // no corresponding spike from inner
    }

} // namespace HSDetection
