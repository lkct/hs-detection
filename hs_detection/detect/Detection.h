#ifndef DETECTION_H
#define DETECTION_H

#include <string>

#include "ProbeLayout.h"
#include "TraceWrapper.h"
#include "RollingArray.h"
#include "SpikeQueue.h"

namespace HSDetection
{
    class Detection
    {
        // constants
    private:
        const IntVolt initBase = 0 * -64; // initial value of baseline // TODO: 64?
        const IntVolt initDev = 400;      // initial value of deviation
        const IntVolt tauBase = 4;        // time constant for baseline update
        const IntVolt devChange = 1;      // changing for deviation update
        const IntVolt minDev = 200;       // minimum level of deviation

        // input data
    public:
        TraceWrapper trace;            // input trace
        TraceWrapper commonRef;        // common median/average reference
        RollingArray runningBaseline;  // running estimation of baseline (33 percentile)
        RollingArray runningDeviation; // running estimation of deviation from baseline

    private:
        IntVolt *_commonRef;      // internal buffer for commonRef
        IntChannel numChannels;   // number of probe channels
        IntFrame chunkSize;       // size of each chunk, only the last chunk can be of a different (smaller) size
        IntFrame chunkLeftMargin; // margin on the left of each chunk

        // detection
    private:
        IntFrame *spikeTime; // counter for time since spike peak
        IntVolt *spikeAmp;   // spike peak amplitude
        IntFxV *spikeArea;   // area under spike used for average amplitude, actually integral*fps
        bool *hasAHP;        // flag for AHP existence

        IntFrame spikeDur;  // duration of a whole spike
        IntFrame ampAvgDur; // duration to average amplitude
        IntVolt threshold;  // threshold to detect spikes, used as multiplier of deviation
        IntVolt minAvgAmp;  // threshold for average amplitude of peak, used as multiplier of deviation
        IntVolt maxAHPAmp;  // threshold for voltage level of AHP, used as multiplier of deviation

        // queue processing
    private:
        SpikeQueue *pQueue; // spike queue, must be a pointer to be new-ed later

    public:
        ProbeLayout probeLayout; // geometry for probe layout

        std::vector<Spike> result; // detection result, use vector to expand as needed

        IntFrame jitterTol; // tolerance of jitter in raw data
        IntFrame peakDur;   // duration of spike peaks

        // decay filtering
    public:
        bool decayFilter;      // whether to use decay filtering instead of normal one
        FloatRatio decayRatio; // ratio of amplitude to be considered as decayed

        // localization
    public:
        bool localize; // whether to turn on localization

        // save shape
    public:
        bool saveShape;       // whether to save spike shapes to file
        std::string filename; // filename for saving
        IntFrame cutoutStart; // the start of spike shape cutout
        IntFrame cutoutLen;   // the length of shape cutout

        // methods
    private:
        void commonMedian(IntFrame chunkStart, IntFrame chunkLen);
        void commonAverage(IntFrame chunkStart, IntFrame chunkLen);

    public:
        Detection(IntChannel numChannels, IntFrame chunkSize, IntFrame chunkLeftMargin,
                  IntFrame spikeDur, IntFrame ampAvgDur, IntVolt threshold, IntVolt minAvgAmp, IntVolt maxAHPAmp,
                  FloatGeom *channelPositions, FloatGeom neighborRadius, FloatGeom innerRadius,
                  IntFrame jitterTol, IntFrame peakDur,
                  bool decayFiltering, FloatRatio decayRatio, bool localize,
                  bool saveShape, std::string filename, IntFrame cutoutStart, IntFrame cutoutLen);
        ~Detection();

        // copy constructor deleted to protect internals
        Detection(const Detection &) = delete;
        // copy assignment deleted to protect internals
        Detection &operator=(const Detection &) = delete;

        void step(IntVolt *traceBuffer, IntFrame chunkStart, IntFrame chunkLen);
        IntResult finish();
        const Spike *getResult() const;

    }; // class Detection

} // namespace HSDetection

#endif
