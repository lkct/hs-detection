#include <cmath>

#include "SpikeWriter.h"

using namespace std;

namespace HSDetection
{
    SpikeWriter::SpikeWriter(const string &filename)
        : spikeFile(filename, ios::binary | ios::trunc) {}

    SpikeWriter::~SpikeWriter()
    {
        spikeFile.close();
    }

    void SpikeWriter::operator()(Spike *pSpike)
    {
        int x = floor(pSpike->position.x * 1000 + .5); // NOTE: default pos w/o localize is 0
        int y = floor(pSpike->position.y * 1000 + .5);

        spikeFile.write((const char *)&pSpike->channel, sizeof(int));
        spikeFile.write((const char *)&pSpike->frame, sizeof(int));
        spikeFile.write((const char *)&pSpike->amplitude, sizeof(int));
        spikeFile.write((const char *)&x, sizeof(int));
        spikeFile.write((const char *)&y, sizeof(int));
        spikeFile.write((const char *)&pSpike->written_cutout[0], pSpike->written_cutout.size() * sizeof(int));
    }

} // namespace HSDetection
