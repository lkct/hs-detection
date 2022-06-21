#include <algorithm>

#include "SpikeLocalizer.h"

using namespace std;

namespace HSDetection
{
    SpikeLocalizer::SpikeLocalizer(ProbeLayout *pLayout, TraceWrapper *pTrace,
                                   TraceWrapper *pAGlobal, RollingArray *pBaseline,
                                   IntChannel numCoMCenters, IntFrame noiseDuration, IntFrame spikePeakDuration)
        : pLayout(pLayout), pTrace(pTrace), pAGlobal(pAGlobal), pBaseline(pBaseline),
          numCoMCenters(numCoMCenters), noiseDuration(noiseDuration), spikePeakDuration(spikePeakDuration) {}

    SpikeLocalizer::~SpikeLocalizer() {}

    void SpikeLocalizer::operator()(Spike *pSpike)
    {
        Point sumCoM(0, 0);
        IntChannel sumWeight = 0;

        IntFrame frameLeft = pSpike->frame - noiseDuration;
        IntFrame frameRight = pSpike->frame + noiseDuration;

        vector<pair<IntChannel, IntFxV>> chAmp;

        IntVolt aGlobal = (*pAGlobal)(pSpike->frame, 0);
        const IntVolt *baselines = (*pBaseline)[pSpike->frame - spikePeakDuration];

        for (IntChannel i = 0; i < numCoMCenters; i++)
        {
            chAmp.clear();

            IntChannel centerChannel = pLayout->getInnerNeighbors(pSpike->channel)[i];

            for (IntChannel neighborChannel : pLayout->getInnerNeighbors(centerChannel))
            {
                IntVolt offset = aGlobal + baselines[neighborChannel];
                IntFxV sum = 0;
                for (IntFrame t = frameLeft; t < frameRight; t++)
                {
                    IntVolt a = (*pTrace)(t, neighborChannel) - offset;
                    if (a > 0)
                    {
                        sum += a;
                    }
                }
                chAmp.push_back(make_pair(neighborChannel, sum));
            }

            IntChannel chCount = pLayout->getInnerNeighbors(centerChannel).size();

            IntFxV median;
            {
                // TODO: extract as a class?
                vector<pair<IntChannel, IntFxV>>::iterator mid = chAmp.begin() + (chCount - 1) / 2;
                nth_element(chAmp.begin(), mid, chAmp.end(),
                            [](const pair<IntChannel, IntFxV> &lhs, const pair<IntChannel, IntFxV> &rhs)
                            { return lhs.second < rhs.second; });
                median = mid->second;
                if (chCount % 2 == 0)
                {
                    IntFxV next = min_element(mid + 1, chAmp.end(),
                                              [](const pair<IntChannel, IntFxV> &lhs, const pair<IntChannel, IntFxV> &rhs)
                                              { return lhs.second < rhs.second; })
                                      ->second;
                    median = (median + next) / 2;
                }
            }

            Point CoM(0, 0);
            IntFCV sumAmp = 0;
            for (IntChannel i = 0; i < chCount; i++)
            {
                IntFxV amp = chAmp[i].second - median; // correction and threshold
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
                vector<pair<IntChannel, IntFxV>>::iterator it = find_if(chAmp.begin(), chAmp.end(),
                                                                        [median](const pair<IntChannel, IntFxV> &x)
                                                                        { return x.second == median; });
                CoM = pLayout->getChannelPosition(it->first);
            }
            else
            {
                CoM /= sumAmp;
            }

            sumCoM += 1 * CoM; // TODO: weight?
            sumWeight += 1;
        }

        pSpike->position = sumCoM / sumWeight;
    }

} // namespace HSDetection
