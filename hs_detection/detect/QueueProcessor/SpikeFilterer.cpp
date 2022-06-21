#include <algorithm>

#include "SpikeFilterer.h"

using namespace std;

namespace HSDetection
{
    SpikeFilterer::SpikeFilterer(ProbeLayout *pLayout, IntFrame noiseDuration)
        : pLayout(pLayout), framesFilter(noiseDuration + 1) {}

    SpikeFilterer::~SpikeFilterer() {}

    void SpikeFilterer::operator()(SpikeQueue *pQueue)
    {
        IntFrame frameBound = pQueue->begin()->frame + framesFilter;
        IntChannel centerChannel = pQueue->begin()->channel;

        SpikeQueue::iterator itMax = max_element(
            pQueue->begin(), pQueue->end(),
            [this, frameBound, centerChannel](const Spike &lhs, const Spike &rhs)
            { return rhs.frame < frameBound &&
                     pLayout->areNeighbors(rhs.channel, centerChannel) &&
                     lhs.amplitude <= rhs.amplitude; }); // TODO: <=?

        IntChannel maxChannel = itMax->channel;
        IntVolt maxAmp = itMax->amplitude;

        pQueue->push_front(move(*itMax));
        pQueue->erase(itMax);

        pQueue->remove_if([this, maxChannel, maxAmp](const Spike &spike)
                          { return pLayout->areNeighbors(spike.channel, maxChannel) &&
                                   spike.amplitude < maxAmp; });
    }

} // namespace HSDetection
