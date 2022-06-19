#include "SpikeDecayFilterer.h"

using namespace std;

namespace HSDetection
{
    SpikeDecayFilterer::SpikeDecayFilterer(ProbeLayout *pLayout, int noiseDuration, float noiseAmpPercent)
        : pLayout(pLayout), noiseDuration(noiseDuration), noiseAmpPercent(noiseAmpPercent) {}

    SpikeDecayFilterer::~SpikeDecayFilterer() {}

    void SpikeDecayFilterer::operator()(SpikeQueue *pQueue)
    {
        int curr_channel, curr_amp, curr_frame;
        Spike curr_spike(0, 0, 0);
        Spike max_spike(0, 0, 0);
        int max_spike_amp;
        max_spike = *(pQueue->begin());
        max_spike_amp = max_spike.amplitude;
        int spikeChannel = max_spike.channel;
        int frameBound = max_spike.frame + noiseDuration + 1;
        // Find the max amplitude neighbor of the first spike
        SpikeQueue::iterator it;
        SpikeQueue::iterator max_it;
        max_it = it = pQueue->begin();

        // Find max
        while (it != pQueue->end())
        {
            curr_spike = *it;
            curr_channel = it->channel;
            curr_amp = it->amplitude;
            curr_frame = it->frame;
            if (pLayout->areNeighbors(spikeChannel, curr_channel))
            {
                if (curr_amp >= max_spike_amp)
                {
                    if (curr_frame < frameBound)
                    {
                        max_spike = curr_spike;
                        max_spike_amp = curr_amp;
                        max_it = it;
                    }
                }
            }
            ++it;
        }
        pQueue->erase(max_it);

        if (!pQueue->empty())
        {
            filterOuterNeighbors(pQueue, max_spike);
            filterInnerNeighbors(pQueue, max_spike);
        }

        pQueue->push_front(move(max_spike));
    }

    void SpikeDecayFilterer::filterOuterNeighbors(SpikeQueue *pQueue, Spike max_spike)
    {
        Spike curr_spike(0, 0, 0);
        int curr_channel, curr_frame;
        vector<Spike> outer_spikes_to_be_filtered;
        SpikeQueue::iterator it;
        it = pQueue->begin();
        int frameBound = max_spike.frame + noiseDuration + 1;

        while (it != pQueue->end())
        {
            curr_spike = *it;
            curr_channel = curr_spike.channel;
            curr_frame = curr_spike.frame;
            if (curr_frame < frameBound)
            {
                if (pLayout->areNeighbors(max_spike.channel, curr_channel))
                {
                    if (!pLayout->areInnerNeighbors(max_spike.channel, curr_channel))
                    {
                        if (filteredOuterSpike(pQueue, curr_spike, max_spike))
                        {
                            outer_spikes_to_be_filtered.push_back(curr_spike);
                            ++it;
                        }
                        else
                        {
                            // Not a decaying outer spike (probably a new spike), filter later
                            ++it;
                        }
                    }
                    else
                    {
                        // Inner neighbor - all inner neighbors that need filtering will be filtered later
                        ++it;
                    }
                }
                else
                {
                    // Not a neighbor, filter later
                    ++it;
                }
            }
            else
            {
                // Too far away,filter later
                ++it;
            }
        }

        // Filter all spikes that need to be filtered all together at once
        Spike curr_spike_to_be_filtered(0, 0, 0);
        int curr_channel_to_be_filtered;
        int curr_frame_to_be_filtered;
        vector<Spike>::iterator it2;
        it2 = outer_spikes_to_be_filtered.begin();
        while (it2 != outer_spikes_to_be_filtered.end())
        {
            curr_spike_to_be_filtered = *it2;
            curr_channel_to_be_filtered = curr_spike_to_be_filtered.channel;
            curr_frame_to_be_filtered = curr_spike_to_be_filtered.frame;

            it = pQueue->begin();
            while (it != pQueue->end())
            {
                curr_spike = *it;
                curr_channel = curr_spike.channel;
                curr_frame = curr_spike.frame;
                if (curr_channel == curr_channel_to_be_filtered && curr_frame == curr_frame_to_be_filtered)
                {
                    it = pQueue->erase(it);
                    break;
                }
                else
                {
                    ++it;
                }
            }
            ++it2;
        }
    }

