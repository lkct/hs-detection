#ifndef SPIKE_H
#define SPIKE_H

#include <vector>

// // Internal representation of a spike. User has no need to use it.
class Spike
{
public:
    int channel;
    int frame;
    int amplitude;
    std::vector<int> largest_channels;
    std::vector<int> written_cutout;
    std::vector<std::vector<int>> waveforms;
    std::vector<int> neighbor_counts;

    int aGlobal;                // Global noise
    std::vector<int> baselines; // Contains spike_delay number of frames of median baseline values. Updated by user at every frame.

    Spike(int frame, int channel, int amplitude)
        : channel(channel), frame(frame), amplitude(amplitude) {}
    ~Spike() {}
};

#endif
