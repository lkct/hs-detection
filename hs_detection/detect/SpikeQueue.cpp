#include "SpikeQueue.h"
#include "Utils.h"
#include "Detection.h"
#include "SpikeFilterer.h"
#include "SpikeLocalizer.h"
#include "SpikeWriter.h"

namespace HSDetection
{
    void SpikeQueue::add(Spike spike)
    {
        // TODO: move to processing?
        // NOTE: currently cannot, rely on trace, break at chunk update
        spike = Utils::storeWaveformCutout(Detection::cutout_size, spike);
        if (Detection::to_localize)
        {
            spike = Utils::storeCOMWaveformsCounts(spike);
        }

        while (true)
        {
            if (queue.empty() ||
                spike.frame <= queue.front().frame + Detection::spike_peak_duration + Detection::noise_duration)
            {
                queue.push_back(spike);
                break;
            }

            process();
        }
    }

    void SpikeQueue::close()
    {
        while (!queue.empty())
        {
            process();
        }
    }

    void SpikeQueue::process()
    {
        int last_frame = queue.front().frame;

        while (!queue.empty() && queue.front().frame <= last_frame + Detection::noise_duration)
        {
            last_frame = queue.front().frame;

            if (Detection::decay_filtering == true)
            {
                // Spike max_spike = FilterSpikes::filterSpikesDecay(queue.front());
                // TODO: undefined
            }
            else
            {
                SpikeFilterer()(this);
            }

            if (Detection::to_localize)
            {
                SpikeLocalizer()(&queue.front());
            }

            SpikeWriter()(&queue.front());

            queue.erase(queue.begin());
        }
    }

} // namespace HSDetection
