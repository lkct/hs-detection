#include "SpikeLocalizer.h"
#include <utility>
#include <vector>
#include <algorithm>
#include "../Detection.h"
#include "../Point.h"

using namespace std;

namespace HSDetection
{

    void storeCOMWaveformsCounts(vector<vector<pair<int, int>>> &chAmps, int frame, int channel)
    {
        int num_com_centers = Detection::num_com_centers;
        ProbeLayout &probe = Detection::probeLayout;
        int noise_duration = Detection::noise_duration;

        chAmps = vector<vector<pair<int, int>>>(num_com_centers, vector<pair<int, int>>());

        // Get closest channels for COM
        for (int i = 0; i < num_com_centers; i++)
        {
            int max_channel = probe.getInnerNeighbors(channel)[i];
            // TODO: check i in innerneighbor range (numCoM not too large)

            const vector<int> &neib = probe.getInnerNeighbors(max_channel);

            for (int j = 0; j < (int)neib.size(); j++) // -Wsign-compare
            {
                int curr_neighbor_channel = neib[j];

                // TODO: assert (cutout_start >= noise_duration && cutout_end >= noise_duration)

                int sum = 0;
                for (int t = frame - noise_duration; t < frame + noise_duration; t++)
                {
                    int curr_reading = Detection::trace(t, curr_neighbor_channel);
                    int curr_amp = curr_reading - Detection::AGlobal(frame, 0) -
                                   Detection::QBs(
                                       frame - Detection::spike_peak_duration, // TODO: tSpike > peakDur?
                                       curr_neighbor_channel);
                    if (curr_amp >= 0)
                    {
                        sum += curr_amp;
                    }
                }
                chAmps[i].push_back(make_pair(curr_neighbor_channel, sum));
            }
        }
    }

    void SpikeLocalizer::operator()(Spike *pSpike)
    {
        // TODO: generate waveform here???
        vector<vector<pair<int, int>>> waveforms;
        storeCOMWaveformsCounts(waveforms, pSpike->frame, pSpike->channel);

        Point sumCoM(0, 0);
        int sumWeight = 0;

        for (int i = 0; i < Detection::num_com_centers; i++)
        {
            vector<pair<int, int>> &chAmp = waveforms[i];
            int chCount = chAmp.size();

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
                int amp = chAmp[i].second - median; // correction and threshold
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
