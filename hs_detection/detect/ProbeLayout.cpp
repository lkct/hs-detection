#include "ProbeLayout.h"

using namespace std;

namespace HSDetection
{
    ProbeLayout::ProbeLayout() {} // TODO: remove this when not static

    ProbeLayout::ProbeLayout(int numChannels, int *channelPositions,
                             float neighborRadius, float innerRadius)
        : numChannels(numChannels), positions(numChannels),
          neighborList(numChannels), innerNeighborList(numChannels)
    {
        for (int i = 0; i < numChannels; i++)
        {
            positions[i].x = channelPositions[i * 2];
            positions[i].y = channelPositions[i * 2 + 1];
        }

        // TODO: opportunity to vectorize?

        vector<float> distance(numChannels);

        for (int i = 0; i < numChannels; i++)
        {
            for (int j = 0; j < numChannels; j++)
            {
                float dis = distance[j] = channelDistance(i, j);
                if (dis < neighborRadius)
                {
                    neighborList[i].push_back(j);
                    if (dis < innerRadius) // TODO: <=?
                    {
                        innerNeighborList[i].push_back(j);
                    }
                }
            }

            // inner neighbors are sorted, with self at the first
            sort(innerNeighborList[i].begin(), innerNeighborList[i].end(),
                 [&distance](int lhs, int rhs)
                 { return distance[lhs] < distance[rhs]; });
        }
    }

    ProbeLayout::~ProbeLayout() {}

} // namespace HSDetection
