#include "SpikeShapeWriter.h"

using namespace std;

namespace HSDetection
{
    SpikeShapeWriter::SpikeShapeWriter(const string &filename, TraceWrapper *pTrace,
                                       IntFrame cutoutStart, IntFrame cutoutLen)
        : spikeFile(filename, ios::binary | ios::trunc),
          buffer(new IntVolt[cutoutLen]), pTrace(pTrace),
          cutoutStart(cutoutStart), cutoutLen(cutoutLen) {}

    SpikeShapeWriter::~SpikeShapeWriter()
    {
        delete[] buffer;
        spikeFile.close();
    }

    void SpikeShapeWriter::operator()(Spike *pSpike)
    {
        IntFrame cutoutStart = pSpike->frame - this->cutoutStart;
        for (IntFrame t = 0; t < cutoutLen; t++)
        {
            buffer[t] = (*pTrace)(cutoutStart + t, pSpike->channel);
        }

        spikeFile.write((const char *)buffer, cutoutLen * sizeof(IntVolt));
    }

} // namespace HSDetection
