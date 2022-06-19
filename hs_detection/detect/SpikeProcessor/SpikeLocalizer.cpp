#include <algorithm>
#include <numeric>

#include "SpikeLocalizer.h"

using namespace std;

namespace HSDetection
{
    SpikeLocalizer::SpikeLocalizer(ProbeLayout *pLayout, TraceWrapper *pTrace,
                                   TraceWrapper *pAGlobal, RollingArray *pBaseline,
                                   int numCoMCenters, int noiseDuration, int spikePeakDuration)
        : pLayout(pLayout), pTrace(pTrace), pAGlobal(pAGlobal), pBaseline(pBaseline),
          numCoMCenters(numCoMCenters), noiseDuration(noiseDuration), spikePeakDuration(spikePeakDuration) {}

    SpikeLocalizer::~SpikeLocalizer() {}

    void SpikeLocalizer::operator()(Spike *pSpike)
    {
        Point sumCoM(0, 0);
        int sumWeight = 0;

        int frameLeft = pSpike->frame - noiseDuration;
        int frameRight = pSpike->frame + noiseDuration;
        // TODO: assert (cutout_start >= noise_duration && cutout_end >= noise_duration)

        vector<pair<int, int>> chAmp;

        int aGlobal = (*pAGlobal)(pSpike->frame, 0);
        const short *baselines = (*pBaseline)[pSpike->frame - spikePeakDuration]; // TODO: tSpike > peakDur?

        for (int i = 0; i < numCoMCenters; i++)
        {
            chAmp.clear();

            // TODO: check i in inner neighbor range (numCoM not too large)
            int centerChannel = pLayout->getInnerNeighbors(pSpike->channel)[i];

            for (int neighborChannel : pLayout->getInnerNeighbors(centerChannel))
            {
                int offset = aGlobal + baselines[neighborChannel];
                int sum = 0;
                for (int t = frameLeft; t < frameRight; t++)
                {
                    int a = (*pTrace)(t, neighborChannel) - offset;
                    if (a > 0)
                    {
                        sum += a;
                    }
                }
                chAmp.push_back(make_pair(neighborChannel, sum));
            }

            int chCount = pLayout->getInnerNeighbors(centerChannel).size();

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
                    CoM += amp * pLayout->getChannelPosition(chAmp[i].first);
                    sumAmp += amp;
                }
            }

            if (sumAmp == 0)
            {
                // NOTE: amp > 0 never entered, all <= median, i.e. max <= median
                // NOTE: this iff. max == median, i.e. upper half value all same
                // NOTE: unlikely happens, therefore loop again instead of merge into previous
                // NOTE: choose any point with amp == median == max
                // TODO: really need? choose any?
                vector<pair<int, int>>::iterator it = find_if(chAmp.begin(), chAmp.end(),
                                                              [median](const pair<int, int> &x)
                                                              { return x.second == median; });
                CoM = pLayout->getChannelPosition(it->first);
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
