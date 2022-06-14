#include "SpikeQueue.h"
#include "SpikeHandler.h"
#include "Detection.h"
#include "ProcessSpikes.h"

namespace HSDetection
{
    SpikeQueue::SpikeQueue() : queue() {}

    SpikeQueue::~SpikeQueue() {}

    void SpikeQueue::add(Spike spike_to_be_added)
    {
        // TODO: move to processing?
        spike_to_be_added = SpikeHandler::storeWaveformCutout(Detection::cutout_size, spike_to_be_added);
        if (HSDetection::Detection::to_localize)
        {
            spike_to_be_added = SpikeHandler::storeCOMWaveformsCounts(spike_to_be_added);
        }

        while (true)
        {
            Spike firstSpike(0, 0, 0); // TODO: ???
            if (queue.empty() ||
                spike_to_be_added.frame <= (firstSpike = queue.front()).frame +
                                               Detection::spike_peak_duration + Detection::noise_duration)
            {
                queue.push_back(spike_to_be_added);
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
