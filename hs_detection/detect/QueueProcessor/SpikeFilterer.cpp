#include "SpikeFilterer.h"

namespace HSDetection
{
    SpikeFilterer::SpikeFilterer(ProbeLayout *pLayout) : pLayout(pLayout) {}

    SpikeFilterer::~SpikeFilterer() {}

    void SpikeFilterer::operator()(SpikeQueue *pQueue)
    {
        IntChannel maxChannel = pQueue->begin()->channel;
        IntVolt maxAmp = pQueue->begin()->amplitude;

        pQueue->remove_if([this, maxChannel, maxAmp](const Spike &spike)
                          { return pLayout->areNeighbors(spike.channel, maxChannel) &&
                                   spike.amplitude < maxAmp; });
    }

} // namespace HSDetection
