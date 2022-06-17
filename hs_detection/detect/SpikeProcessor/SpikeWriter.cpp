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
        spikeFile.write((const char *)&pSpike->written_cutout[0], pSpike->written_cutout.size() * sizeof(int));
    }

} // namespace HSDetection
