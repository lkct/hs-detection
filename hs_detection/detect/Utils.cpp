#include "Utils.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include "Detection.h"
#include "ProbeLayout.h"

using namespace std;

namespace Utils
{

    Spike storeCOMWaveformsCounts(Spike curr_spike)
    {
        int num_com_centers = HSDetection::Detection::num_com_centers;
        HSDetection::ProbeLayout &probe = HSDetection::Detection::probeLayout;
        int noise_duration = HSDetection::Detection::noise_duration;

        vector<vector<pair<int, int>>> chAmps(num_com_centers, vector<pair<int, int>>());

        // Get closest channels for COM
        for (int i = 0; i < num_com_centers; i++)
        {
            int max_channel = probe.getInnerNeighbors(curr_spike.channel)[i];
            // TODO: check i in innerneighbor range (numCoM not too large)

            const vector<int> & neib = probe.getInnerNeighbors(max_channel);

            for (int j = 0; j < (int)neib.size(); j++) // -Wsign-compare
            {
                int curr_neighbor_channel = neib[j];

                // TODO: assert (cutout_start >= noise_duration && cutout_end >= noise_duration)

                int sum = 0;
                for (int t = curr_spike.frame - noise_duration; t < curr_spike.frame + noise_duration; t++)
                {
                    int curr_reading = HSDetection::Detection::trace(t, curr_neighbor_channel);
                    int curr_amp = curr_reading - HSDetection::Detection::AGlobal(curr_spike.frame, 0) -
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

}
