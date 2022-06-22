#ifndef SPIKEFILTERER_H
#define SPIKEFILTERER_H

#include "QueueProcessor.h"
#include "../ProbeLayout.h"

namespace HSDetection
{
    class SpikeFilterer : public QueueProcessor
    {
    private:
        ProbeLayout *pLayout; // passed in, should not release here

        IntFrame jitterTol;

    public:
        SpikeFilterer(ProbeLayout *pLayout, IntFrame jitterTol);
        ~SpikeFilterer();

        void operator()(SpikeQueue *pQueue);
    };

} // namespace HSDetection

#endif
