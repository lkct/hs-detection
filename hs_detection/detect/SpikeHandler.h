#ifndef SPIKEHANDLER_H
#define SPIKEHANDLER_H

#include <vector>
#include <tuple>
#include "Spike.h"

namespace SpikeHandler
{

    // Inner neighbor creation methods
    void fillNeighborLayerMatrices();
    std::vector<int> getInnerNeighborsRadius(std::vector<std::tuple<int, float>> distances_neighbors, int central_channel);
    int **createInnerNeighborMatrix();
    int **createOuterNeighborMatrix();
    Spike storeWaveformCutout(int cutout_size, Spike curr_spike);
    Spike storeCOMWaveformsCounts(Spike curr_spike);

};

#endif
