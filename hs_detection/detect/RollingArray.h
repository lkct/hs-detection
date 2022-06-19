#ifndef ROLLINGARRAY_H
#define ROLLINGARRAY_H

namespace HSDetection
{
    class RollingArray
    {
    private:
        short *arrayBuffer; // created and released here
        int numChannels;

        // TODO: must be >=chunkSize if cache all chunk, better to be 2^n?
        static constexpr int length = 131072; // rolling length

    public:
        RollingArray(int numChannels) : arrayBuffer(new short[length * numChannels]), numChannels(numChannels) {}
        ~RollingArray() { delete[] arrayBuffer; }

        // copy constructor deleted to protect buffer
        RollingArray(const RollingArray &) = delete;
        // copy assignment deleted to protect buffer
        RollingArray &operator=(const RollingArray &) = delete;

        const short *operator[](int frame) const { return arrayBuffer + (frame + length) % length * numChannels; }
        short *operator[](int frame) { return arrayBuffer + (frame + length) % length * numChannels; }
        // allow negative idx within one length, due to c++ mod property

        const short &operator()(int frame, int channel) const { return (*this)[frame][channel]; }
        short &operator()(int frame, int channel) { return (*this)[frame][channel]; }
    };

} // namespace HSDetection

#endif
