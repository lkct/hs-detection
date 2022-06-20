#include <algorithm>

#include "ProbeLayout.h"

using namespace std;

namespace HSDetection
{
    ProbeLayout::ProbeLayout(int numChannels, float *channelPositions,
                             float neighborRadius, float innerRadius)
        : positions(numChannels), distances(numChannels, vector<float>(numChannels)),
          neighborList(numChannels), innerNeighborList(numChannels),
          neighborRadius(neighborRadius), innerRadius(innerRadius)
    {
        for (int i = 0; i < numChannels; i++)
        {
            positions[i].x = (int)channelPositions[i * 2]; // TODO: ???
            positions[i].y = (int)channelPositions[i * 2 + 1];
        }

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
