#include "FilterSpikes.h"
#include <cmath>
#include <climits>
#include "Detection.h"

using namespace std;

namespace FilterSpikes
{

    Spike filterSpikesDecay(Spike first_spike)
    {
        /*Removes all duplicate spikes and returns the max spike

        Parameters
        ----------
        first_spike: Spike
            The first spike detected in the event.

        Returns
        -------
        max_spike: Spike
            The largest amplitude spike belonging to the event of the first spike.
        */
        int curr_channel, curr_amp, curr_frame;
        Spike curr_spike(0, 0, 0);
        Spike max_spike(0, 0, 0);
        int max_spike_amp;
        max_spike = first_spike;
        max_spike_amp = first_spike.amplitude;
        // Find the max amplitude neighbor of the first spike
        HSDetection::SpikeQueue::iterator it;
        HSDetection::SpikeQueue::iterator max_it;
        max_it = it = HSDetection::Detection::queue.begin();

        // Find max
        while (it != HSDetection::Detection::queue.end())
        {
            curr_spike = *it;
            curr_channel = it->channel;
            curr_amp = it->amplitude;
            curr_frame = it->frame;
            if (areNeighbors(first_spike.channel, curr_channel))
            {
                if (curr_amp >= max_spike_amp)
                {
                    if (curr_frame <= first_spike.frame + HSDetection::Detection::noise_duration)
                    {
                        max_spike = curr_spike;
                        max_spike_amp = curr_amp;
                        max_it = it;
                    }
                }
            }
            ++it;
        }
        HSDetection::Detection::queue.erase(max_it);

        if (!HSDetection::Detection::queue.empty())
        {
            filterOuterNeighbors(max_spike);
            filterInnerNeighbors(max_spike);
        }
        return max_spike;
    }

