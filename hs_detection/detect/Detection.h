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
}

#endif
