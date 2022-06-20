#ifndef SPIKESHAPEWRITER_H
#define SPIKESHAPEWRITER_H

#include <string>
#include <fstream>

#include "SpikeProcessor.h"
#include "../TraceWrapper.h"

namespace HSDetection
{
    class SpikeShapeWriter : public SpikeProcessor
    {
    private:
        std::ofstream spikeFile;
        int *buffer; // created and released here

        TraceWrapper *pTrace; // passed in, should not release here
        int cutoutLeft;       // TODO: name???
        int cutoutLen;

    public:
        SpikeShapeWriter(const std::string &filename, TraceWrapper *pTrace,
                         int cutoutLeft, int cutoutLen);
        ~SpikeShapeWriter();

        using SpikeProcessor::operator(); // allow call on iterator
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
