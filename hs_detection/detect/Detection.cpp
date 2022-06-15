#include <algorithm>
#include <cstring>
#include "Utils.h"
#include "Detection.h"

using namespace std;

namespace HSDetection
{
    int Detection::num_com_centers = 0;
    int Detection::num_channels = 0;
    int Detection::spike_peak_duration = 0;
    int Detection::noise_duration = 0;
    float Detection::noise_amp_percent = 0;
    int Detection::max_neighbors = 0;
    bool Detection::to_localize = 0;
    bool Detection::decay_filtering = 0;
    int Detection::maxsl = 0;
    int Detection::cutout_start = 0;
    int Detection::cutout_end = 0;
    int Detection::cutout_size = 0;
    float Detection::inner_radius = 0;
    int **Detection::neighbor_matrix = nullptr;
    Point *Detection::channel_positions = nullptr;
    int **Detection::inner_neighbor_matrix = nullptr;
    int **Detection::outer_neighbor_matrix = nullptr;
    int Detection::t_inc = 0;

    VoltTrace Detection::trace(0, 0, 0);

    Detection::Detection(int tInc, int *positionMatrix, int *neighborMatrix,
                         int nChannels, int spikePeakDuration, string filename,
                         int noiseDuration, float noiseAmpPercent, float innerRadius,
                         int maxNeighbors, int numComCenters, bool localize,
                         int threshold, int cutoutStart, int cutoutEnd, int minAvgAmp,
                         int ahpthr, int maxSl, int minSl, bool decayFiltering)
        : nChannels(nChannels), tInc(tInc), threshold(threshold), minAvgAmp(minAvgAmp),
          AHPthr(ahpthr), maxSl(maxSl), minSl(minSl),
          currQmsPosition(-1), spikePeakDuration(spikePeakDuration), filename(filename + ".bin")
    {
        Qd = new int[nChannels];
        Qm = new int[nChannels];
        Qms = new int *[spikePeakDuration + maxSl];
        for (int i = 0; i < spikePeakDuration + maxSl; i++)
        {
            Qms[i] = new int[nChannels];
        }

        Sl = new int[nChannels];
        AHP = new bool[nChannels];
        Amp = new int[nChannels];
        SpkArea = new int[nChannels];

        A = new int[nChannels];

        Slice = new int[nChannels];
        Aglobal = new int[tInc];

        fill_n(Qd, nChannels, 400); // TODO: magic number?
        fill_n(Qm, nChannels, Voffset * Ascale);

        memset(Sl, 0, nChannels * sizeof(int));      // TODO: 0 init?
        memset(AHP, 0, nChannels * sizeof(bool));    // TODO: 0 init?
        memset(Amp, 0, nChannels * sizeof(int));     // TODO: 0 init?
        memset(SpkArea, 0, nChannels * sizeof(int)); // TODO: 0 init?

        fill_n(A, nChannels, artT);

        // TODO: init of Slice?
        memset(Aglobal, 0, tInc * sizeof(int)); // TODO: 0 init?

        Point *channelPosition; // TODO: float or int? input is int
        int **channelNeighbor;
        channelPosition = new Point[nChannels];
        channelNeighbor = new int *[nChannels];
        for (int i = 0; i < nChannels; i++)
        {
            channelPosition[i] = Point(positionMatrix[i * 2], positionMatrix[i * 2 + 1]);
            channelNeighbor[i] = new int[maxNeighbors];
            memcpy(channelNeighbor[i], neighborMatrix + i * maxNeighbors, maxNeighbors * sizeof(int));
        }

        // TODO: error check?
        num_com_centers = numComCenters;
        num_channels = nChannels;
        spike_peak_duration = spikePeakDuration;
        noise_duration = noiseDuration;
        noise_amp_percent = noiseAmpPercent;
        max_neighbors = maxNeighbors;
        to_localize = localize;
        decay_filtering = decayFiltering;
        maxsl = maxSl;
        cutout_start = cutoutStart;
        cutout_end = cutoutEnd;
        cutout_size = cutoutStart + cutoutEnd + 1;
        inner_radius = innerRadius;
        channel_positions = channelPosition;
        neighbor_matrix = channelNeighbor;
        t_inc = tInc;

        trace = VoltTrace(cutoutStart + maxSl, nChannels, tInc);

        inner_neighbor_matrix = Utils::createInnerNeighborMatrix();
        outer_neighbor_matrix = Utils::createOuterNeighborMatrix();
        Utils::fillNeighborLayerMatrices();

        pQueue = new SpikeQueue(this); // all the params should be ready
    }

