#ifndef TRACEWRAPPER_H
#define TRACEWRAPPER_H

namespace HSDetection
{
    class TraceWrapper
    {
    private:
        short *traceBuffer; // passed in, should not release here

        int frameOffset; // offset of current chunk
        int numChannels;
        int chunkSize; // offset advenced by this size with each new chunk

    public:
        TraceWrapper(int leftMargin, int numChannels, int chunkSize)
            : traceBuffer(nullptr), frameOffset(-leftMargin - chunkSize),
              numChannels(numChannels), chunkSize(chunkSize) {}
        ~TraceWrapper() {}

        // should be called to both provide a buffer and advance the offset
        void updateChunk(short *traceBuffer) { this->traceBuffer = traceBuffer, frameOffset += chunkSize; }

        const short *operator[](int frame) const { return traceBuffer + (frame - frameOffset) * numChannels; }
        short *operator[](int frame) { return traceBuffer + (frame - frameOffset) * numChannels; }

        const short &operator()(int frame, int channel) const { return (*this)[frame][channel]; }
        short &operator()(int frame, int channel) { return (*this)[frame][channel]; }
    };

} // namespace HSDetection

#endif
