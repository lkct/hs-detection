#ifndef SPIKE_H
#define SPIKE_H

#include "Point.h"

namespace HSDetection
{
    class Spike
    {
    public:
        int frame;
        int channel;
        int amplitude;
        Point position; // default to (0,0) if not localized

        Spike(int frame, int channel, int amplitude)
            : frame(frame), channel(channel), amplitude(amplitude), position() {}
        ~Spike() {}
    };

} // namespace HSDetection

#endif
