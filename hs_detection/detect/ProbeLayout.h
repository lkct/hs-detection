#ifndef PROBELAYOUT_H
#define PROBELAYOUT_H

#include <vector>

#include "Point.h"

namespace HSDetection
{
    class ProbeLayout
    {
    private:
        std::vector<Point> positions;              // channel positions, NxXY
        std::vector<std::vector<float>> distances; // channel distances, NxN

        std::vector<std::vector<int>> neighborList;      // adjacency list sorted by channel for neighbors, contains self, NxVariable
        std::vector<std::vector<int>> innerNeighborList; // adjacency list sorted by distance for inner neighbors, contains self, NxVariable

        float neighborRadius;
        float innerRadius;

    public:
        ProbeLayout(int numChannels, float *channelPositions,
                    float neighborRadius, float innerRadius);
        ~ProbeLayout();

        // copy constructor deleted to avoid copy of containers
        ProbeLayout(const ProbeLayout &) = delete;
        // copy assignment deleted to avoid copy of containers
        ProbeLayout &operator=(const ProbeLayout &) = delete;

        const Point &getChannelPosition(int channel) const { return positions[channel]; }

        float getChannelDistance(int channel1, int channel2) const { return distances[channel1][channel2]; }

        const std::vector<int> &getNeighbors(int channel) const { return neighborList[channel]; }
        const std::vector<int> &getInnerNeighbors(int channel) const { return innerNeighborList[channel]; }

        bool areNeighbors(int channel1, int channel2) const { return getChannelDistance(channel1, channel2) < neighborRadius; }
        bool areInnerNeighbors(int channel1, int channel2) const { return getChannelDistance(channel1, channel2) < innerRadius; } // TODO: <=?
    };

} // namespace HSDetection

#endif
