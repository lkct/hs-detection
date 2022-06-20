#include "SpikeShapeSaver.h"

using namespace std;

namespace HSDetection
{
    SpikeShapeSaver::SpikeShapeSaver(const string &filename, TraceWrapper *pTrace,
                                     int cutoutLeft, int cutoutLen)
        : spikeFile(filename, ios::binary | ios::trunc),
          buffer(new int[cutoutLen]), pTrace(pTrace),
          cutoutLeft(cutoutLeft), cutoutLen(cutoutLen) {}

    SpikeShapeSaver::~SpikeShapeSaver()
    {
        spikeFile.close();
        delete[] buffer;
    }

    void SpikeShapeSaver::operator()(Spike *pSpike)
    {
        int tStart = pSpike->frame - cutoutLeft;
        for (int i = 0; i < cutoutLen; i++)
        {
            buffer[i] = (*pTrace)(tStart + i, pSpike->channel);
        }

        spikeFile.write((const char *)buffer, cutoutLen * sizeof(int));
    }

} // namespace HSDetection
