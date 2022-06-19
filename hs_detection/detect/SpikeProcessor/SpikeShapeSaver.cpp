#include "SpikeShapeSaver.h"

using namespace std;

namespace HSDetection
{
    SpikeShapeSaver::SpikeShapeSaver(const string &filename, TraceWrapper *pTrace,
                                     int cutoutLeft, int cutoutLength)
        : spikeFile(filename, ios::binary | ios::trunc),
          buffer(new int[cutoutLength]), pTrace(pTrace),
          cutoutLeft(cutoutLeft), cutoutLength(cutoutLength) {}

    SpikeShapeSaver::~SpikeShapeSaver()
    {
        spikeFile.close();
        delete[] buffer;
    }

    void SpikeShapeSaver::operator()(Spike *pSpike)
    {
        int tStart = pSpike->frame - cutoutLeft;
        for (int i = 0; i < cutoutLength; i++)
        {
            buffer[i] = (*pTrace)(tStart + i, pSpike->channel);
        }

        spikeFile.write((const char *)buffer, cutoutLength * sizeof(int));
    }

} // namespace HSDetection
