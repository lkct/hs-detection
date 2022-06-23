#include <algorithm>

#include "MaxSpikeFinder.h"

using namespace std;

namespace HSDetection
{
    MaxSpikeFinder::MaxSpikeFinder(const ProbeLayout *pLayout, IntFrame jitterTol)
        : pLayout(pLayout), jitterTol(jitterTol) {}

    MaxSpikeFinder::~MaxSpikeFinder() {}

    void MaxSpikeFinder::operator()(SpikeQueue *pQueue)
    {
        IntFrame frameBound = pQueue->begin()->frame + jitterTol;
        IntChannel centerChannel = pQueue->begin()->channel;

        SpikeQueue::iterator itMax = max_element(
            pQueue->begin(), pQueue->end(),
            [this, frameBound, centerChannel](const Spike &lhs, const Spike &rhs)
            { return rhs.frame <= frameBound &&
                     pLayout->areNeighbors(rhs.channel, centerChannel) &&
                     lhs.amplitude <= rhs.amplitude; }); // not sure whether <= or <

        pQueue->push_front(move(*itMax));
        pQueue->erase(itMax);
    }

} // namespace HSDetection
