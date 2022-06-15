#ifndef SPIKEQUEUE_H
#define SPIKEQUEUE_H

#include <list>
#include <vector>

#include "Spike.h"

namespace HSDetection
{
    // cannot include in cycle
    class QueueProcessor;
    class SpikeProcessor;
    class Detection;

    class SpikeQueue
    {
    private:
        // TODO: list of Spike* ???
        std::list<Spike> queue; // list has constant erase, but forward_list not convenient enough
        std::vector<QueueProcessor *> queProcs;
        std::vector<SpikeProcessor *> spkProcs;

    public:
        SpikeQueue(Detection *pDet); // passing everything altogether
        ~SpikeQueue();

        void add(Spike &spike);
        void close();
        void process();

        typedef std::list<Spike>::iterator iterator;
        typedef std::list<Spike>::const_iterator const_iterator;

        iterator begin() { return queue.begin(); }
        const_iterator begin() const { return queue.begin(); }
        iterator end() { return queue.end(); }
        const_iterator end() const { return queue.end(); }
        bool empty() const { return queue.empty(); }
        iterator erase(const_iterator position) { return queue.erase(position); }
        Spike &front() { return queue.front(); }
        const Spike &front() const { return queue.front(); }
        void push_front(const Spike &spike) { queue.push_front(spike); }
        void push_front(Spike &&spike) { queue.push_front(spike); }
    };

} // namespace HSDetection

#endif
