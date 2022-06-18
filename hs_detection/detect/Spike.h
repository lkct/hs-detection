#ifndef SPIKE_H
#define SPIKE_H

#include <vector>
#include <utility>

#include "Point.h"

namespace HSDetection
{
    class Spike
    {
    public:
        int frame;
        int channel;
        int amplitude;
        Point position;

        Spike(int frame, int channel, int amplitude)
            : frame(frame), channel(channel), amplitude(amplitude), position() {}
        ~Spike() {}

    public: // TODO: remove?
        std::vector<std::vector<std::pair<int, int>>> waveforms;
    };

} // namespace HSDetection

#endif
