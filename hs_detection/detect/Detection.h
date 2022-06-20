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
        const int Tau_m0 = 4;        // timescale for updating Qb (increment is Qv/Tau_m)
        const int Qvmin = 200;       // set minimum value of Qv
        const int Voffset = 0 * -64; // mean ADC counts, as initial value for Qb
        const int Qvstart = 400;     // start value of Qv // TODO: ???
        const int QvChange = 1;      // TODO: Qv change amount?

        // input trace
    public:
        TraceWrapper trace;
        TraceWrapper commonRef;
        RollingArray runningBaseline; // median
        RollingArray runningVariance; // noise amplitude

    private:
        short *_commonRef; // for commonRef
        int numChannels;
        int chunkSize;
        int chunkLeftMargin;

        // detection
    private:
        int *spikeTime; // counter for spike length
        int *spikeAmp;  // buffers spike amplitude
        int *spikeArea; // integrates over spike // actually area*fps
        bool *hasAHP;   // counter for repolarizing current

        int spikeLen;  // dead time in frames after peak, used for further testing
        int peakLen;   // length considered for determining avg. spike amplitude
        int threshold; // threshold to detect spikes >11 is likely to be real spikes, but can and should be sorted afterwards(in units of Qv)
        int minAvgAmp; // minimal avg. amplitude of peak (in units of Qv)
        int maxAHPAmp; // signal should go below that threshold within MaxSl-MinSl frames(in units of Qv)

        // queue processing
    private:
        SpikeQueue *pQueue;

    public:
        ProbeLayout probeLayout;

        std::vector<Spike> result;

        int noiseDuration;     // The number of frames that the true spike can occur after the first detection.
        int spikePeakDuration; // The number of frames it takes a spike amplitude to fully decay.

        // decay filtering
    public:
        bool decayFilter;      // if true, then tries to filter by decay (more effective for less dense arrays)
        float noiseAmpPercent; // percent??? Amplitude percentage allowed to differentiate between decreasing amplitude duplicate spike

        // localization
    public:
        bool localize;     // True: filter and localize the spike, False: just filter the spike.
        int numCoMCenters; // Number of channels used for center of mass

        // save shape
    public:
        bool saveShape;
        std::string filename;
        int cutoutStart; // The number of frames before the spike that the cutout starts at
        int cutoutLen;   // The number of frames after the spike that the cutout ends atextern int filtered_spikes; //number of filtered spikes

        // methods
    private:
        void commonMedian(int chunkStart, int chunkLen);
        void commonAverage(int chunkStart, int chunkLen);

    public:
        Detection(int numChannels, int chunkSize, int chunkLeftMargin,
                  int spikeLen, int peakLen, int threshold, int minAvgAmp, int maxAHPAmp,
                  float *channelPositions, float neighborRadius, float innerRadius,
                  int noiseDuration, int spikePeakDuration,
                  bool decayFiltering, float noiseAmpPercent,
                  bool localize, int numCoMCenters,
                  bool saveShape, std::string filename, int cutoutStart, int cutoutLen);
        ~Detection();

        // copy constructor deleted to protect internals
        Detection(const Detection &) = delete;
        // copy assignment deleted to protect internals
        Detection &operator=(const Detection &) = delete;

        void step(short *traceBuffer, int chunkStart, int chunkLen);
        int finish();
        const Spike *getResult() const;

    }; // class Detection

} // namespace HSDetection

#endif
