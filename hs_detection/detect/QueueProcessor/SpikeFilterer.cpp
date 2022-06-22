#include "SpikeFilterer.h"

namespace HSDetection
{
    SpikeFilterer::SpikeFilterer(ProbeLayout *pLayout, IntFrame jitterTol)
        : pLayout(pLayout), jitterTol(jitterTol) {}

    SpikeFilterer::~SpikeFilterer() {}

    void SpikeFilterer::operator()(SpikeQueue *pQueue)
    {
#ifndef FRAMEBOUND
        jitterTol = 1000000; // TODO: ???
#endif
        IntFrame frameBound = pQueue->begin()->frame + jitterTol;
        IntChannel maxChannel = pQueue->begin()->channel;
        IntVolt maxAmp = pQueue->begin()->amplitude;

        pQueue->remove_if([this, frameBound, maxChannel, maxAmp](const Spike &spike)
                          { return spike.frame <= frameBound &&
                                   pLayout->areNeighbors(spike.channel, maxChannel) &&
                                   spike.amplitude < maxAmp; });
    }

} // namespace HSDetection
