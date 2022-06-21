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
        IntVolt *buffer; // created and released here

        TraceWrapper *pTrace; // passed in, should not release here
        IntFrame cutoutLeft;  // TODO: name???
        IntFrame cutoutLen;

    public:
        SpikeShapeWriter(const std::string &filename, TraceWrapper *pTrace,
                         IntFrame cutoutLeft, IntFrame cutoutLen);
        ~SpikeShapeWriter();

        using SpikeProcessor::operator(); // allow call on iterator
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
