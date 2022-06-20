#ifndef SPIKEQUEUE_H
#define SPIKEQUEUE_H

#include <list>
#include <vector>
#include <utility>

#include "Spike.h"

namespace HSDetection
{
    class Detection;
    class QueueProcessor;
    class SpikeProcessor;
    // no include in header to avoid cyclic dependency

    class SpikeQueue
    {
    private:
        std::list<Spike> queue; // list has constant-time erase and also bi-directional iter

        std::vector<QueueProcessor *> queProcs; // content created and released here
        std::vector<SpikeProcessor *> spkProcs; // content created and released here

        std::vector<Spike> *result; // passed in, should not release here

        int framesInQueue;    // TODO: name??? number of frames from where the spikes should be kept in queue
        int framesToContinue; // TODO: name???

    public:
        SpikeQueue(Detection *pDet); // passing thw whole param set altogether
        ~SpikeQueue();

        // copy constructor deleted to protect container content
        SpikeQueue(const SpikeQueue &) = delete;
        // copy assignment deleted to protect container content
        SpikeQueue &operator=(const SpikeQueue &) = delete;

        void process(int frameBound);
        void finalize();

        // wrappers of container interface

        typedef std::list<Spike>::iterator iterator;
        typedef std::list<Spike>::const_iterator const_iterator;

        bool empty() const { return queue.empty(); }

        const_iterator begin() const { return queue.begin(); }
        iterator begin() { return queue.begin(); }

        const_iterator end() const { return queue.end(); }
        iterator end() { return queue.end(); }

        void push_front(Spike &&spike) { queue.push_front(std::move(spike)); }

        void push_back(Spike &&spike) { process(spike.frame - framesInQueue), queue.push_back(std::move(spike)); }

        iterator erase(const_iterator position) { return queue.erase(position); }

        template <class UnaryPredicate>
        void remove_if(UnaryPredicate predicate) { queue.remove_if(predicate); }
        // no need for return value (c++20)
    };

} // namespace HSDetection

#endif
