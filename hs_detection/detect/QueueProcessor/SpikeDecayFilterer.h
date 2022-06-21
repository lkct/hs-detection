#ifndef SPIKEDECAYFILTERER_H
#define SPIKEDECAYFILTERER_H

#include "QueueProcessor.h"
#include "../ProbeLayout.h"

namespace HSDetection
{
    class SpikeDecayFilterer : public QueueProcessor
    {
    private:
        ProbeLayout *pLayout;   // passed in, should not release here
        IntFrame noiseDuration; // TODO: name???
        float noiseAmpPercent;

        bool shouldFilterOuter(SpikeQueue *pQueue, const Spike &outerSpike) const;

    public:
        SpikeDecayFilterer(ProbeLayout *pLayout, IntFrame noiseDuration, float noiseAmpPercent);
        ~SpikeDecayFilterer();

        void operator()(SpikeQueue *pQueue);
    };

} // namespace HSDetection

#endif
