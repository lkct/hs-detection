#include "SpikeHandler.h"
#include <deque>
#include <iostream>
#include <fstream>
#include <string>
#include "Parameters.h"
#include <cmath>
#include <algorithm>
#include "ProcessSpikes.h"
#include "Detection.h"

using namespace std;

int Parameters::aGlobal;
int **Parameters::baselines;
int Parameters::index_baselines;

deque<Spike> Parameters::spikes_to_be_processed;
ofstream spikes_filtered_file;

namespace SpikeHandler
{

    struct CustomLessThan
    {
        bool operator()(tuple<int, float> const &lhs, tuple<int, float> const &rhs) const
        {
            return std::get<1>(lhs) < std::get<1>(rhs);
        }
    };

    void setInitialParameters(string file_name)
    {
        /*This sets all the initial parameters needed to run the filtering algorithm.

        Parameters
        ----------
        _num_channels: int
                Number of channels on the probe
      (where the beginning of the spike was).
        file_name: string
        The name of the file that the processed spikes will be written to in binary.
        _noise_duration: int
                The frames in which the true spike can occur after the first detection
      of the spike.
                Ideally, the _noise_duration would be zero if there is no noise (then
      the first spike
                that occurs is the original spike), but sometimes the true spike is
      detected after
                a duplicate.
        _noise_amp_percent: float
                The amplitude percent difference at which two spikes can be considered
      unique
                even if the spike detected after has a smaller amplitude (with zero
      _noise_amp_percent,
                two concurrent spikes where the second spike has a slightly smaller
      amplitude
                will be considered duplicates).
      _masked_channels: 1D int array
        The channels that are masked for the detection (AKA we get input from them)
        _channel_positions: 2D int array
                Indexed by the channel number starting at 0 and going up to
      num_channels: Each
                index contains pointer to another array which contains X and Y
      position of the channel. User creates
                this before calling SpikeHandler.
        _neighbor_matrix: 2D int array
                Indexed by the channel number starting at 0 and going up to
      num_channels: Each
                index contains pointer to another array which contains channel number
      of all its neighbors.
                User creates this before calling SpikeHandler.
        _max_neighbors: int
                The maximum number of neighbors a channel can have (a neighbor is any
      channel that can receive the
                same spike waveform as the original channel if the spike occurred at
      the original channel).
        _to_localize: bool
                True: Localize the spike using our localization method
                False: Do not localize.
        cutout_length: int
                The cutout length to be written out for the spike. Can't be larger
      than extra data tacked on to raw data.
        _spikes_to_be_processed: Spike deque
                Contains all spikes to be proccessed when spike_peak_duration number
      of frames is stored.
        _maxsl: int
                The number of frames until a spike is accepted when the peak value is
      given.
        */

        HSDetection::Detection::inner_neighbor_matrix = createInnerNeighborMatrix();
        HSDetection::Detection::outer_neighbor_matrix = createOuterNeighborMatrix();
        fillNeighborLayerMatrices();
        if (false)
        {
            for (int i = 0; i < HSDetection::Detection::num_channels; i++)
            {
                cerr << "Channel: " << i << endl;
                cerr << "Inner Neighbors: ";
                for (int j = 0; j < HSDetection::Detection::max_neighbors; j++)
                {
                    cerr << HSDetection::Detection::inner_neighbor_matrix[i][j] << "  ";
                }
                cerr << endl;
                cerr << "Outer Neighbors: ";
                for (int k = 0; k < HSDetection::Detection::max_neighbors; k++)
                {
                    cerr << HSDetection::Detection::outer_neighbor_matrix[i][k] << "  ";
                }
                cerr << endl;
            }
        }

        if (spikes_filtered_file.is_open())
        {
            spikes_filtered_file.close();
        }
        //   spikes_filtered_file.open(file_name + ".bin", ios::trunc | ios::binary);
        spikes_filtered_file.open(file_name + ".bin", ios::binary);
        Parameters::spikes_to_be_processed.clear();
    }

