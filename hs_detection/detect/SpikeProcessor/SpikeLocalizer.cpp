#include "SpikeLocalizer.h"
#include <utility>
#include <vector>
#include <algorithm>
#include "../Detection.h"
#include "../Point.h"
#include "../Utils.h"

using namespace std;

namespace HSDetection
{
    void SpikeLocalizer::operator()(Spike *pSpike)
    {
        // TODO: generate waveform here???
        Utils::storeCOMWaveformsCounts(pSpike->waveforms, pSpike->frame, pSpike->channel);
        vector<vector<pair<int, int>>> *waveforms = &pSpike->waveforms;

        Point sumCoM(0, 0);
        int sumWeight = 0;

        for (int i = 0; i < Detection::num_com_centers; i++)
        {
            vector<pair<int, int>> chAmp = (*waveforms)[i];
            int chCount = chAmp.size();

            // // compute median, threshold at median
            int median;
            {
                // TODO: extract as a class?
                vector<pair<int, int>>::iterator mid = chAmp.begin() + (chCount - 1) / 2;
                nth_element(chAmp.begin(), mid, chAmp.end(),
                            [](const pair<int, int> &lhs, const pair<int, int> &rhs)
                            { return lhs.second < rhs.second; });
                median = mid->second;
                if (chCount % 2 == 0)
                {
                    int next = min_element(mid + 1, chAmp.end(),
                                           [](const pair<int, int> &lhs, const pair<int, int> &rhs)
                                           { return lhs.second < rhs.second; })
                                   ->second;
                    median = (median + next) / 2;
                }
            }

            Point CoM(0, 0);
            int sumAmp = 0;
            for (int i = 0; i < chCount; i++)
            {
                int amp = chAmp[i].second - median; // // Correct amplitudes (threshold)
                if (amp > 0)
                {
                    CoM += amp * Detection::probeLayout.getChannelPosition(chAmp[i].first);
                    sumAmp += amp;
                }
            }

            if (sumAmp == 0)
            {
                // NOTE: amp > 0 never entered, all <= median, i.e. max <= median
                // NOTE: this iff. max == median, i.e. upper half value all same
                // NOTE: unlikely happens, therefore loop again instead of merge into previous
                // TODO: really need?
                for (int i = 0; i < chCount; i++)
                {
                    if (chAmp[i].second == median) // NOTE: choose any median == max
                    {
                        CoM = Detection::probeLayout.getChannelPosition(chAmp[i].first);
                        break;
                    }
                }
            }
            else
            {
                CoM /= sumAmp;
            }

            sumCoM += 1 * CoM; // TODO: weight?
            sumWeight += 1;    // TODO: inc amount?
        }

        pSpike->position = sumCoM / sumWeight;
    }

} // namespace HSDetection
