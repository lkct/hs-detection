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
        int last_frame = HSDetection::Detection::queue.front().frame;

        while (!HSDetection::Detection::queue.empty() && HSDetection::Detection::queue.front().frame <= last_frame + HSDetection::Detection::noise_duration)
        {
            last_frame = HSDetection::Detection::queue.front().frame;

            if (HSDetection::Detection::decay_filtering == true)
            {
                // Spike max_spike = FilterSpikes::filterSpikesDecay(HSDetection::Detection::queue.front());
                // TODO: undefined
            }
            else
            {
                HSDetection::SpikeFilterer()(&HSDetection::Detection::queue);
            }

            if (HSDetection::Detection::to_localize)
            {
                HSDetection::SpikeLocalizer()(&HSDetection::Detection::queue.front());
            }

            HSDetection::SpikeWriter()(&HSDetection::Detection::queue.front());

            HSDetection::Detection::queue.erase(HSDetection::Detection::queue.begin());
        }
    }
}
