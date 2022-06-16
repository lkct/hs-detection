#include "Utils.h"
#include <deque>
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include "Detection.h"

using namespace std;

namespace Utils
{

    float channelsDist(int start_channel, int end_channel)
    {
        return (HSDetection::Detection::channel_positions[start_channel] - HSDetection::Detection::channel_positions[end_channel]).abs();
    }

    vector<int>
    getInnerNeighborsRadius(vector<pair<int, float>> distances_neighbors,
                            int central_channel)
    {
        int curr_neighbor;
        float curr_dist;
        vector<int> inner_channels;
        vector<pair<int, float>>::iterator it;
        inner_channels.push_back(central_channel);
        it = distances_neighbors.begin();
        while (it != distances_neighbors.end())
        {
            curr_neighbor = it->first;
            curr_dist = it->second;
            if (curr_dist <= HSDetection::Detection::inner_radius)
            {
                inner_channels.push_back(curr_neighbor);
                ++it;
            }
            else
            {
                break;
            }
        }
        return inner_channels;
    }

    struct CustomLessThan
    {
        bool operator()(pair<int, float> const &lhs, pair<int, float> const &rhs) const
        {
            return lhs.second < rhs.second;
        }
    };

    void fillNeighborLayerMatrices()
    {
        int curr_channel;
        int curr_neighbor;
        float curr_dist;
        vector<pair<int, float>> distances_neighbors;
        vector<int> inner_neighbors;
        for (int i = 0; i < HSDetection::Detection::num_channels; i++)
        {
            curr_channel = i;
            for (int j = 0; j < HSDetection::Detection::max_neighbors; j++)
            {
                curr_neighbor = HSDetection::Detection::neighbor_matrix[curr_channel][j];
                if (curr_channel != curr_neighbor && curr_neighbor != -1)
                {
                    curr_dist = channelsDist(curr_neighbor, curr_channel);
                    distances_neighbors.push_back(make_pair(curr_neighbor, curr_dist));
                }
            }
            if (distances_neighbors.size() != 0)
            {
                sort(begin(distances_neighbors), end(distances_neighbors),
                     CustomLessThan());
                inner_neighbors = getInnerNeighborsRadius(distances_neighbors, i);
            }

            vector<int>::iterator it;
            it = inner_neighbors.begin();
            int curr_inner_neighbor;
            int k = 0;
            // Fill Inner neighbors matrix
            while (it != inner_neighbors.end())
            {
                curr_inner_neighbor = *it;
                HSDetection::Detection::inner_neighbor_matrix[i][k] = curr_inner_neighbor;
                ++k;
                ++it;
            }
            while (k < HSDetection::Detection::max_neighbors)
            {
                HSDetection::Detection::inner_neighbor_matrix[i][k] = -1;
                ++k;
            }
            // Fill outer neighbor matrix
            k = 0;
            for (int l = 0; l < HSDetection::Detection::max_neighbors; l++)
            {
                curr_neighbor = HSDetection::Detection::neighbor_matrix[i][l];
                if (curr_neighbor != -1 && curr_neighbor != i)
                {
                    bool is_outer_neighbor = true;
                    for (size_t m = 0; m < inner_neighbors.size(); m++)
                    {
                        if (HSDetection::Detection::inner_neighbor_matrix[i][m] == curr_neighbor)
                        {
                            is_outer_neighbor = false;
                            break;
                        }
                    }
                    if (is_outer_neighbor)
                    {
                        HSDetection::Detection::outer_neighbor_matrix[i][k] = curr_neighbor;
                        ++k;
                    }
                }
            }
            while (k < HSDetection::Detection::max_neighbors)
            {
                HSDetection::Detection::outer_neighbor_matrix[i][k] = -1;
                ++k;
            }
            inner_neighbors.clear();
            distances_neighbors.clear();
        }
    }

    int **createInnerNeighborMatrix()
    {
        int **inner_neighbor_matrix;

        inner_neighbor_matrix = new int *[HSDetection::Detection::num_channels];
        for (int i = 0; i < HSDetection::Detection::num_channels; i++)
        {
            inner_neighbor_matrix[i] = new int[HSDetection::Detection::max_neighbors];
        }

        return inner_neighbor_matrix;
    }

    int **createOuterNeighborMatrix()
    {
        int **outer_neighbor_matrix;

        outer_neighbor_matrix = new int *[HSDetection::Detection::num_channels];
        for (int i = 0; i < HSDetection::Detection::num_channels; i++)
        {
            outer_neighbor_matrix[i] = new int[HSDetection::Detection::max_neighbors];
        }
        return outer_neighbor_matrix;
    }

    Spike storeWaveformCutout(Spike curr_spike)
    {
        for (int t = curr_spike.frame - HSDetection::Detection::cutout_start;
             t < curr_spike.frame + HSDetection::Detection::cutout_end + 1;
             t++)
        {
            curr_spike.written_cutout.push_back(HSDetection::Detection::trace(t, curr_spike.channel));
        }
        return curr_spike;
    }

    Spike storeCOMWaveformsCounts(Spike curr_spike)
    {
        int num_com_centers = HSDetection::Detection::num_com_centers;
        int **inner_neighbor_matrix = HSDetection::Detection::inner_neighbor_matrix;
        int max_neighbors = HSDetection::Detection::max_neighbors;
        int cutout_start = HSDetection::Detection::cutout_start;
        int cutout_end = HSDetection::Detection::cutout_end;
        int cutout_size = HSDetection::Detection::cutout_size;
        int noise_duration = HSDetection::Detection::noise_duration;

        vector<vector<pair<int, int>>> chAmps(num_com_centers, vector<pair<int, int>>());

        // Get closest channels for COM
        for (int i = 0; i < num_com_centers; i++)
        {
            int max_channel = inner_neighbor_matrix[curr_spike.channel][i];
            if (max_channel == -1)
            {
                cerr << "num_com_centers too large. Not enough inner neighbors."
                     << endl;
                exit(EXIT_FAILURE);
            }

            for (int j = 0; j < max_neighbors; j++)
            {
                int curr_neighbor_channel = inner_neighbor_matrix[max_channel][j];
                // Out of inner neighbors
                if (curr_neighbor_channel == -1)
                {
                    break;
                }

                // TODO: assert (cutout_start >= noise_duration && cutout_end >= noise_duration)

                int sum = 0;
                for (int t = curr_spike.frame - noise_duration; t < curr_spike.frame + noise_duration; t++)
                {
                    int curr_reading = HSDetection::Detection::trace(t, curr_neighbor_channel);
                    int curr_amp = (curr_reading - curr_spike.aGlobal) * HSDetection::Detection::ASCALE -
                                   curr_spike.baselines[curr_neighbor_channel];
                    if (curr_amp >= 0)
                    {
                        sum += curr_amp;
                    }
                }
                chAmps[i].push_back(make_pair(curr_neighbor_channel, sum));
            }
        }
        curr_spike.waveforms = chAmps;
        return curr_spike;
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

}
