#ifndef SPIKEDECAYFILTERER_H
#define SPIKEDECAYFILTERER_H

#include "QueueProcessor.h"
#include "../ProbeLayout.h"

namespace HSDetection
{
    class SpikeDecayFilterer : public QueueProcessor
    {
    private:
        ProbeLayout *pLayout; // passed in, should not release here
        IntFrame jitterTol;   // TODO: name???
        float decayRatio;

        bool shouldFilterOuter(SpikeQueue *pQueue, const Spike &outerSpike) const;

    public:
        SpikeDecayFilterer(ProbeLayout *pLayout, IntFrame jitterTol, float decayRatio);
        ~SpikeDecayFilterer();

        void operator()(SpikeQueue *pQueue);
    };

} // namespace HSDetection

#endif
