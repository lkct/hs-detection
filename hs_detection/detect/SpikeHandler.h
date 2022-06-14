#ifndef SPIKEHANDLER_H
#define SPIKEHANDLER_H

#include <vector>
#include <tuple>
#include "Spike.h"

namespace SpikeHandler
{

    void setInitialParameters();
    void setLocalizationParameters(int _aGlobal, int **_baselines, int _index_baselines);
    void addSpike(int channel, int frame, int amplitude);
    void terminateSpikeHandler();
    // Inner neighbor creation methods
    float channelsDist(int start_channel, int end_channel);
    void fillNeighborLayerMatrices();
    std::vector<int> getInnerNeighborsRadius(std::vector<std::tuple<int, float>> distances_neighbors, int central_channel);
    int **createInnerNeighborMatrix();
    int **createOuterNeighborMatrix();
    Spike storeWaveformCutout(int cutout_size, Spike curr_spike);
    Spike storeCOMWaveformsCounts(Spike curr_spike);

};

#endif
