#ifndef SPIKEHANDLER_H
#define SPIKEHANDLER_H

#include <string>
#include <vector>
#include <tuple>
#include "Spike.h"

namespace SpikeHandler
{

    void setInitialParameters(int _num_channels, int _spike_peak_duration, std::string file_name,
                              int _noise_duration, float _noise_amp_percent, float _inner_radius, float **_channel_positions, int **_neighbor_matrix,
                              int _max_neighbors, int _num_com_centers, bool _to_localize, int _cutout_start, int _cutout_end, int _maxsl,
                              bool _decay_filtering);
    void loadRawData(short *_raw_data, int _index_data, int _iterations, int maxFramesProcessed, int before_chunk, int after_chunk);
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
