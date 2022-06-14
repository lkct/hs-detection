#include "LocalizeSpikes.h"
#include <iostream>
#include <algorithm>
#include "Detection.h"

using namespace std;

namespace LocalizeSpikes
{

    struct CustomLessThan
    {
        bool operator()(tuple<int, int> const &lhs,
                        tuple<int, int> const &rhs) const
        {
            return std::get<1>(lhs) < std::get<1>(rhs);
        }
    };

    Point localizeSpike(Spike spike_to_be_localized)
    {
        /*Estimates the X and Y position of where a spike occured on the probe.

           Parameters
           ----------
           spike_to_be_localized: Spike
           The spike that will be used to determine where the origin of the spike
           occurred.

           Returns
           -------
           position: tuple<float, float>
           An X and Y coordinate tuple that corresponds to where the spike occurred.
         */

        vector<int> *waveforms = &spike_to_be_localized.waveforms;
        vector<int> *neighbor_counts = &spike_to_be_localized.neighbor_counts;
        vector<int> *largest_channels = &spike_to_be_localized.largest_channels;
        vector<tuple<Point, int>> com_positions_amps;

        int cutout_size = HSDetection::Detection::noise_duration * 2; // compute amplitudes using sum over 2*noise_duration data points

        int offset = 0;

        for (int i = 0; i < HSDetection::Detection::num_com_centers; i++)
        {
            int neighbor_count = (*neighbor_counts)[i];
            int curr_max_channel = (*largest_channels)[i];
            vector<tuple<int, int>> amps;

            for (int j = 0; j < neighbor_count; j++, offset++)
            {
                int curr_neighbor_channel = HSDetection::Detection::inner_neighbor_matrix[curr_max_channel][j];
                int sum_amp = 0;
                for (int k = 0; k < cutout_size; k++)
                {
                    sum_amp += (*waveforms)[k + offset * cutout_size];
                }
                amps.push_back(make_tuple(curr_neighbor_channel, sum_amp));
            }

            // compute median, threshold at median
            int do_correction = 1;
            int correct = 0;
            int amps_size = amps.size();
            if (do_correction == 1)
            {
                sort(begin(amps), end(amps), CustomLessThan()); // sort the array
                // correct = get<1>(amps.at(0))-1;
                if (amps_size % 2 == 0)
                {
                    correct = (get<1>(amps.at(amps_size / 2 - 1)) + get<1>(amps.at(amps_size / 2))) / 2;
                }
                else
                {
                    correct = get<1>(amps.at(amps_size / 2));
                }
            }
            // Correct amplitudes (threshold)
            vector<tuple<int, int>> centered_amps;
            if (amps_size != 1)
            {
                for (int i = 0; i < amps_size; i++)
                {
                    if (get<1>(amps.at(i)) - correct > 0)
                    {
                        centered_amps.push_back(
                            make_tuple(get<0>(amps.at(i)), get<1>(amps.at(i)) - correct));
                    }
                }
            }
            else
            {
                centered_amps.push_back(amps.at(0));
            }

            Point position = centerOfMass(centered_amps);
            tuple<Point, int> position_amp_tuple = make_tuple(position, 1);
            com_positions_amps.push_back(position_amp_tuple);

            amps.clear();
            centered_amps.clear();
        }

        Point reweighted_com(0, 0);
        if (com_positions_amps.size() > 1)
        {
            reweighted_com = reweightedCenterOfMass(com_positions_amps);
        }
        else
        {
            reweighted_com = get<0>(com_positions_amps[0]);
        }

        return reweighted_com;
    }

    Point reweightedCenterOfMass(vector<tuple<Point, int>> com_positions_amps)
    {
        float X = 0;
        float Y = 0;
        float X_numerator = 0;
        float Y_numerator = 0;
        int denominator = 0;
        float X_coordinate;
        float Y_coordinate;
        int weight; // contains the amplitudes for the center of mass calculation.
                    // Updated each localization

        for (int i = 0; i < HSDetection::Detection::num_com_centers; i++)
        {
            X_coordinate = get<0>((com_positions_amps[i])).x;
            Y_coordinate = get<0>((com_positions_amps[i])).y;
            weight = get<1>(com_positions_amps[i]);
            if (weight < 0)
            {
                cerr << "\ncenterOfMass::weight < 0 - this should not happen" << endl;
            }
            X_numerator += weight * X_coordinate;
            Y_numerator += weight * Y_coordinate;
            denominator += weight;
        }

        if (denominator == 0)
        {
            cerr << "Whopodis" << endl;
            for (int i = 0; i < HSDetection::Detection::num_com_centers; i++)
            {
                X_coordinate = get<0>((com_positions_amps[i])).x;
                Y_coordinate = get<0>((com_positions_amps[i])).y;
                weight = get<1>(com_positions_amps[i]);
                if (weight < 0)
                {
                    cerr << "\ncenterOfMass::weight < 0 - this should not happen" << endl;
                }
                cerr << "Weight" << weight << endl;
                cerr << "X coordinate" << X_coordinate << endl;
                cerr << "Y coordinate" << Y_coordinate << endl;
            }
        }

        X = (X_numerator) / (float)(denominator);
        Y = (Y_numerator) / (float)(denominator);

        return Point(X, Y);
    }

    Point centerOfMass(vector<tuple<int, int>> centered_amps)
    {
        /*Calculates the center of mass of a spike to calculate where it occurred
           using a weighted average.

           Parameters
           ----------
           centered_amps: deque<tuple<int, int>>
           A deque containing all non-zero amplitudes and their neighbors. Used for
           center of mass.

           Returns
           -------
           position: tuple<float, float>
           An X and Y coordinate tuple that corresponds to where the spike occurred.
         */
        // int curr_amp;
        float X = 0;
        float Y = 0;
        int X_numerator = 0;
        int Y_numerator = 0;
        int denominator = 0;
        int X_coordinate;
        int Y_coordinate;
        int channel;
        int weight; // contains the amplitudes for the center of mass calculation.
                    // Updated each localization
        int centered_amps_size = centered_amps.size();

        for (int i = 0; i < centered_amps_size; i++)
        {
            weight = get<1>(centered_amps.at(i));
            channel = get<0>(centered_amps.at(i));
            X_coordinate = HSDetection::Detection::channel_positions[channel][0];
            Y_coordinate = HSDetection::Detection::channel_positions[channel][1];
            if (weight < 0)
            {
                cerr << "\ncenterOfMass::weight < 0 - this should not happen" << endl;
            }
            X_numerator += weight * X_coordinate;
            Y_numerator += weight * Y_coordinate;
            denominator += weight;
        }

        if (denominator == 0) //| (X>10 & X<11))
        {
            // cerr << "\ncenterOfMass::denominator == 0 - This should not happen\n";
            for (int i = 0; i < centered_amps_size; i++)
            {
                channel = get<0>(centered_amps.at(i));
                // cerr << " " << get<1>(centered_amps.at(i)) << " "
                //      << Parameters::channel_positions[channel][0] << "\n";
                X = HSDetection::Detection::channel_positions[channel][0];
                Y = HSDetection::Detection::channel_positions[channel][1];
            }
            cerr << endl;
        }
        else
        {
            X = (float)(X_numerator) / (float)(denominator);
            Y = (float)(Y_numerator) / (float)(denominator);
        }

        return Point(X, Y);
    }

}
