#include "SpikeWriter.h"
#include <cmath>
#include "Detection.h"

namespace HSDetection
{
    void SpikeWriter::operator()(Spike *pSpike)
    {
        int x = floor(pSpike->position.x * 1000 + .5); // NOTE: default pos w/o localize is 0
        int y = floor(pSpike->position.y * 1000 + .5);

        Detection::spikes_filtered_file.write((const char *)&pSpike->channel, sizeof(int));
        Detection::spikes_filtered_file.write((const char *)&pSpike->frame, sizeof(int));
        Detection::spikes_filtered_file.write((const char *)&pSpike->amplitude, sizeof(int));
        Detection::spikes_filtered_file.write((const char *)&x, sizeof(int));
        Detection::spikes_filtered_file.write((const char *)&y, sizeof(int));
        Detection::spikes_filtered_file.write((const char *)&pSpike->written_cutout[0], pSpike->written_cutout.size() * sizeof(int));
    }

} // namespace HSDetection
