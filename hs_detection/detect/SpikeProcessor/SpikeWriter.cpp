#include <cmath>

#include "SpikeWriter.h"

using namespace std;

namespace HSDetection
{
    SpikeWriter::SpikeWriter(const string &filename)
        : spikes_filtered_file(filename, ios::binary | ios::trunc)
    {
    }

    SpikeWriter::~SpikeWriter()
    {
        spikes_filtered_file.close();
    }

    void SpikeWriter::operator()(Spike *pSpike)
    {
        int x = floor(pSpike->position.x * 1000 + .5); // NOTE: default pos w/o localize is 0
        int y = floor(pSpike->position.y * 1000 + .5);

        spikes_filtered_file.write((const char *)&pSpike->channel, sizeof(int));
        spikes_filtered_file.write((const char *)&pSpike->frame, sizeof(int));
        spikes_filtered_file.write((const char *)&pSpike->amplitude, sizeof(int));
        spikes_filtered_file.write((const char *)&x, sizeof(int));
        spikes_filtered_file.write((const char *)&y, sizeof(int));
        spikes_filtered_file.write((const char *)&pSpike->written_cutout[0], pSpike->written_cutout.size() * sizeof(int));
    }

} // namespace HSDetection
