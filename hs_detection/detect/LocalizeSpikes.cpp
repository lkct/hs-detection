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
            int max_channel = (*largest_channels)[i];
            vector<tuple<int, int>> amps;

            for (int j = 0; j < neighbor_count; j++)
            {
                int curr_neighbor_channel = HSDetection::Detection::inner_neighbor_matrix[max_channel][j];
                amps.push_back(make_tuple(curr_neighbor_channel, (*waveforms)[i][j]));
            }

            // // compute median, threshold at median
            nth_element(amps.begin(), amps.begin() + (neighbor_count - 1) / 2, amps.end(), CustomLessThan());
            int correct = get<1>(amps[(neighbor_count - 1) / 2]);
            if (neighbor_count % 2 == 0)
            {
                correct = (get<1>(*min_element(amps.begin() + neighbor_count / 2, amps.end(), CustomLessThan())) + correct) / 2;
            }

            Point com(0.0f, 0.0f);
            int sumAmp = 0;

            for (int i = 0; i < neighbor_count; i++)
            {
                int ch = get<0>(amps[i]);
                int amp = get<1>(amps[i]) - correct; // // Correct amplitudes (threshold)
                if (amp > 0)
                {
                    com += amp * HSDetection::Detection::channel_positions[ch];
                    sumAmp += amp;
                }
            }

            if (sumAmp == 0)
            {
                // NOTE: apm > 0 not entered, all <= correct, i.e. max <= median
                // NOTE: this iff. max == median, i.e. upper half value all same
                // NOTE: unlikely, therefore loop again instead of merge into previous
                // TODO: really need?
                for (int i = 0; i < neighbor_count; i++)
                {
                    int ch = get<0>(amps[i]);
                    int amp = get<1>(amps[i]) - correct;
                    if (amp == 0) // NOTE: choose any median == max
                    {
                        com = HSDetection::Detection::channel_positions[ch];
                        break;
                    }
                }
            }
            else
            {
                com /= sumAmp;
            }

            sumPos += 1 * com; // TODO: weight?
            sumWeight += 1;    // TODO: inc amount?
        }

        // TODO: div 0 check???
        return sumPos /= sumWeight;
    }

}
