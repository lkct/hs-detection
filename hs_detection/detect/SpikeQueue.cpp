#include "SpikeQueue.h"
#include "SpikeHandler.h"
#include "Detection.h"
#include "ProcessSpikes.h"

namespace HSDetection
{
    void SpikeQueue::add(Spike spike)
    {
        // TODO: move to processing?
        // NOTE: currently cannot, rely on trace, break at chunk update
        spike = SpikeHandler::storeWaveformCutout(Detection::cutout_size, spike);
        if (Detection::to_localize)
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

            ProcessSpikes::filterLocalizeSpikes();
        }
    }

    void SpikeQueue::close()
    {
        while (!queue.empty())
        {
            ProcessSpikes::filterLocalizeSpikes();
        }
    }

} // namespace HSDetection
