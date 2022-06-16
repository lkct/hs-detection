#ifndef PROBELAYOUT_H
#define PROBELAYOUT_H

#include <algorithm>
#include <vector>

#include "Point.h"

namespace HSDetection
{
    class ProbeLayout
    {
    private:
        int numChannels;
        std::vector<Point> positions;
        std::vector<std::vector<int>> neighborList;
        std::vector<std::vector<int>> innerNeighborList;

        float channelDistance(int channel1, int channel2) const { return (positions[channel1] - positions[channel2]).abs(); }

    public:
        ProbeLayout();                                      // TODO: remove this when not static
        ProbeLayout(int numChannels, int *channelPositions, // TODO: pass in float position?
                    float neighborRadius, float innerRadius);
        ~ProbeLayout();

        const Point &getChannelPosition(int channel) const { return positions[channel]; }
        const std::vector<int> &getInnerNeighbors(int channel) const { return innerNeighborList[channel]; }
        bool areNeighbors(int channel1, int channel2) const
        {
            const std::vector<int> &neighbors = neighborList[channel1];
            return std::find(neighbors.begin(), neighbors.end(), channel2) != neighbors.end();
        }
    };

} // namespace HSDetection

#endif
