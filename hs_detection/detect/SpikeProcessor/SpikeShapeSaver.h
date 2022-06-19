#ifndef SPIKESHAPESAVER_H
#define SPIKESHAPESAVER_H

#include <string>
#include <fstream>

#include "SpikeProcessor.h"
#include "../TraceWrapper.h"

namespace HSDetection
{
    class SpikeShapeSaver : public SpikeProcessor
    {
    private:
        std::ofstream spikeFile;
        int *buffer; // created and released here

        TraceWrapper *pTrace; // passed in, should not release here
        int cutoutLeft;       // TODO: name???
        int cutoutLength;

    public:
        SpikeShapeSaver(const std::string &filename, TraceWrapper *pTrace,
                        int cutoutLeft, int cutoutLength);
        ~SpikeShapeSaver();

        using SpikeProcessor::operator(); // allow call on iterator
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
