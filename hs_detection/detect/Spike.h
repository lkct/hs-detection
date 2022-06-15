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
        std::vector<int> written_cutout;
        std::vector<std::vector<std::pair<int, int>>> waveforms;

        int aGlobal = 0;            // Global noise
        std::vector<int> baselines; // Contains spike_delay number of frames of median baseline values. Updated by user at every frame.
    };

} // namespace HSDetection

#endif
