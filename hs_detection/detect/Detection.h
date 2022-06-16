#ifndef DETECTION_H
#define DETECTION_H

#include <string>
#include "VoltTrace.h"
#include "SpikeQueue.h"
#include "Point.h"

namespace HSDetection
{
    class Detection
    {
    private:
        int nChannels; // number of probe channels
        int tInc;      // chunk size of data reading (the last chunk can be smaller than this)

        // Variables for variance and mean
        int *Qd;   // noise amplitude
        int *Qm;   // median
        int **Qms; // stores spike_delay + MaxSl baseline values;

        // Variables for the spike detection
        int *Sl;      // counter for spike length
        bool *AHP;    // counter for repolarizing current
        int *Amp;     // buffers spike amplitude
        int *SpkArea; // integrates over spike

        // Parameters for spike detection
        int threshold; // threshold to detect spikes >11 is likely to be real spikes, but can and should be sorted afterwards
        int minAvgAmp; // minimal avg. amplitude of peak (in units of Qd)
        int AHPthr;    // signal should go below that threshold within MaxSl-MinSl frames
        int maxSl;     // dead time in frames after peak, used for further testing
        int minSl;     // length considered for determining avg. spike amplitude

        // Files to save the spikes etc.
        int *Aglobal;

        int currQmsPosition;

        int spikePeakDuration;

        // TODO: consts
        const int Ascale = -64; // factor to multiply to raw traces to increase
                                // resolution; definition of ADC counts had been
                                // changed!
        const int Voffset = 0;  // mean ADC counts, as initial value for Qm

        // Parameters for variance and mean updates
        const int Qdmin = 200; // set minimum value of Qd
        const int Tau_m0 = 4;  // timescale for updating Qm (increment is Qd/Tau_m)

        SpikeQueue *pQueue;

    public:
        std::string filename;

    public:
        // TODO: from parameters
        static const int ASCALE = -64;  // Scaling on the raw extracellular data
        static int num_com_centers;     // Number of channels used for center of mass
        static int num_channels;        // Number of channels on the probe
        static int spike_peak_duration; // The number of frames it takes a spike amplitude to fully decay.
        static int noise_duration;      // The number of frames that the true spike can occur after the first detection.
        static float noise_amp_percent; // Amplitude percentage allowed to differentiate between decreasing amplitude duplicate spike
        static int max_neighbors;       // Maximum number of neighbors a channel can have in the probe
        static bool to_localize;        // True: filter and localize the spike, False: just filter the spike.
        static bool decay_filtering;    // if true, then tries to filter by decay (more effective for less dense arrays)
        static int maxsl;               // Number of frames after a detection that a spike is accepted
        static int cutout_start;        // The number of frames before the spike that the cutout starts at
        static int cutout_end;          // The number of frames after the spike that the cutout ends atextern int filtered_spikes; //number of filtered spikes
        static int cutout_size;
        static float inner_radius;
        static int **neighbor_matrix;       /*Indexed by the channel number starting at 0 and going up to num_channels - 1. Each
                                             index contains pointer to another array which contains channel number of all its neighbors.
                                             User creates this before calling SpikeHandler. Each column has size equal to max neighbors where
                                             any channels that have less neighbors fills the rest with -1 (important). */
        static int **inner_neighbor_matrix; /*Indexed by the channel number starting at 0 and going up to num_channels - 1. Each
                                      index contains pointer to another array which contains channel number of all its inner neighbors.
                                      Created by SpikeHandler; */
        static int **outer_neighbor_matrix; /*Indexed by the channel number starting at 0 and going up to num_channels - 1. Each
                                            index contains pointer to another array which contains channel number of all its outer neighbors.
                                            Created by SpikeHandler; */
        static Point *channel_positions;    /*Indexed by the channel number starting at 0 and going up to num_channels - 1. Each
                                          index contains pointer to another array which contains X and Y position of the channel. User creates
                                          this before calling SpikeHandler. */
        static int t_inc;

        static VoltTrace trace;

        Detection(int tInc, int *positionMatrix, int *neighborMatrix,
                  int nChannels, int spikePeakDuration, std::string filename,
                  int noiseDuration, float noiseAmpPercent, float innerRadius,
                  int maxNeighbors, int numComCenters, bool localize,
                  int threshold, int cutoutStart, int cutoutEnd, int minAvgAmp,
                  int ahpthr, int maxSl, int minSl, bool decayFiltering);
        ~Detection();
        void MedianVoltage(short *traceBuffer);
        void MeanVoltage(short *traceBuffer, int tInc, int tCut);
        void Iterate(short *traceBuffer, int t0, int tInc, int tCut, int tCut2);
        void FinishDetection();
    };

} // namespace HSDetection

#endif
