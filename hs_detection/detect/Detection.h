#ifndef DETECTION_H
#define DETECTION_H

#include <string>
#include "TraceWrapper.h"
#include "SpikeQueue.h"
#include "Point.h"
#include "ProbeLayout.h"
#include "RollingArray.h"

namespace HSDetection
{
    class Detection
    {
    private:
        int nChannels; // number of probe channels

        // Variables for variance and mean
        // int *Qv;   // noise amplitude
        // int *Qb;   // median
        // int **Qbs; // stores spike_delay + MaxSl baseline values;
        // int **Qvs; // stores spike_delay + MaxSl baseline values;

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
        short *_Aglobal;

        int framesLeftMargin; // num of frames in input as left margin

        // TODO: consts
        const int Voffset = 0 * -64; // mean ADC counts, as initial value for Qb

        // Parameters for variance and mean updates
        const int Qvmin = 200; // set minimum value of Qv
        const int Tau_m0 = 4;  // timescale for updating Qb (increment is Qv/Tau_m)

        const int QvChange = 1; // TODO: Qv change amount?

        SpikeQueue *pQueue;

        void MedianVoltage(short *traceBuffer, int t0, int tInc);
        void MeanVoltage(short *traceBuffer, int t0, int tInc);

    public:
        std::string filename;

        std::vector<char> result;

        bool to_localize; // True: filter and localize the spike, False: just filter the spike.
        bool saveShape;

        int cutout_start;        // The number of frames before the spike that the cutout starts at
        int cutout_size;         // The number of frames after the spike that the cutout ends atextern int filtered_spikes; //number of filtered spikes
        int noise_duration;      // The number of frames that the true spike can occur after the first detection.
        int spike_peak_duration; // The number of frames it takes a spike amplitude to fully decay.
        int num_com_centers;     // Number of channels used for center of mass

        ProbeLayout probeLayout;

        TraceWrapper trace;
        TraceWrapper AGlobal;

        RollingArray QBs;
        RollingArray QVs;

        // TODO: decay filtering
        bool decay_filtering;    // if true, then tries to filter by decay (more effective for less dense arrays)
        float noise_amp_percent; // Amplitude percentage allowed to differentiate between decreasing amplitude duplicate spike

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
    };

} // namespace HSDetection

#endif
