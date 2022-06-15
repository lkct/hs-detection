#include "LocalizeSpikes.h"
#include <iostream>
#include <algorithm>
#include "Detection.h"

using namespace std;

namespace LocalizeSpikes
{

    struct CustomLessThan
    {
        bool operator()(pair<int, int> const &lhs,
                        pair<int, int> const &rhs) const
        {
            return lhs.second < rhs.second;
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
           position: pair<float, float>
           An X and Y coordinate pair that corresponds to where the spike occurred.
         */

        vector<vector<pair<int, int>>> *waveforms = &spike_to_be_localized.waveforms;

        Point sumCoM(0, 0);
        int sumWeight = 0;

        for (int i = 0; i < HSDetection::Detection::num_com_centers; i++)
        {
            vector<pair<int, int>> chAmp = (*waveforms)[i];
            int chCount = chAmp.size();

            // // compute median, threshold at median
            nth_element(chAmp.begin(), chAmp.begin() + (chCount - 1) / 2, chAmp.end(), CustomLessThan());
            int correct = chAmp[(chCount - 1) / 2].second;
            if (chCount % 2 == 0)
            {
                correct = (min_element(chAmp.begin() + chCount / 2, chAmp.end(), CustomLessThan())->second + correct) / 2;
            }

            Point com(0.0f, 0.0f);
            int sumAmp = 0;

            for (int i = 0; i < chCount; i++)
            {
                int ch = chAmp[i].first;
                int amp = chAmp[i].second - correct; // // Correct amplitudes (threshold)
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
                for (int i = 0; i < chCount; i++)
                {
                    int ch = chAmp[i].first;
                    int amp = chAmp[i].second - correct;
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

            sumCoM += 1 * com; // TODO: weight?
            sumWeight += 1;    // TODO: inc amount?
        }

        // TODO: div 0 check???
        return sumCoM /= sumWeight;
    }

}
