#ifndef SPIKE_H
#define SPIKE_H

#include <deque>
#include <vector>
#include <tuple>

// // Internal representation of a spike. User has no need to use it.
struct Spike
{
    int amplitude;
    int channel;
    int frame;
    std::deque<int> largest_channels;
    std::vector<int> written_cutout;
    std::tuple<std::vector<int>, int *> waveformscounts;
};

#endif
