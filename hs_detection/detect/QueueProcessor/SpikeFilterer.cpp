#include "SpikeFilterer.h"

namespace HSDetection
{
    SpikeFilterer::SpikeFilterer(const ProbeLayout *pLayout, IntFrame jitterTol)
        : pLayout(pLayout), jitterTol(jitterTol) {}

    SpikeFilterer::~SpikeFilterer() {}

    void SpikeFilterer::operator()(SpikeQueue *pQueue)
    {
        IntFrame frameBound = pQueue->begin()->frame + jitterTol;
        IntChannel maxChannel = pQueue->begin()->channel;
        IntVolt maxAmp = pQueue->begin()->amplitude;

        pQueue->remove_if([this, frameBound, maxChannel, maxAmp](const Spike &spike)
                          { return spike.frame <= frameBound &&
                                   pLayout->areNeighbors(spike.channel, maxChannel) &&
                                   spike.amplitude < maxAmp; }); // TODO:??? <=, remember to add maxSpike back
    }

} // namespace HSDetection
