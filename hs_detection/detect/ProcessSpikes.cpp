#include "ProcessSpikes.h"
#include "Spike.h"
#include "FilterSpikes.h"
#include "SpikeLocalizer.h"
#include <iostream>
#include <cmath>
#include "Detection.h"
#include "SpikeFilterer.h"
#include "SpikeWriter.h"

using namespace std;

namespace ProcessSpikes
{

    void filterLocalizeSpikes()
    {
        Spike max_spike(0, 0, 0);
        int last_frame = HSDetection::Detection::queue.front().frame;

        while (!HSDetection::Detection::queue.empty() && HSDetection::Detection::queue.front().frame <= last_frame + HSDetection::Detection::noise_duration)
        {
            last_frame = HSDetection::Detection::queue.front().frame;

            if (HSDetection::Detection::decay_filtering == true)
            {
                max_spike = FilterSpikes::filterSpikesDecay(HSDetection::Detection::queue.front());
            }
            else
            {
                max_spike = HSDetection::SpikeFilterer()(&HSDetection::Detection::queue, HSDetection::Detection::queue.begin());
            }

            if (HSDetection::Detection::to_localize)
            {
                HSDetection::SpikeLocalizer()(&max_spike);
            }

            HSDetection::SpikeWriter()(&max_spike);
        }
    }
}
