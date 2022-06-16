#ifndef SPIKEFILTERER_H
#define SPIKEFILTERER_H

#include "QueueProcessor.h"

namespace HSDetection
{
    class SpikeFilterer : public QueueProcessor
    {
    private:
        int framesFilter; // TODO: ???

    public:
        SpikeFilterer(int noiseDuration);
        ~SpikeFilterer();

        void operator()(SpikeQueue *pQueue);
    };

} // namespace HSDetection

#endif
