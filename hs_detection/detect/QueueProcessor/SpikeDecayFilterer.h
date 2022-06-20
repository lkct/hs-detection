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
        int noiseDuration;    // TODO: name???
        float noiseAmpPercent;

    public:
        SpikeDecayFilterer(ProbeLayout *pLayout, int noiseDuration, float noiseAmpPercent);
        ~SpikeDecayFilterer();

        void operator()(SpikeQueue *pQueue);

        // TODO: const function?
        void filterOuterNeighbors(SpikeQueue *pQueue, Spike maxSpike);
        bool filteredOuterSpike(SpikeQueue *pQueue, Spike outerSpike, Spike maxSpike);
    };

} // namespace HSDetection

#endif
