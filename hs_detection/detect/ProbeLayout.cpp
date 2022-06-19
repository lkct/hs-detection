#include "ProbeLayout.h"

using namespace std;

namespace HSDetection
{
    ProbeLayout::ProbeLayout(int numChannels, int *channelPositions,
                             float neighborRadius, float innerRadius)
        : positions(numChannels), distances(numChannels, vector<float>(numChannels)),
          neighborList(numChannels), innerNeighborList(numChannels),
          neighborRadius(neighborRadius), innerRadius(innerRadius)
    {
        for (int i = 0; i < numChannels; i++)
        {
            positions[i].x = channelPositions[i * 2];
            positions[i].y = channelPositions[i * 2 + 1];
        }

        // TODO: opportunity to vectorize?
        for (int i = 0; i < numChannels; i++)
        {
            for (int j = 0; j < numChannels; j++)
            {
                float dis = distances[i][j] = (positions[i] - positions[j]).abs();
                if (dis < neighborRadius)
                {
                    neighborList[i].push_back(j);
                    if (dis < innerRadius) // TODO: <=?
                    {
                        innerNeighborList[i].push_back(j);
                    }
                }
            }

            // self definitely sorted to the first
            sort(innerNeighborList[i].begin(), innerNeighborList[i].end(),
                 [&dist = distances[i]](int lhs, int rhs)
                 { return dist[lhs] < dist[rhs]; });
        }
    }

    ProbeLayout::~ProbeLayout() {}

} // namespace HSDetection
