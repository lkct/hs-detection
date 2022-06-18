#ifndef ROLLINGARRAY_H
#define ROLLINGARRAY_H

#include <utility>

namespace HSDetection
{
    class RollingArray
    {
    private:
        short *arrayBuffer; // created and released here
        int numChannels;

        static constexpr int length = 131072; // TODO: must be >=chunkSize if cache all chunk, better to be 2^n?

    public:
        RollingArray(int numChannels) : numChannels(numChannels) { arrayBuffer = new short[length * numChannels]; }
        ~RollingArray() { delete[] arrayBuffer; }

        // delete copy to guard the internal buffer
        RollingArray(const RollingArray &) = delete;
        RollingArray &operator=(const RollingArray &) = delete;

        const short *operator[](int frame) const { return arrayBuffer + (frame + length) % length * numChannels; }
        short *operator[](int frame) { return arrayBuffer + (frame + length) % length * numChannels; }

        const short &operator()(int frame, int channel) const { return (*this)[frame][channel]; }
        short &operator()(int frame, int channel) { return (*this)[frame][channel]; }
    };

} // namespace HSDetection

#endif
