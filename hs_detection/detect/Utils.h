#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <utility>
#include "Spike.h"

using HSDetection::Spike;

namespace Utils
{

    // Inner neighbor creation methods
    void fillNeighborLayerMatrices();

    int **createInnerNeighborMatrix();
    int **createOuterNeighborMatrix();

    Spike storeWaveformCutout(int cutout_size, Spike curr_spike);
    Spike storeCOMWaveformsCounts(Spike curr_spike);

    bool areNeighbors(int channel_one, int channel_two);

};

#endif
