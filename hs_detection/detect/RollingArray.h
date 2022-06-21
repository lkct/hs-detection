#ifndef ROLLINGARRAY_H
#define ROLLINGARRAY_H

#include "Types.h"

namespace HSDetection
{
    class RollingArray
    {
    private:
        IntVolt *arrayBuffer; // created and released here
        IntChannel numChannels;

        // TODO: must be >=chunkSize if cache all chunk, better to be 2^n?
        static constexpr IntFrame length = 131072; // rolling length

    public:
        RollingArray(IntChannel numChannels) : arrayBuffer(new IntVolt[(IntFxC)length * numChannels]), numChannels(numChannels) {}
        ~RollingArray() { delete[] arrayBuffer; }

        // copy constructor deleted to protect buffer
        RollingArray(const RollingArray &) = delete;
        // copy assignment deleted to protect buffer
        RollingArray &operator=(const RollingArray &) = delete;

        const IntVolt *operator[](IntFrame frame) const { return arrayBuffer + ((IntFxC)frame + length) % length * numChannels; }
        IntVolt *operator[](IntFrame frame) { return arrayBuffer + ((IntFxC)frame + length) % length * numChannels; }
        // allow negative idx within one length, due to c++ mod property

        const IntVolt &operator()(IntFrame frame, IntChannel channel) const { return (*this)[frame][channel]; }
        IntVolt &operator()(IntFrame frame, IntChannel channel) { return (*this)[frame][channel]; }
    };

} // namespace HSDetection

#endif
