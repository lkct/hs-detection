#ifndef SPIKEDECAYFILTERER_H
#define SPIKEDECAYFILTERER_H

#include "QueueProcessor.h"
#include "../ProbeLayout.h"

namespace HSDetection
{
    class SpikeDecayFilterer : public QueueProcessor
    {
    private:
        const ProbeLayout *pLayout; // passed in, should not release here

        IntFrame jitterTol;
        FloatRatio decayRatio;

        bool shouldFilterOuter(SpikeQueue *pQueue, const Spike &outerSpike) const;

    public:
        SpikeDecayFilterer(const ProbeLayout *pLayout, IntFrame jitterTol, FloatRatio decayRatio);
        ~SpikeDecayFilterer();

        void operator()(SpikeQueue *pQueue);
    };

} // namespace HSDetection

#endif
