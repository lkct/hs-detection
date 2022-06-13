#ifndef FILTERSPIKES_H
#define FILTERSPIKES_H

#include "Spike.h"

namespace FilterSpikes
{

    Spike filterSpikesDecay(Spike largest_amp_spike);
    Spike filterSpikesAll(Spike largest_amp_spike);
    void filterAllNeighbors(Spike max_spike);
    Spike findMaxSpikeNeighbor(Spike first_spike);
    void filterOuterNeighbors(Spike max_spike);
    bool filteredOuterSpike(Spike outer_spike, Spike max_spike);
    int getClosestInnerNeighborChannel(int outer_channel, int central_channel);
    Spike getSpikeFromChannel(int channel);
    void filterInnerNeighbors(Spike max_spike);
    float channelsDist(int start_channel, int end_channel);
    bool isInnerNeighbor(int central_channel, int curr_channel);
    bool areNeighbors(int channel_one, int channel_two);
};

#endif