    Detection::~Detection()
    {
        delete[] Qd;
        delete[] Qm;
        for (int i = 0; i < spikePeakDuration + maxSl; i++)
        {
            delete[] Qms[i];
        }
        delete[] Qms;

        delete[] Sl;
        delete[] AHP;
        delete[] Amp;
        delete[] SpkArea;

        delete[] A;

        delete[] Slice;
        delete[] Aglobal;

        delete[] channel_positions;
        for (int i = 0; i < nChannels; i++)
        {
            delete[] neighbor_matrix[i];
        }
        delete[] neighbor_matrix;

        delete pQueue;
    }

    void Detection::MedianVoltage(short *traceBuffer) // TODO: add, use?
    {
    }

    void Detection::MeanVoltage(short *traceBuffer, int tInc, int tCut)
    {
        // // if median takes too long...
        // // or there are only few
        // // channnels (?)
        for (int t = 0, tTrace = tCut; t < tInc; t++, tTrace++)
        {
            int sum = 0;
            for (int i = 0; i < nChannels; i++)
            {
                sum += traceBuffer[tTrace * nChannels + i];
            }
            Aglobal[t] = sum / (nChannels + 1); // TODO: no need +1
        }
    }

    void Detection::Iterate(short *traceBuffer, int t0, int tInc, int tCut, int tCut2)
    {
        trace.updateChunk(traceBuffer);

        // // Does this need to end at tInc + tCut? (Cole+Martino)
        for (int t = tCut; t < tInc; t++)
        {
            currQmsPosition++;
            for (int i = 0; i < nChannels; i++)
            {
                // // CHANNEL OUT OF LINEAR REGIME
                // // difference between ADC counts and Qm
                int a = (traceBuffer[t * nChannels + i] - Aglobal[t - tCut]) * Ascale - Qm[i];

                // TODO: clean `if`s
                // // UPDATE Qm and Qd
                if (a > 0)
                {
                    if (a > Qd[i])
                    {
                        Qm[i] += Qd[i] / Tau_m0;
                        if (a < 5 * Qd[i])
                        {
                            Qd[i]++; // TODO: inc/dec amount (relative to Ascale)
                        }
                        else if ((Qd[i] > Qdmin) && (a > 6 * Qd[i]))
                        {
                            Qd[i]--;
                        }
                    }
                    else if (Qd[i] > Qdmin)
                    {
                        // // set a minimum level for Qd
                        Qd[i]--;
                    }
                }
                else if (a < -Qd[i])
                {
                    Qm[i] -= Qd[i] / (Tau_m0 * 2);
                }

                Qms[currQmsPosition % (maxSl + spikePeakDuration)][i] = Qm[i];

                // // should tCut be subtracted here??
                // calc against updated Qm
                a = (traceBuffer[t * nChannels + i] - Aglobal[t - tCut]) * Ascale - Qm[i];

                // // TREATMENT OF THRESHOLD CROSSINGS
                if (Sl[i] > 0)
                {
                    // // Sl frames after peak value
                    // // increment Sl[i]
                    Sl[i]++;
                    if (Sl[i] < minSl)
                    {
                        // calculate area under first and second frame
                        // after spike
                        SpkArea[i] += a;
                    }
                    else if (a < AHPthr * Qd[i])
                    {
                        // // check whether it does repolarize
                        AHP[i] = true;
                    }

                    if ((Sl[i] == maxSl) && AHP[i])
                    {
                        if (2 * SpkArea[i] > minSl * minAvgAmp * Qd[i])
                        {
                            Spike spike = Spike(t0 - maxSl + t - tCut + 1, i, Amp[i]);
                            if (t - tCut - maxSl + 1 > 0)
                            {
                                spike.aGlobal = Aglobal[t - tCut - maxSl + 1];
                            }
                            else
                            {
                                spike.aGlobal = Aglobal[t - tCut];
                            }
                            int *tmp = Qms[(currQmsPosition + 1) % (maxSl + spikePeakDuration)];
                            spike.baselines = vector<int>(tmp, tmp + nChannels);

                            pQueue->add(spike);
                        }
                        Sl[i] = 0;
                    }
                    else if (Amp[i] < a)
                    {
                        // // check whether current ADC count is higher
                        Sl[i] = 1; // // reset peak value
                        Amp[i] = a;
                        AHP[i] = false;  // // reset AHP
                        SpkArea[i] += a; // // not resetting this one (anyway don't need to care if the spike is wide)
                    }
                }
                else if (a > threshold * Qd[i] / 2)
                {
                    // // check for threshold crossings
                    Sl[i] = 1;
                    Amp[i] = a;
                    AHP[i] = false;
                    SpkArea[i] = a;
                }

            } // for i

        } // for t

    } // Detection::Iterate

    // // write spikes in interval after last recalibration; close file
    void Detection::FinishDetection()
    {
        pQueue->close();
    }

} // namespace HSDetection
