#ifndef MAXSPIKEFINDER_H
#define MAXSPIKEFINDER_H

#include "QueueProcessor.h"
#include "../ProbeLayout.h"

namespace HSDetection
{
    class MaxSpikeFinder : public QueueProcessor
    {
    private:
        const ProbeLayout *pLayout; // passed in, should not release here

        IntFrame jitterTol;

    public:
        MaxSpikeFinder(const ProbeLayout *pLayout, IntFrame jitterTol);
        ~MaxSpikeFinder();

        void operator()(SpikeQueue *pQueue);
    };

} // namespace HSDetection

#endif