    void setLocalizationParameters(int _aGlobal, int **_baselines,
                                   int _index_baselines)
    {
        /*Sets all time dependent variables for localization. The localization needs a
        global noise value
        and a precalculated median baseline value for processing amplitudes of spikes.

        Parameters
        ----------
        _aGlobal: int
                The global noise values for all the channels.
        _baselines: int
                Contains spike_delay number of frames of median baseline values.
        Updated by user at every frame.
        */
        if (_index_baselines < 0)
        {
            spikes_filtered_file.close();
            cerr << "Index baselines less than 0. Terminating Spike Handler" << endl;
            exit(EXIT_FAILURE);
        }
        Parameters::aGlobal = _aGlobal;
        Parameters::baselines = _baselines;
        Parameters::index_baselines = _index_baselines;
    }

    void addSpike(int channel, int frame, int amplitude)
    {
        /*Adds a spike to the spikes_to_be_processed deque. Once the frame of the
      spike to be added is
        greater than the spike_peak_duration larger than the first spike in the deque,
      it will process
        all the current spikes in the deque and then attempt to add the spike again.
      It will keep repeating
        this process until the spike to be added frame is smaller than the
      spike_peak_duration plus the frames
      of the first spike or the deque is empty.

        Parameters
        ----------
        channel: int
                The channel at which the spike is detected.
        frame: int
                The frame at which the spike is detected.
        amplitude: int
                The amplitude at which the spike is detected.
        */
        int cutout_size = HSDetection::Detection::cutout_start + HSDetection::Detection::cutout_end + 1;

        Spike spike_to_be_added;
        spike_to_be_added.channel = channel;
        spike_to_be_added.frame = frame;
        spike_to_be_added.amplitude = amplitude;

        if (false)
        {
            cerr << "storing COM cutouts " << endl;
        }
        spike_to_be_added = storeWaveformCutout(cutout_size, spike_to_be_added);
        if (false)
        {
            cerr << "... done storing COM cutouts " << endl;
        }
        if (HSDetection::Detection::to_localize)
        {
            if (false)
            {
                cerr << "Storing counts..." << endl;
            }
            spike_to_be_added = storeCOMWaveformsCounts(spike_to_be_added);
            if (false)
            {
                cerr << "... done storing counts!" << endl;
            }
        }

        bool isAdded = false;
        while (!isAdded)
        {
            if (Parameters::spikes_to_be_processed.empty())
            {
                Parameters::spikes_to_be_processed.push_back(spike_to_be_added);
                isAdded = true;
            }
            else
            {
                Spike first_spike = Parameters::spikes_to_be_processed.front();
                int first_frame = first_spike.frame;
                int current_frame = spike_to_be_added.frame;
                if (current_frame > first_frame + (HSDetection::Detection::spike_peak_duration +
                                                   HSDetection::Detection::noise_duration))
                {
                    if (HSDetection::Detection::to_localize)
                    {
                        try
                        {
                            if (false)
                            {
                                cerr << "spike frame: " << spike_to_be_added.frame << endl;
                            }
                            ProcessSpikes::filterLocalizeSpikes(spikes_filtered_file);
                        }
                        catch (...)
                        {
                            spikes_filtered_file.close();
                            cerr << "Baseline matrix or its parameters entered incorrectly. "
                                    "Terminating SpikeHandler."
                                 << endl;
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        ProcessSpikes::filterSpikes(spikes_filtered_file);
                    }
                }
                else
                {
                    Parameters::spikes_to_be_processed.push_back(spike_to_be_added);
                    isAdded = true;
                }
            }
        }
    }
    void terminateSpikeHandler()
    {
        // Filter any remaining spikes leftover at the end and close the spike file.
        while (Parameters::spikes_to_be_processed.size() != 0)
        {
            if (HSDetection::Detection::to_localize)
            {
                ProcessSpikes::filterLocalizeSpikes(spikes_filtered_file);
            }
            else
            {
                ProcessSpikes::filterSpikes(spikes_filtered_file);
            }
        }
        spikes_filtered_file.close();
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

    void fillNeighborLayerMatrices()
    {
        if (false)
        {
            cerr << "Filling Neighbor Layer Matrix" << endl;
        }
        int curr_channel;
        int curr_neighbor;
        float curr_dist;
        vector<tuple<int, float>> distances_neighbors;
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
                    distances_neighbors.push_back(make_tuple(curr_neighbor, curr_dist));
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

    vector<int>
    getInnerNeighborsRadius(vector<tuple<int, float>> distances_neighbors,
                            int central_channel)
    {
        int curr_neighbor;
        float curr_dist;
        vector<int> inner_channels;
        vector<tuple<int, float>>::iterator it;
        inner_channels.push_back(central_channel);
        it = distances_neighbors.begin();
        while (it != distances_neighbors.end())
        {
            curr_neighbor = get<0>(*it);
            curr_dist = get<1>(*it);
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

    Spike storeWaveformCutout(int cutout_size, Spike curr_spike)
    {
        /*Stores the largest waveform in the written_cutout deque of the spike

        Parameters
        ----------
        channel: int
                The channel at which the spike is detected.
        cutout_size: int
                The length of the cutout
        curr_spike: Spike
                The current detected spike

        Returns
        ----------
        curr_spike: Spike
                The current detected spike (now with the waveform stored)
        */
        int32_t curr_written_reading;
        for (int i = 0; i < cutout_size; i++)
        {
            try
            {
                curr_written_reading = HSDetection::Detection::trace.get(
                    curr_spike.frame - HSDetection::Detection::cutout_start + i,
                    curr_spike.channel);
            }
            catch (...)
            {
                spikes_filtered_file.close();
                cerr << "Raw Data and it parameters entered incorrectly, could not "
                        "access data. Terminating SpikeHandler."
                     << endl;
                exit(EXIT_FAILURE);
            }
            curr_spike.written_cutout.push_back(curr_written_reading);
        }
        return curr_spike;
    }

    Spike storeCOMWaveformsCounts(Spike curr_spike)
    {
        /*Stores the inner neighbor waveforms in the amp_cutouts deque of the spike

        Parameters
        ----------
        channel: int
                The channel at which the spike is detected.
        curr_spike: Spike
                The current detected spike

        Returns
        ----------
        curr_spike: Spike
                The current detected spike (now with the nearest waveforms stored)
        */

        vector<int> com_cutouts;
        int *nearest_neighbor_counts;
        nearest_neighbor_counts = new int[HSDetection::Detection::num_com_centers];
        for (int i = 0; i < HSDetection::Detection::num_com_centers; i++)
        {
            nearest_neighbor_counts[i] = 0;
        }

        // Get closest channels for COM
        int channel = curr_spike.channel;
        for (int i = 0; i < HSDetection::Detection::num_com_centers; i++)
        {
            int curr_max_channel = HSDetection::Detection::inner_neighbor_matrix[channel][i];
            if (curr_max_channel == -1)
            {
                cerr << "num_com_centers too large. Not enough inner neighbors."
                     << endl;
                exit(EXIT_FAILURE);
            }
            curr_spike.largest_channels.push_back(curr_max_channel);
            for (int j = 0; j < HSDetection::Detection::max_neighbors; j++)
            {
                int curr_neighbor_channel = HSDetection::Detection::inner_neighbor_matrix[curr_max_channel][j];
                // Out of inner neighbors
                if (curr_neighbor_channel != -1)
                {
                    nearest_neighbor_counts[i] += 1;
                    // Check if noise_duration is too large in comparison to the buffer size
                    int cutout_size = HSDetection::Detection::cutout_start + HSDetection::Detection::cutout_end + 1;
                    int amp_cutout_size, cutout_start_index;
                    if (HSDetection::Detection::cutout_start < HSDetection::Detection::noise_duration || HSDetection::Detection::cutout_end < HSDetection::Detection::noise_duration)
                    {
                        amp_cutout_size = cutout_size;
                        cutout_start_index = HSDetection::Detection::cutout_start;
                    }
                    else
                    {
                        amp_cutout_size = HSDetection::Detection::noise_duration * 2;
                        cutout_start_index = HSDetection::Detection::noise_duration;
                    }
                    for (int k = 0; k < amp_cutout_size; k++)
                    {
                        int curr_reading = HSDetection::Detection::trace.get(
                            curr_spike.frame - cutout_start_index + k, curr_neighbor_channel);
                        int curr_amp = ((curr_reading - Parameters::aGlobal) * HSDetection::Detection::ASCALE -
                                        Parameters::baselines[curr_neighbor_channel]
                                                             [Parameters::index_baselines]);
                        if (curr_amp < 0)
                        {
                            com_cutouts.push_back(0);
                        }
                        else
                        {
                            com_cutouts.push_back(curr_amp);
                        }
                    }
                }
                // Out of neighbors to add cutout for
                else
                {
                    break;
                }
            }
        }
        curr_spike.waveformscounts = make_tuple(com_cutouts, nearest_neighbor_counts);
        return curr_spike;
    }

}
