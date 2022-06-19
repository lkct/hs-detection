#ifndef SPIKESAVER_H
#define SPIKESAVER_H

#include "SpikeProcessor.h"

namespace HSDetection
{
    class SpikerSaver : public SpikeProcessor
    {
    private:
        std::vector<char> *resultBuffer; // passed in, should not release here

    public:
        SpikerSaver(std::vector<char> *resultBuffer);
        ~SpikerSaver();

        using SpikeProcessor::operator(); // allow call on iterator
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
