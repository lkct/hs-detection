#include <algorithm>

#include "SpikeLocalizer.h"

using namespace std;

namespace HSDetection
{
    SpikeLocalizer::SpikeLocalizer(const ProbeLayout *pLayout, const TraceWrapper *pTrace,
                                   const TraceWrapper *pRef, const RollingArray *pBaseline,
                                   IntFrame jitterTol, IntFrame peakDur)
        : pLayout(pLayout), pTrace(pTrace), pRef(pRef), pBaseline(pBaseline),
          jitterTol(jitterTol), peakDur(peakDur) {}

    SpikeLocalizer::~SpikeLocalizer() {}

    void SpikeLocalizer::operator()(Spike *pSpike)
    {
        IntFrame frameLeft = pSpike->frame - jitterTol; // TODO: name?
        IntFrame frameRight = pSpike->frame + jitterTol;

        IntVolt ref = (*pRef)(pSpike->frame, 0); // TODO: why not -peakDur
        const IntVolt *baselines = (*pBaseline)[pSpike->frame - peakDur];

        vector<pair<IntChannel, IntFxV>> chAmps;

        for (IntChannel neighborChannel : pLayout->getInnerNeighbors(pSpike->channel))
        {
            IntVolt baseline = ref + baselines[neighborChannel];
            IntFxV sumVolt = 0;
            for (IntFrame t = frameLeft; t < frameRight; t++) // TODO: why not <=
            {
                IntVolt volt = (*pTrace)(t, neighborChannel) - baseline;
                if (volt > 0)
                {
                    sumVolt += volt;
                }
            }
            chAmps.push_back(make_pair(neighborChannel, sumVolt));
        }

        IntFxV median;
        {
            // TODO: extract as a class?
            vector<pair<IntChannel, IntFxV>>::iterator middle = chAmps.begin() + (chAmps.size() - 1) / 2;
            nth_element(chAmps.begin(), middle, chAmps.end(),
                        [](const pair<IntChannel, IntFxV> &lhs, const pair<IntChannel, IntFxV> &rhs)
                        { return lhs.second < rhs.second; });
            median = middle->second;
            if (chAmps.size() % 2 == 0)
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
            pSpike->position = pLayout->getChannelPosition(find_if(chAmps.begin(), chAmps.end(),
                                                                   [median](const pair<IntChannel, IntFxV> &chAmp)
                                                                   { return chAmp.second == median; })
                                                               ->first);
        }
        else
        {
            pSpike->position = CoM /= sumAmp;
        }
    }

} // namespace HSDetection
