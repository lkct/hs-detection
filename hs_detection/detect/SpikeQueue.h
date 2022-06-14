#ifndef SPIKEQUEUE_H
#define SPIKEQUEUE_H

#include "Spike.h"
#include <list>

namespace HSDetection
{
    class SpikeQueue
    {
    private:
    public:
        // NOTE: list has constant erase, but forward_list not convenient enough
        std::list<Spike> queue; // TODO: list of Spike* ???

        typedef std::list<Spike>::iterator iterator;

        SpikeQueue();
        ~SpikeQueue();

        void add(Spike spike_to_be_added);
        void close();
    };

} // namespace HSDetection

#endif
