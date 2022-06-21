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
        const IntVolt Tau_m0 = 4;        // timescale for updating Qb (increment is Qv/Tau_m)
        const IntVolt Qvmin = 200;       // set minimum value of Qv
        const IntVolt Voffset = 0 * -64; // mean ADC counts, as initial value for Qb
        const IntVolt Qvstart = 400;     // start value of Qv // TODO: ???
        const IntVolt QvChange = 1;      // TODO: Qv change amount?

        // input trace
    public:
        TraceWrapper trace;
        TraceWrapper commonRef;
        RollingArray runningBaseline; // median
        RollingArray runningVariance; // noise amplitude

    private:
        IntVolt *_commonRef; // for commonRef
        IntChannel numChannels;
        IntFrame chunkSize;
        IntFrame chunkLeftMargin;

        // detection
    private:
        IntFrame *spikeTime; // counter for spike length
        IntVolt *spikeAmp;   // buffers spike amplitude
        IntFxV *spikeArea;   // integrates over spike // actually area*fps
        bool *hasAHP;        // counter for repolarizing current

        IntFrame spikeLen; // dead time in frames after peak, used for further testing
        IntFrame peakLen;  // length considered for determining avg. spike amplitude
        IntVolt threshold; // threshold to detect spikes >11 is likely to be real spikes, but can and should be sorted afterwards(in units of Qv)
        IntVolt minAvgAmp; // minimal avg. amplitude of peak (in units of Qv)
        IntVolt maxAHPAmp; // signal should go below that threshold within MaxSl-MinSl frames(in units of Qv)

        // queue processing
    private:
        SpikeQueue *pQueue;

    public:
        ProbeLayout probeLayout;

        std::vector<Spike> result;

        IntFrame noiseDuration;     // The number of frames that the true spike can occur after the first detection.
        IntFrame spikePeakDuration; // The number of frames it takes a spike amplitude to fully decay.

        // decay filtering
    public:
        bool decayFilter;      // if true, then tries to filter by decay (more effective for less dense arrays)
        float noiseAmpPercent; // percent??? Amplitude percentage allowed to differentiate between decreasing amplitude duplicate spike

        // localization
    public:
        bool localize;            // True: filter and localize the spike, False: just filter the spike.
        IntChannel numCoMCenters; // Number of channels used for center of mass

        // save shape
    public:
        bool saveShape;
        std::string filename;
        IntFrame cutoutStart; // The number of frames before the spike that the cutout starts at
        IntFrame cutoutLen;   // The number of frames after the spike that the cutout ends at

        // methods
    private:
        void commonMedian(IntFrame chunkStart, IntFrame chunkLen);
        void commonAverage(IntFrame chunkStart, IntFrame chunkLen);

    public:
        Detection(IntChannel numChannels, IntFrame chunkSize, IntFrame chunkLeftMargin,
                  IntFrame spikeLen, IntFrame peakLen, IntVolt threshold, IntVolt minAvgAmp, IntVolt maxAHPAmp,
                  FloatGeom *channelPositions, FloatGeom neighborRadius, FloatGeom innerRadius,
                  IntFrame noiseDuration, IntFrame spikePeakDuration,
                  bool decayFiltering, float noiseAmpPercent,
                  bool localize, IntChannel numCoMCenters,
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
