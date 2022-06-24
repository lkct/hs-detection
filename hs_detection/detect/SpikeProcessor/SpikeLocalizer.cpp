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

        vector<pair<IntChannel, IntMax>> chAmps;

        for (IntChannel neighborChannel : pLayout->getInnerNeighbors(pSpike->channel))
        {
            IntVolt baseline = ref + baselines[neighborChannel];
            IntMax sumVolt = 0;
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

        IntMax median;
        {
            // TODO: extract as a class?
            vector<pair<IntChannel, IntMax>>::iterator middle = chAmps.begin() + (chAmps.size() - 1) / 2;
            nth_element(chAmps.begin(), middle, chAmps.end(),
                        [](const pair<IntChannel, IntMax> &lhs, const pair<IntChannel, IntMax> &rhs)
                        { return lhs.second < rhs.second; });
            median = middle->second;
            if (chAmps.size() % 2 == 0)
            {
                IntMax med1 = min_element(middle + 1, chAmps.end(),
                                          [](const pair<IntChannel, IntMax> &lhs, const pair<IntChannel, IntMax> &rhs)
                                          { return lhs.second < rhs.second; })
                                  ->second;
                median = (median + med1) / 2;
            }
        }

        Point CoM(0, 0);
        FloatGeom sumAmp = 0;
        for (const pair<IntChannel, IntMax> &chAmp : chAmps)
        {
            IntMax amp = chAmp.second - median; // correction and threshold
            if (amp >= 0)
            {
                CoM += (amp + eps) * pLayout->getChannelPosition(chAmp.first);
                sumAmp += amp + eps;
            }
        }

        pSpike->position = CoM /= sumAmp;
    }

} // namespace HSDetection
