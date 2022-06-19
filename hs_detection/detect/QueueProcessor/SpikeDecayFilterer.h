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
        int noiseDuration;
        int noiseAmpPercent;

    public:
        SpikeDecayFilterer(ProbeLayout *pLayout, int noiseDuration, int noiseAmpPercent);
        ~SpikeDecayFilterer();

        void operator()(SpikeQueue *pQueue);

        void filterOuterNeighbors(SpikeQueue *pQueue, Spike max_spike);
        bool filteredOuterSpike(SpikeQueue *pQueue, Spike outer_spike, Spike max_spike);
        void filterInnerNeighbors(SpikeQueue *pQueue, Spike max_spike);
    };

} // namespace HSDetection

#endif
