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
        short *operator[](int frame) const { return traceBuffer + (frame - frameOffset) * numChannels; }
        short get(int frame, int channel) const { return (*this)[frame][channel]; }
        // TODO: sliceFrame
    };

} // namespace HSDetection

#endif
