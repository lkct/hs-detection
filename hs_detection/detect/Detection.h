#ifndef DETECTION_H
#define DETECTION_H

#include <string>

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

        // Parameters for recalibration events and artefact handling
        int *A; // control parameter for amplifier effects

        // Files to save the spikes etc.
        int *Aglobal;
        int *Slice;

        int iterations;
        int currQmsPosition;

        int spikePeakDuration;

        // TODO: consts
        const int Ascale = -64; // factor to multiply to raw traces to increase
                                // resolution; definition of ADC counts had been
                                // changed!
        const int Voffset = 0;  // mean ADC counts, as initial value for Qm
        const int artT = 10;    // to use after artefacts; to update Qm for 10 frames

        // Parameters for variance and mean updates
        const int Qdmin = 200; // set minimum value of Qd
        const int Tau_m0 = 4;  // timescale for updating Qm (increment is Qd/Tau_m)

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
        static float **channel_positions;   /*Indexed by the channel number starting at 0 and going up to num_channels - 1. Each
                                         index contains pointer to another array which contains X and Y position of the channel. User creates
                                         this before calling SpikeHandler. */
        static int t_inc;

        Detection(int tInc, int *positionMatrix, int *neighborMatrix,
                  int nChannels, int spikePeakDuration, std::string filename,
                  int noiseDuration, float noiseAmpPercent, float innerRadius,
                  int maxNeighbors, int numComCenters, bool localize,
                  int threshold, int cutoutStart, int cutoutEnd, int minAvgAmp,
                  int ahpthr, int maxSl, int minSl, bool decayFiltering);
        ~Detection();
        void MedianVoltage(short *vm);
        void MeanVoltage(short *vm, int tInc, int tCut);
        void Iterate(short *vm, int t0, int tInc, int tCut, int tCut2);
        void FinishDetection();
    };

    class RawData
    {
    public:
        static short *raw_data;     // raw data passed in for current iteration
        static int index_data;      // The index given to start accessing the raw data. To account for extra data tacked on for cutout purposes.
        static int iterations;      // Number of current iterations of raw data passed in. User starts this at 0 and increments it for each chunk of data;
    };
} // namespace HSDetection

#endif
