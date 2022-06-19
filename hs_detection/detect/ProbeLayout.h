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
        std::vector<Point> positions;

        std::vector<std::vector<int>> neighborList;      // adjacency list sorted by channel for neighbors, contains self
        std::vector<std::vector<int>> innerNeighborList; // adjacency list sorted by distance for inner neighbors, contains self

    public:
        ProbeLayout(int numChannels, int *channelPositions, // TODO: pass in float position?
                    float neighborRadius, float innerRadius);
        ~ProbeLayout();

        // copy constructor deleted to avoid copy of containers
        ProbeLayout(const ProbeLayout &) = delete;
        // copy assignment deleted to avoid copy of containers
        ProbeLayout &operator=(const ProbeLayout &) = delete;

        const Point &getChannelPosition(int channel) const { return positions[channel]; }

        float channelDistance(int channel1, int channel2) const { return (positions[channel1] - positions[channel2]).abs(); }

        const std::vector<int> &getNeighbors(int channel) const { return neighborList[channel]; }
        const std::vector<int> &getInnerNeighbors(int channel) const { return innerNeighborList[channel]; }

        // TODO: or calc channel distance?
        bool areNeighbors(int channel1, int channel2) const
        {
            const std::vector<int> &neighbors = neighborList[channel1];
            return std::binary_search(neighbors.begin(), neighbors.end(), channel2);
        }
        bool areInnerNeighbors(int channel1, int channel2) const
        {
            const std::vector<int> &neighbors = innerNeighborList[channel1];
            return std::find(neighbors.begin(), neighbors.end(), channel2) != neighbors.end();
        }
    };

} // namespace HSDetection

#endif
