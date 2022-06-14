#include "SpikeQueue.h"
#include "SpikeHandler.h"
#include "Detection.h"
#include "ProcessSpikes.h"

namespace HSDetection
{
    void SpikeQueue::add(Spike spike)
    {
        // TODO: move to processing?
        spike = SpikeHandler::storeWaveformCutout(Detection::cutout_size, spike);
        if (HSDetection::Detection::to_localize)
        {
            spike = SpikeHandler::storeCOMWaveformsCounts(spike);
        }

        while (true)
        {
            if (queue.empty() ||
                spike.frame <= queue.front().frame + Detection::spike_peak_duration + Detection::noise_duration)
            {
                queue.push_back(spike);
                break;
            }

            if (Detection::to_localize)
            {
                ProcessSpikes::filterLocalizeSpikes();
            }
            else
            {
                ProcessSpikes::filterSpikes();
            }
        }
    }

    void SpikeQueue::close()
    {
        while (!queue.empty())
        {
            if (Detection::to_localize)
            {
                ProcessSpikes::filterLocalizeSpikes();
            }
            else
            {
                ProcessSpikes::filterSpikes();
            }
        }
    }

} // namespace HSDetection
