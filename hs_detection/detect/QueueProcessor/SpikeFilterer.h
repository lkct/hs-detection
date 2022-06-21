#ifndef SPIKEFILTERER_H
#define SPIKEFILTERER_H

#include "QueueProcessor.h"
#include "../ProbeLayout.h"

namespace HSDetection
{
    class SpikeFilterer : public QueueProcessor
    {
    private:
        ProbeLayout *pLayout;  // passed in, should not release here
        IntFrame framesFilter; // TODO: name???

    public:
        SpikeFilterer(ProbeLayout *pLayout, IntFrame noiseDuration);
        ~SpikeFilterer();

        void operator()(SpikeQueue *pQueue);
    };

} // namespace HSDetection

#endif