    void filterOuterNeighbors(Spike max_spike)
    {
        /*Filters or leaves all outer neighbors based on minimum spanning tree
        algorithm. Basically, the outer spikes must pass through an inner spike
        to reach the maximum

        Parameters
        ----------
        max_spike: Spike
            The max spike detected in the event.
        */

        Spike curr_spike(0, 0, 0);
        int curr_channel, curr_frame;
        vector<Spike> outer_spikes_to_be_filtered;
        HSDetection::SpikeQueue::iterator it;
        it = HSDetection::Detection::queue.begin();

        while (it != HSDetection::Detection::queue.end())
        {
            curr_spike = *it;
            curr_channel = curr_spike.channel;
            curr_frame = curr_spike.frame;
            if (curr_frame <= max_spike.frame + HSDetection::Detection::noise_duration)
            {
                if (areNeighbors(max_spike.channel, curr_channel))
                {
                    if (!isInnerNeighbor(max_spike.channel, curr_channel))
                    {
                        if (filteredOuterSpike(curr_spike, max_spike))
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

            it = HSDetection::Detection::queue.begin();
            while (it != HSDetection::Detection::queue.end())
            {
                curr_spike = *it;
                curr_channel = curr_spike.channel;
                curr_frame = curr_spike.frame;
                if (curr_channel == curr_channel_to_be_filtered && curr_frame == curr_frame_to_be_filtered)
                {
                    it = HSDetection::Detection::queue.erase(it);
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

    bool filteredOuterSpike(Spike outer_spike, Spike max_spike)
    {
        // True if should be filtered, false by default
        bool filtered_spike = false;
        // bool IS_INNER_EVENT = true;
        bool shares_inner = false;
        int NOT_VALID_FRAME = -1;

        int curr_inner_neighbor;
        for (int i = 0; i < HSDetection::Detection::max_neighbors; i++)
        {
            curr_inner_neighbor = HSDetection::Detection::inner_neighbor_matrix[outer_spike.channel][i];
            if (curr_inner_neighbor == -1)
            {
                break;
            }
            else
            {
                if (isInnerNeighbor(max_spike.channel, curr_inner_neighbor))
                {
                    HSDetection::SpikeQueue::iterator it;
                    it = HSDetection::Detection::queue.begin();
                    // Find max
                    while (it != HSDetection::Detection::queue.end())
                    {
                        if (curr_inner_neighbor == it->channel)
                        {
                            if (outer_spike.amplitude < it->amplitude * HSDetection::Detection::noise_amp_percent)
                            {
                                if (outer_spike.frame < it->frame - HSDetection::Detection::noise_duration)
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
        }
        // for(int i = 0; i < Parameters::max_neighbors; i++) {
        float curr_dist;
        float outer_dist_from_center = channelsDist(outer_spike.channel, max_spike.channel);
        for (int i = 0; i < HSDetection::Detection::max_neighbors; i++)
        {
            int curr_inner_channel = HSDetection::Detection::inner_neighbor_matrix[outer_spike.channel][i];
            // out of inner channels
            if (curr_inner_channel == -1)
            {
                break;
            }
            else
            {
                curr_dist = channelsDist(curr_inner_channel, max_spike.channel);
                if (curr_dist < outer_dist_from_center)
                {
                    Spike inner_spike = getSpikeFromChannel(curr_inner_channel);
                    if (inner_spike.frame == NOT_VALID_FRAME)
                    {
                        // not a spike, keep searching
                    }
                    else
                    {
                        if (outer_spike.amplitude >= inner_spike.amplitude * HSDetection::Detection::noise_amp_percent)
                        {
                            // keep searching
                        }
                        else
                        {
                            // Find the closest neighbor in the spike vector and recurse with it (Recursive Step)
                            if (outer_spike.frame < inner_spike.frame - HSDetection::Detection::noise_duration)
                            {
                                // Occured too far before the inner spike to be related
                                // keep searching
                            }
                            else
                            {
                                return filteredOuterSpike(inner_spike, max_spike);
                            }
                        }
                    }
                }
            }
        }
        return filtered_spike;
        //}
    }

    void filterInnerNeighbors(Spike max_spike)
    {
        /*Filters or leaves all outer neighbors based on minimum spanning tree
        algorithm. Basically, the outer spikes must pass through an inner spike
        to reach the maximum

        Parameters
        ----------
        max_spike: Spike
            The max spike detected in the event.
        */

        Spike curr_spike(0, 0, 0);
        int curr_channel, curr_amp, curr_frame;
        HSDetection::SpikeQueue::iterator it;
        it = HSDetection::Detection::queue.begin();

        while (it != HSDetection::Detection::queue.end())
        {
            curr_spike = *it;
            curr_channel = it->channel;
            curr_amp = it->amplitude;
            curr_frame = it->frame;
            if (curr_frame <= max_spike.frame + HSDetection::Detection::noise_duration)
            {
                if (areNeighbors(max_spike.channel, curr_channel))
                {
                    if (isInnerNeighbor(max_spike.channel, curr_channel))
                    {
                        if (curr_amp < max_spike.amplitude)
                        {
                            if (curr_amp >= max_spike.amplitude * HSDetection::Detection::noise_amp_percent)
                            {
                                it = HSDetection::Detection::queue.erase(it);
                            }
                            else
                            {
                                it = HSDetection::Detection::queue.erase(it);
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

    float channelsDist(int start_channel, int end_channel)
    {
        /*Finds the distance between two channels

        Parameters
        ----------
        start_channel: int
            The start channel where distance measurement begins
        end_channel: int
            The end channel where distance measurement ends

        Returns
        -------
        dist: float
            The distance between the two channels
        */
        float start_position_x;
        float start_position_y;
        float end_position_x;
        float end_position_y;
        float x_displacement;
        float y_displacement;
        float dist;

        start_position_x = HSDetection::Detection::channel_positions[start_channel][0];
        start_position_y = HSDetection::Detection::channel_positions[start_channel][1];
        end_position_x = HSDetection::Detection::channel_positions[end_channel][0];
        end_position_y = HSDetection::Detection::channel_positions[end_channel][1];
        x_displacement = start_position_x - end_position_x;
        y_displacement = start_position_y - end_position_y;
        dist = sqrt(pow(x_displacement, 2) + pow(y_displacement, 2));

        return dist;
    }

    bool isInnerNeighbor(int center_channel, int curr_channel)
    {
        /*Determines whether or not two channels are inner neighbors.

        Parameters
        ----------
        center_channel: int
            The channel number whose inner neighborhoold is checked.
        curr_channel: int
            The channel number of the potential neighbor.

        Returns
        -------
        is_inner_neighbor: bool
            True if the channels are inner neighbors.
            False if the channels are not inner neighbors.
        */
        bool is_inner_neighbor = false;

        int curr_inner_neighbor;
        for (int i = 0; i < HSDetection::Detection::max_neighbors; i++)
        {
            curr_inner_neighbor = HSDetection::Detection::inner_neighbor_matrix[center_channel][i];
            if (curr_inner_neighbor == curr_channel)
            {
                is_inner_neighbor = true;
                break;
            }
            else if (curr_inner_neighbor == -1)
            {
                break;
            }
        }
        return is_inner_neighbor;
    }

    bool areNeighbors(int channel_one, int channel_two)
    {
        /*Determines whether or not two channels are neighbors.

        Parameters
        ----------
        channel_one: int
            The channel number whose neighborhoold is checked.
        channel_two: int
            The channel number of the potential neighbor.

        Returns
        -------
        are_neighbors: bool
            True if the channels are neighbors.
            False if the channels are not neighbors.
        */
        bool are_neighbors = false;
        int curr_neighbor;
        for (int i = 0; i < HSDetection::Detection::max_neighbors; i++)
        {
            curr_neighbor = HSDetection::Detection::neighbor_matrix[channel_one][i];
            if (curr_neighbor == channel_two)
            {
                are_neighbors = true;
                break;
            }
            else if (curr_neighbor == -1)
            {
                break;
            }
        }
        return are_neighbors;
    }

    Spike getSpikeFromChannel(int channel)
    {
        HSDetection::SpikeQueue::iterator it;
        it = HSDetection::Detection::queue.begin();
        // Find max
        while (it != HSDetection::Detection::queue.end())
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

}
