#ifndef VOLTTRACE_H
#define VOLTTRACE_H

namespace HSDetection
{
    class VoltTrace
    {
    private:
        short *traceBuffer; // passed in, should not release here
        int frameOffset;
        int numChannels;
        int chunkSize;

    public:
        VoltTrace(int cutoutLeft, int numChannels, int chunkSize)
            : traceBuffer(nullptr), frameOffset(-cutoutLeft - chunkSize),
              numChannels(numChannels), chunkSize(chunkSize) {}
        ~VoltTrace() {}

        void updateChunk(short *traceBuffer) { this->traceBuffer = traceBuffer, frameOffset += chunkSize; }

        const short *operator[](int frame) const { return traceBuffer + (frame - frameOffset) * numChannels; }
        short *operator[](int frame) { return traceBuffer + (frame - frameOffset) * numChannels; }

        const short &operator()(int frame, int channel) const { return (*this)[frame][channel]; }
        short &operator()(int frame, int channel) { return (*this)[frame][channel]; }
    };

} // namespace HSDetection

#endif
