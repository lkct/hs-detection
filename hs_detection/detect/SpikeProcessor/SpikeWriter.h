#ifndef SPIKEWRITER_H
#define SPIKEWRITER_H

#include <string>
#include <fstream>

#include "SpikeProcessor.h"

namespace HSDetection
{
    class SpikeWriter : public SpikeProcessor
    {
    private:
        std::ofstream spikeFile;

    public:
        SpikeWriter(const std::string &filename);
        ~SpikeWriter();

        using SpikeProcessor::operator();
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
