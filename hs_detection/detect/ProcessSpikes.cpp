#include "ProcessSpikes.h"
#include "Spike.h"
#include "FilterSpikes.h"
#include "SpikeLocalizer.h"
#include <iostream>
#include <cmath>
#include "Detection.h"
#include "SpikeFilterer.h"

using namespace std;

namespace ProcessSpikes
{

    void filterLocalizeSpikes()
    {
        Spike first_spike = HSDetection::Detection::queue.front();
        Spike max_spike(0, 0, 0);
        bool isProcessed = false;

        while (!isProcessed)
        {

            if (HSDetection::Detection::decay_filtering == true)
            {
                max_spike = FilterSpikes::filterSpikesDecay(first_spike);
            }
            else
            {
                max_spike = HSDetection::SpikeFilterer()(&HSDetection::Detection::queue, HSDetection::Detection::queue.begin());
            }

            if (HSDetection::Detection::to_localize)
            {
                HSDetection::SpikeLocalizer()(&max_spike);
            }

            int32_t msc = (int32_t)max_spike.channel;
            int32_t msf = (int32_t)max_spike.frame;
            int32_t msa = (int32_t)max_spike.amplitude;
            int32_t X = (int32_t)floor(max_spike.position.x * 1000 + .5);  // NOTE: default pos w/o localize is 0
            int32_t Y = (int32_t)floor(max_spike.position.y * 1000 + .5);

            HSDetection::Detection::spikes_filtered_file.write((char *)&msc, sizeof(msc));
            HSDetection::Detection::spikes_filtered_file.write((char *)&msf, sizeof(msf));
            HSDetection::Detection::spikes_filtered_file.write((char *)&msa, sizeof(msa));
            HSDetection::Detection::spikes_filtered_file.write((char *)&X, sizeof(X));
            HSDetection::Detection::spikes_filtered_file.write((char *)&Y, sizeof(Y));
            HSDetection::Detection::spikes_filtered_file.write((char *)&max_spike.written_cutout[0], max_spike.written_cutout.size() * sizeof(int32_t));

            if (HSDetection::Detection::queue.empty())
            {
                isProcessed = true;
            }
            else
            {
                max_spike = HSDetection::Detection::queue.front();
                if (max_spike.frame > first_spike.frame + HSDetection::Detection::noise_duration)
                {
                    isProcessed = true;
                }
                else
                {
                    first_spike = max_spike;
                }
            }
        }
    }
}
