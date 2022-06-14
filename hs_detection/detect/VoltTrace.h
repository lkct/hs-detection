#ifndef VOLTTRACE_H
#define VOLTTRACE_H

#include <vector>

namespace HSDetection
{
    class VoltTrace
    {
    private:
        short *traceBuffer; // NOTE: passed in, should not release here
        int numChannels;
        int frameOffset;
        int chunkSize;

    public: // NOTE: simple implementation, inlined
        VoltTrace(int chunkSize, int cutoutLeft, int numChannels)
            : traceBuffer(nullptr), numChannels(numChannels),
              frameOffset(-cutoutLeft - chunkSize), chunkSize(chunkSize) {}
        ~VoltTrace() {}

        void updateChunk(short *traceBuffer)
        {
            this->traceBuffer = traceBuffer;
            frameOffset += chunkSize;
        }
        short *operator[](int frame) const { return traceBuffer + (frame - frameOffset) * numChannels; }
        short get(int frame, int ch) const { return (*this)[frame][ch]; }
        // TODO: sliceFrame
    };

} // namespace HSDetection

#endif
