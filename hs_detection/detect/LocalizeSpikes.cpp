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

        vector<vector<int>> *waveforms = &spike_to_be_localized.waveforms;
        vector<int> *neighbor_counts = &spike_to_be_localized.neighbor_counts;
        vector<int> *largest_channels = &spike_to_be_localized.largest_channels;

        Point sumPos(0, 0);
        int sumWeight = 0;

        for (int i = 0; i < HSDetection::Detection::num_com_centers; i++)
        {
            int neighbor_count = (*neighbor_counts)[i];
            int curr_max_channel = (*largest_channels)[i];
            vector<tuple<int, int>> amps;

            for (int j = 0; j < neighbor_count; j++)
            {
                int curr_neighbor_channel = HSDetection::Detection::inner_neighbor_matrix[curr_max_channel][j];
                amps.push_back(make_tuple(curr_neighbor_channel, (*waveforms)[i][j]));
            }

            // // compute median, threshold at median
            nth_element(amps.begin(), amps.begin() + (neighbor_count - 1) / 2, amps.end(), CustomLessThan());
            int correct = get<1>(amps[(neighbor_count - 1) / 2]);
            if (neighbor_count % 2 == 0)
            {
                correct = (get<1>(*min_element(amps.begin() + neighbor_count / 2, amps.end(), CustomLessThan())) + correct) / 2;
            }

            // Correct amplitudes (threshold)
            vector<tuple<int, int>> centered_amps;
            if (neighbor_count != 1)
            {
                for (int i = 0; i < neighbor_count; i++)
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

            sumPos += centerOfMass(centered_amps);
            sumWeight += 1; // TODO: inc amount?
        }

        // TODO: div 0 check???
        return sumPos *= 1 / (float)sumWeight;
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
            X_coordinate = HSDetection::Detection::channel_positions[channel].x;
            Y_coordinate = HSDetection::Detection::channel_positions[channel].y;
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
                X = HSDetection::Detection::channel_positions[channel].x;
                Y = HSDetection::Detection::channel_positions[channel].y;
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
