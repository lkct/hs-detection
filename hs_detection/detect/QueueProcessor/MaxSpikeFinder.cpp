#include <algorithm>

#include "MaxSpikeFinder.h"

using namespace std;

namespace HSDetection
{
    MaxSpikeFinder::MaxSpikeFinder(ProbeLayout *pLayout, IntFrame noiseDuration)
        : pLayout(pLayout), framesFilter(noiseDuration + 1) {}

    MaxSpikeFinder::~MaxSpikeFinder() {}

    void MaxSpikeFinder::operator()(SpikeQueue *pQueue)
    {
        IntFrame frameBound = pQueue->begin()->frame + framesFilter;
        IntChannel centerChannel = pQueue->begin()->channel;

        SpikeQueue::iterator itMax = max_element(
            pQueue->begin(), pQueue->end(),
            [this, frameBound, centerChannel](const Spike &lhs, const Spike &rhs)
            { return rhs.frame < frameBound &&
                     pLayout->areNeighbors(rhs.channel, centerChannel) &&
                     lhs.amplitude <= rhs.amplitude; }); // TODO: <=?

        pQueue->push_front(move(*itMax));
        pQueue->erase(itMax);
    }

} // namespace HSDetection
