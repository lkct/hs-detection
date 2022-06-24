#include <algorithm>

#include "SpikeLocalizer.h"

using namespace std;

namespace HSDetection
{
    SpikeLocalizer::SpikeLocalizer(const ProbeLayout *pLayout, const RollingArray *pTrace,
                                   const RollingArray *pRef, const RollingArray *pBaseline,
                                   IntFrame jitterTol, IntFrame peakDur)
        : pLayout(pLayout), pTrace(pTrace), pRef(pRef), pBaseline(pBaseline),
          jitterTol(jitterTol), peakDur(peakDur) {}

    SpikeLocalizer::~SpikeLocalizer() {}

    void SpikeLocalizer::operator()(Spike *pSpike)
    {
        const vector<IntChannel> &neighbors = pLayout->getInnerNeighbors(pSpike->channel);
        int numNeighbors = neighbors.size();

        vector<IntMax> weights(numNeighbors);
        for (int i = 0; i < numNeighbors; i++)
        {
            weights[i] = sumCutout(pSpike->frame, neighbors[i]);
        }

        IntMax median = getMedian(weights);

        Point sumPoint(0, 0);
        FloatGeom sumWeight = 0;
        for (int i = 0; i < numNeighbors; i++)
        {
            IntMax weight = weights[i] - median; // correction and threshold on median
            if (weight >= 0)
            {
                sumPoint += (weight + eps) * pLayout->getChannelPosition(neighbors[i]);
                sumWeight += weight + eps;
            }
        }

        pSpike->position = sumPoint / sumWeight;
    }

    IntMax SpikeLocalizer::sumCutout(IntFrame frame, IntChannel channel) const
    {
        IntVolt baseline = (*pBaseline)[frame - peakDur][channel]; // baseline at the start of event

        IntMax sum = 0;
        for (IntFrame t = frame - jitterTol; t < frame + jitterTol; t++) // TODO:??? <=
        {
            IntVolt volt = (*pTrace)(t, channel) - baseline - (*pRef)(frame, 0); // TODO: shoule be ref(t)
            if (volt > 0)
            {
                sum += volt;
            }
        }
        return sum;
    }

    IntMax SpikeLocalizer::getMedian(vector<IntMax> weights) const // copy param to be modified inside
    {
        vector<IntMax>::iterator middle = weights.begin() + weights.size() / 2;
        nth_element(weights.begin(), middle, weights.end());
        if (weights.size() % 2 == 0)
        {
            return (*middle + *max_element(weights.begin(), middle)) / 2;
        }
        else
        {
            return *middle;
        }
    }

} // namespace HSDetection
