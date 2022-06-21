#include <algorithm>

#include "SpikeLocalizer.h"

using namespace std;

namespace HSDetection
{
    SpikeLocalizer::SpikeLocalizer(ProbeLayout *pLayout, TraceWrapper *pTrace,
                                   TraceWrapper *pRef, RollingArray *pBaseline,
                                   IntChannel numCoMCenters, IntFrame noiseDuration, IntFrame spikePeakDuration)
        : pLayout(pLayout), pTrace(pTrace), pRef(pRef), pBaseline(pBaseline),
          numCoMCenters(numCoMCenters), noiseDuration(noiseDuration), spikePeakDuration(spikePeakDuration) {}

    SpikeLocalizer::~SpikeLocalizer() {}

    void SpikeLocalizer::operator()(Spike *pSpike)
    {
        Point sumCoM(0, 0);
        IntChannel sumWeight = 0;

        IntFrame frameLeft = pSpike->frame - noiseDuration; // TODO: name?
        IntFrame frameRight = pSpike->frame + noiseDuration;

        vector<pair<IntChannel, IntFxV>> chAmps;

        IntVolt ref = (*pRef)(pSpike->frame, 0);
        const IntVolt *baselines = (*pBaseline)[pSpike->frame - spikePeakDuration];

        for (IntChannel i = 0; i < numCoMCenters; i++)
        {
            chAmps.clear();

            IntChannel centerChannel = pLayout->getInnerNeighbors(pSpike->channel)[i];

            for (IntChannel neighborChannel : pLayout->getInnerNeighbors(centerChannel))
            {
                IntVolt baseline = ref + baselines[neighborChannel];
                IntFxV sumVolt = 0;
                for (IntFrame t = frameLeft; t < frameRight; t++)
                {
                    IntVolt volt = (*pTrace)(t, neighborChannel) - baseline;
                    if (volt > 0)
                    {
                        sumVolt += volt;
                    }
                }
                chAmps.push_back(make_pair(neighborChannel, sumVolt));
            }

            IntChannel chCount = pLayout->getInnerNeighbors(centerChannel).size();

            IntFxV median;
            {
                // TODO: extract as a class?
                vector<pair<IntChannel, IntFxV>>::iterator middle = chAmps.begin() + (chCount - 1) / 2;
                nth_element(chAmps.begin(), middle, chAmps.end(),
                            [](const pair<IntChannel, IntFxV> &lhs, const pair<IntChannel, IntFxV> &rhs)
                            { return lhs.second < rhs.second; });
                median = middle->second;
                if (chCount % 2 == 0)
                {
                    IntFxV med1 = min_element(middle + 1, chAmps.end(),
                                              [](const pair<IntChannel, IntFxV> &lhs, const pair<IntChannel, IntFxV> &rhs)
                                              { return lhs.second < rhs.second; })
                                      ->second;
                    median = (median + med1) / 2;
                }
            }

            Point CoM(0, 0);
            IntFCV sumAmp = 0;
            for (const pair<IntChannel, IntFxV> &chAmp : chAmps)
            {
                IntFxV amp = chAmp.second - median; // correction and threshold
                if (amp > 0)
                {
                    CoM += amp * pLayout->getChannelPosition(chAmp.first);
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
                CoM = pLayout->getChannelPosition(find_if(chAmps.begin(), chAmps.end(),
                                                          [median](const pair<IntChannel, IntFxV> &chAmp)
                                                          { return chAmp.second == median; })
                                                      ->first);
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
