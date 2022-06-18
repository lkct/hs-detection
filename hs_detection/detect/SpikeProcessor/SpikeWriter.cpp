#include <cmath>

#include "SpikeWriter.h"

using namespace std;

namespace HSDetection
{
    SpikeWriter::SpikeWriter(const string &filename, VoltTrace *pTrace,
                             int cutout_start, int cutout_end)
        : spikeFile(filename, ios::binary | ios::trunc), pTrace(pTrace),
          cutoutLeft(cutout_start), cutoutLength(cutout_start + cutout_end + 1) // TODO: +1?
    {
        buffer = new char[cutoutLength * sizeof(int)];
    }

    SpikeWriter::~SpikeWriter()
    {
        spikeFile.close();
        delete[] buffer;
    }

    void SpikeWriter::operator()(Spike *pSpike)
    {
        int tStart = pSpike->frame - cutoutLeft;
        for (int i = 0; i < cutoutLength; i++)
        {
            ((int *)buffer)[i] = (*pTrace)(tStart + i, pSpike->channel);
        }
        spikeFile.write(buffer, cutoutLength * sizeof(int));
    }

} // namespace HSDetection
