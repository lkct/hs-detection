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

        const TraceWrapper *pTrace; // passed in, should not release here

        IntFrame cutoutStart;
        IntFrame cutoutLen;

    public:
        SpikeShapeWriter(const std::string &filename, const TraceWrapper *pTrace,
                         IntFrame cutoutStart, IntFrame cutoutLen);
        ~SpikeShapeWriter();

        using SpikeProcessor::operator(); // allow call on iterator
        void operator()(Spike *pSpike);
    };

} // namespace HSDetection

#endif
