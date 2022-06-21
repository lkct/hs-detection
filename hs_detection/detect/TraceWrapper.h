#ifndef TRACEWRAPPER_H
#define TRACEWRAPPER_H

#include "Types.h"

namespace HSDetection
{
    class TraceWrapper
    {
    private:
        IntVolt *traceBuffer; // passed in, should not release here

        IntFrame frameOffset; // offset of current chunk
        IntChannel numChannels;
        IntFrame chunkSize; // offset advenced by this size with each new chunk

    public:
        TraceWrapper(IntFrame leftMargin, IntChannel numChannels, IntFrame chunkSize)
            : traceBuffer(nullptr), frameOffset(-leftMargin - chunkSize),
              numChannels(numChannels), chunkSize(chunkSize) {}
        ~TraceWrapper() {}

        // should be called to both provide a buffer and advance the offset
        void updateChunk(IntVolt *traceBuffer) { this->traceBuffer = traceBuffer, frameOffset += chunkSize; }

        const IntVolt *operator[](IntFrame frame) const { return traceBuffer + ((IntFxC)frame - frameOffset) * numChannels; }
        IntVolt *operator[](IntFrame frame) { return traceBuffer + ((IntFxC)frame - frameOffset) * numChannels; }

        const IntVolt &operator()(IntFrame frame, IntChannel channel) const { return (*this)[frame][channel]; }
        IntVolt &operator()(IntFrame frame, IntChannel channel) { return (*this)[frame][channel]; }
    };

} // namespace HSDetection

#endif
