#ifndef SPIKEQUEUE_H
#define SPIKEQUEUE_H

#include "Spike.h"
#include <list>

namespace HSDetection
{
    class SpikeQueue
    {
    private:
        // NOTE: list has constant erase, but forward_list not convenient enough
        std::list<Spike> queue; // TODO: list of Spike* ???

    public:
        typedef std::list<Spike>::iterator iterator;
        typedef std::list<Spike>::const_iterator const_iterator;

        SpikeQueue() : queue() {}
        ~SpikeQueue() {}

        void add(Spike spike);
        void close();

        iterator begin() { return queue.begin(); }
        const_iterator begin() const { return queue.begin(); }
        iterator end() { return queue.end(); }
        const_iterator end() const { return queue.end(); }
        bool empty() const { return queue.empty(); }
        iterator erase(const_iterator position) { return queue.erase(position); }
        Spike &front() { return queue.front(); }
        const Spike &front() const { return queue.front(); }
    };

} // namespace HSDetection

#endif
