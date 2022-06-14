#ifndef SPIKE_H
#define SPIKE_H

#include <deque>
#include <vector>
#include <tuple>

// // Internal representation of a spike. User has no need to use it.
class Spike
{
public:
    int channel;
    int frame;
    int amplitude;
    std::deque<int> largest_channels;
    std::vector<int> written_cutout;
    std::tuple<std::vector<int>, int *> waveformscounts;

    Spike(int frame, int channel, int amplitude)
        : channel(channel), frame(frame), amplitude(amplitude) {}
    ~Spike() {}
};

#endif
