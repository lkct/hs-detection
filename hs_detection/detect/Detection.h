#ifndef DETECTION_H
#define DETECTION_H

#include <string>
#include "VoltTrace.h"
#include "SpikeQueue.h"
#include "Point.h"
#include "ProbeLayout.h"

namespace HSDetection
{
    class Detection
    {
    private:
        int nChannels; // number of probe channels

        // Variables for variance and mean
        int *Qv;   // noise amplitude
        int *Qb;   // median
        int **Qbs; // stores spike_delay + MaxSl baseline values;
        int **Qvs; // stores spike_delay + MaxSl baseline values;

        int QbsLen;

        // Variables for the spike detection
        int *Sl;      // counter for spike length
        bool *AHP;    // counter for repolarizing current
        int *Amp;     // buffers spike amplitude
        int *SpkArea; // integrates over spike

        // Parameters for spike detection
        int threshold; // threshold to detect spikes >11 is likely to be real spikes, but can and should be sorted afterwards
        int minAvgAmp; // minimal avg. amplitude of peak (in units of Qv)
        int AHPthr;    // signal should go below that threshold within MaxSl-MinSl frames
        int maxSl;     // dead time in frames after peak, used for further testing
        int minSl;     // length considered for determining avg. spike amplitude

        // Files to save the spikes etc.
        int *Aglobal;

        int currQbsPosition;

        int spikePeakDuration;

        int framesLeftMargin; // num of frames in input as left margin

        // TODO: consts
        const int Ascale = -64; // factor to multiply to raw traces to increase
                                // resolution; definition of ADC counts had been
                                // changed!
        const int Voffset = 0;  // mean ADC counts, as initial value for Qb

        // Parameters for variance and mean updates
        const int Qvmin = 200; // set minimum value of Qv
        const int Tau_m0 = 4;  // timescale for updating Qb (increment is Qv/Tau_m)

        const int QvChange = 1; // TODO: Qv change amount?

        SpikeQueue *pQueue;

    public:
        std::string filename;

        std::vector<char> result;

        bool saveShape;

    public:
        // TODO: from parameters
        static const int ASCALE = -64;  // Scaling on the raw extracellular data
        static int num_com_centers;     // Number of channels used for center of mass
        static int num_channels;        // Number of channels on the probe
        static int spike_peak_duration; // The number of frames it takes a spike amplitude to fully decay.
        static int noise_duration;      // The number of frames that the true spike can occur after the first detection.
        static float noise_amp_percent; // Amplitude percentage allowed to differentiate between decreasing amplitude duplicate spike
        static bool to_localize;        // True: filter and localize the spike, False: just filter the spike.
        static bool decay_filtering;    // if true, then tries to filter by decay (more effective for less dense arrays)
        static int maxsl;               // Number of frames after a detection that a spike is accepted
        static int cutout_start;        // The number of frames before the spike that the cutout starts at
        static int cutout_end;          // The number of frames after the spike that the cutout ends atextern int filtered_spikes; //number of filtered spikes
        static int cutout_size;

        static VoltTrace trace;

        static ProbeLayout probeLayout;

        Detection(int chunkSize, int *positionMatrix,
                  int nChannels, int spikePeakDuration, std::string filename,
                  int noiseDuration, float noiseAmpPercent, float neighborRadius, float innerRadius,
                  int numComCenters, bool localize,
                  int threshold, int cutoutStart, int cutoutEnd, int minAvgAmp,
                  int ahpthr, int maxSl, int minSl, bool decayFiltering, bool saveShape,
                  int framesLeftMargin);
        ~Detection();
        void Iterate(short *traceBuffer, int t0, int tInc);
        int FinishDetection();
        char *Get();

    private:
        void MedianVoltage(short *traceBuffer);
        void MeanVoltage(short *traceBuffer, int t0, int tInc);
    };

} // namespace HSDetection

#endif
