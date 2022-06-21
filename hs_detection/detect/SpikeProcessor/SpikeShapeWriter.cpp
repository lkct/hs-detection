#include "SpikeShapeWriter.h"

using namespace std;

namespace HSDetection
{
    SpikeShapeWriter::SpikeShapeWriter(const string &filename, TraceWrapper *pTrace,
                                       IntFrame cutoutLeft, IntFrame cutoutLen)
        : spikeFile(filename, ios::binary | ios::trunc),
          buffer(new IntVolt[cutoutLen]), pTrace(pTrace),
          cutoutLeft(cutoutLeft), cutoutLen(cutoutLen) {}

    SpikeShapeWriter::~SpikeShapeWriter()
    {
        delete[] buffer;
        spikeFile.close();
    }

    void SpikeShapeWriter::operator()(Spike *pSpike)
    {
        IntFrame cutoutStart = pSpike->frame - cutoutLeft;
        for (IntFrame t = 0; t < cutoutLen; t++)
        {
            buffer[t] = (*pTrace)(cutoutStart + t, pSpike->channel);
        }

        spikeFile.write((const char *)buffer, cutoutLen * sizeof(IntVolt));
    }

} // namespace HSDetection
