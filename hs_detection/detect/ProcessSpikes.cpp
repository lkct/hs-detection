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

            HSDetection::SpikeWriter()(&max_spike);

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
