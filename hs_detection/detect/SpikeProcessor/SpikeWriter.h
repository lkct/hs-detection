#ifndef SPIKEWRITER_H
#define SPIKEWRITER_H

#include <string>
#include <fstream>

#include "SpikeProcessor.h"
#include "../VoltTrace.h"

namespace HSDetection
{
    class SpikeWriter : public SpikeProcessor
    {
    private:
        std::ofstream spikeFile;
        VoltTrace *pTrace; // passed in, should not release here
        char *buffer;      // created and released here
        int cutoutLeft;
        int cutoutLength;

    public:
        SpikeWriter(const std::string &filename, VoltTrace *pTrace,
                    int cutout_start, int cutout_end);
        ~SpikeWriter();

        using SpikeProcessor::operator(); // allow call on iterator
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