    Spike getSpikeFromChannel(SpikeQueue *pQueue, int channel)
    {
        SpikeQueue::iterator it;
        it = pQueue->begin();
        // Find max
        while (it != pQueue->end())
        {
            if (channel == it->channel)
            {
                return *it;
            }
            ++it;
        }
        Spike no_spike(0, 0, 0);
        no_spike.channel = -1;
        no_spike.frame = -1;
        no_spike.amplitude = -1;
        return no_spike;
    }
    bool SpikeDecayFilterer::filteredOuterSpike(SpikeQueue *pQueue, Spike outer_spike, Spike max_spike)
    {
        // True if should be filtered, false by default
        bool filtered_spike = false;
        // bool IS_INNER_EVENT = true;
        bool shares_inner = false;
        int NOT_VALID_FRAME = -1;

        for (int curr_inner_neighbor : pLayout->getInnerNeighbors(outer_spike.channel))
        {
            if (pLayout->areInnerNeighbors(max_spike.channel, curr_inner_neighbor))
            {
                SpikeQueue::iterator it;
                it = pQueue->begin();
                // Find max
                while (it != pQueue->end())
                {
                    if (curr_inner_neighbor == it->channel)
                    {
                        if (outer_spike.amplitude < it->amplitude * noiseAmpPercent)
                        {
                            if (outer_spike.frame < it->frame - noiseDuration)
                            {
                                // outer spike occurs too far before inner spike, probably new spike
                                shares_inner = true;
                                break;
                            }
                            else
                            {
                                // Shares an inner neighbor, spikes at a reasonable time, amplitude has decayed enough over this distance, filter
                                filtered_spike = true;
                                shares_inner = true;
                                break;
                            }
                        }
                        else
                        {
                            // amplitude too big to be a duplicate spike
                            shares_inner = true;
                            break;
                        }
                    }
                    ++it;
                }
                if (shares_inner == true)
                {
                    return filtered_spike;
                }
            }
        }
        // for(int i = 0; i < Parameters::max_neighbors; i++) {
        float curr_dist;
        float outer_dist_from_center = pLayout->getChannelDistance(outer_spike.channel, max_spike.channel);
        for (int curr_inner_channel : pLayout->getInnerNeighbors(outer_spike.channel))
        {
            curr_dist = pLayout->getChannelDistance(curr_inner_channel, max_spike.channel);
            if (curr_dist < outer_dist_from_center)
            {
                Spike inner_spike = getSpikeFromChannel(pQueue, curr_inner_channel);
                if (inner_spike.frame == NOT_VALID_FRAME)
                {
                    // not a spike, keep searching
                }
                else
                {
                    if (outer_spike.amplitude >= inner_spike.amplitude * noiseAmpPercent)
                    {
                        // keep searching
                    }
                    else
                    {
                        // Find the closest neighbor in the spike vector and recurse with it (Recursive Step)
                        if (outer_spike.frame < inner_spike.frame - noiseDuration)
                        {
                            // Occured too far before the inner spike to be related
                            // keep searching
                        }
                        else
                        {
                            return filteredOuterSpike(pQueue, inner_spike, max_spike);
                        }
                    }
                }
            }
        }
        return filtered_spike;
        //}
    }

    void SpikeDecayFilterer::filterInnerNeighbors(SpikeQueue *pQueue, Spike max_spike)
    {
        Spike curr_spike(0, 0, 0);
        int curr_channel, curr_amp, curr_frame;
        SpikeQueue::iterator it;
        it = pQueue->begin();
        int frameBound = max_spike.frame + noiseDuration + 1;

        while (it != pQueue->end())
        {
            curr_spike = *it;
            curr_channel = it->channel;
            curr_amp = it->amplitude;
            curr_frame = it->frame;
            if (curr_frame < frameBound)
            {
                if (pLayout->areNeighbors(max_spike.channel, curr_channel))
                {
                    if (pLayout->areInnerNeighbors(max_spike.channel, curr_channel))
                    {
                        if (curr_amp < max_spike.amplitude)
                        {
                            if (curr_amp >= max_spike.amplitude * noiseAmpPercent)
                            {
                                it = pQueue->erase(it);
                            }
                            else
                            {
                                it = pQueue->erase(it);
                            }
                        }
                        else
                        {
                            // Inner neighbor has larger amplitude that max neighbor (probably new spike), filter later
                            ++it;
                        }
                    }
                    else
                    {
                        // In outer neighbors, filter later
                        ++it;
                    }
                }
                else
                {
                    // Not a neighbor, filter later
                    ++it;
                }
            }
            else
            {
                // Inner spike occurred much later that max_spike and is only slightly smaller (probably new spike), filter later
                ++it;
            }
        }
    }

} // namespace HSDetection
