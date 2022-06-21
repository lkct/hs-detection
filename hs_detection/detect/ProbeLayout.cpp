#include <algorithm>

#include "ProbeLayout.h"

using namespace std;

namespace HSDetection
{
    ProbeLayout::ProbeLayout(IntChannel numChannels, FloatGeom *channelPositions,
                             FloatGeom neighborRadius, FloatGeom innerRadius)
        : positions(numChannels), distances(numChannels, vector<FloatGeom>(numChannels)),
          neighborList(numChannels), innerNeighborList(numChannels),
          neighborRadius(neighborRadius), innerRadius(innerRadius)
    {
        for (IntChannel i = 0; i < numChannels; i++)
        {
            positions[i].x = (int)channelPositions[i * 2]; // TODO: int cast???
            positions[i].y = (int)channelPositions[i * 2 + 1];
        }

        for (IntChannel i = 0; i < numChannels; i++)
        {
            for (IntChannel j = 0; j < numChannels; j++)
            {
                FloatGeom dis = distances[i][j] = (positions[i] - positions[j]).abs();
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
                 [&dist = distances[i]](IntChannel lhs, IntChannel rhs)
                 { return dist[lhs] < dist[rhs]; });
        }
    }

    ProbeLayout::~ProbeLayout() {}

} // namespace HSDetection
