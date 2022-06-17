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
    bool Detection::to_localize = 0;
    bool Detection::decay_filtering = 0;
    int Detection::maxsl = 0;
    int Detection::cutout_start = 0;
    int Detection::cutout_end = 0;
    int Detection::cutout_size = 0;

    VoltTrace Detection::trace(0, 0, 0);
    ProbeLayout Detection::probeLayout;

    Detection::Detection(int chunkSize, int *positionMatrix,
                         int nChannels, int spikePeakDuration, string filename,
                         int noiseDuration, float noiseAmpPercent, float neighborRadius, float innerRadius,
                         int numComCenters, bool localize,
                         int threshold, int cutoutStart, int cutoutEnd, int minAvgAmp,
                         int ahpthr, int maxSl, int minSl, bool decayFiltering, bool saveShape,
                         int framesLeftMargin)
        : nChannels(nChannels), threshold(threshold), minAvgAmp(minAvgAmp),
          AHPthr(ahpthr), maxSl(maxSl), minSl(minSl),
          currQmsPosition(0), spikePeakDuration(spikePeakDuration),
          framesLeftMargin(framesLeftMargin), filename(filename), result(),
          saveShape(saveShape)
    {
        Qv = new int[nChannels];
        Qb = new int[nChannels];
        QbsLen = spikePeakDuration + maxSl + 10000; // TODO: qms setting?
        Qbs = new int *[QbsLen];
        for (int i = 0; i < QbsLen; i++)
        {
            Qbs[i] = new int[nChannels];
        }

        Sl = new int[nChannels];
        AHP = new bool[nChannels];
        Amp = new int[nChannels];
        SpkArea = new int[nChannels];

        Aglobal = new int[chunkSize];

        fill_n(Qv, nChannels, 400); // TODO: magic number?
        fill_n(Qb, nChannels, Voffset * Ascale);

        memset(Sl, 0, nChannels * sizeof(int));      // TODO: 0 init?
        memset(AHP, 0, nChannels * sizeof(bool));    // TODO: 0 init?
        memset(Amp, 0, nChannels * sizeof(int));     // TODO: 0 init?
        memset(SpkArea, 0, nChannels * sizeof(int)); // TODO: 0 init?

        memset(Aglobal, 0, chunkSize * sizeof(int)); // TODO: 0 init?

        // TODO: error check?
        num_com_centers = numComCenters;
        num_channels = nChannels;
        spike_peak_duration = spikePeakDuration;
        noise_duration = noiseDuration;
        noise_amp_percent = noiseAmpPercent;
        to_localize = localize;
        decay_filtering = decayFiltering;
        maxsl = maxSl;
        cutout_start = cutoutStart;
        cutout_end = cutoutEnd;
        cutout_size = cutoutStart + cutoutEnd + 1;

        trace = VoltTrace(framesLeftMargin, nChannels, chunkSize);

        probeLayout = ProbeLayout(nChannels, positionMatrix, neighborRadius, innerRadius);

        pQueue = new SpikeQueue(this); // all the params should be ready
    }

    Detection::~Detection()
    {
        delete[] Qv;
        delete[] Qb;
        for (int i = 0; i < QbsLen; i++)
        {
            delete[] Qbs[i];
        }
        delete[] Qbs;

        delete[] Sl;
        delete[] AHP;
        delete[] Amp;
        delete[] SpkArea;

        delete[] Aglobal;

        delete pQueue;
    }

    void Detection::MedianVoltage(short *traceBuffer) // TODO: add, use?
    {
    }

    void Detection::MeanVoltage(short *traceBuffer, int frameInputStart, int framesInputLen)
    {
        // // if median takes too long...
        // // or there are only few
        // // channnels (?)
        for (int t = frameInputStart; t < frameInputStart + framesInputLen; t++)
        {
            int sum = 0;
            for (int i = 0; i < nChannels; i++)
            {
                sum += trace(t, i);
            }
            Aglobal[t - frameInputStart] = sum / (nChannels + 1); // TODO: no need +1
        }
    }

    void Detection::Iterate(short *traceBuffer, int frameInputStart, int framesInputLen)
    {
        trace.updateChunk(traceBuffer);

        if (nChannels >= 20) // TODO: magic number?
        {
            MeanVoltage(traceBuffer, frameInputStart, framesInputLen);
        }

        // // TODO: Does this need to end at framesInputLen + framesLeftMargin? (Cole+Martino)
        for (int t = frameInputStart; t - frameInputStart + framesLeftMargin < framesInputLen; t++, currQmsPosition++)
        {
            for (int i = 0; i < nChannels; i++)
            {
                int a = (trace(t, i) - Aglobal[t - frameInputStart]) * Ascale - Qb[i];

                int cases = -1;
                if (a < -Qv[i])
                    cases = 0;
                if (0 < a && a <= Qv[i])
                    cases = 1;
                if (Qv[i] < a && a < 5 * Qv[i])
                    cases = 2;
                if (5 * Qv[i] <= a && a <= 6 * Qv[i])
                    cases = 3;
                if (6 * Qv[i] < a)
                    cases = 4;

                if (cases == 0)
                {
                    Qb[i] -= Qv[i] / (Tau_m0 * 2);
                }
                if (cases == 1)
                {
                    Qv[i] -= QvChange;
                }
                if (cases == 2)
                {
                    Qb[i] += Qv[i] / Tau_m0;
                    Qv[i] += QvChange;
                }
                if (cases == 3)
                {
                    Qb[i] += Qv[i] / Tau_m0;
                }
                if (cases == 4)
                {
                    Qb[i] += Qv[i] / Tau_m0;
                    Qv[i] -= QvChange;
                }

                // set a minimum level for Qv
                if (Qv[i] < Qvmin)
                {
                    Qv[i] = Qvmin;
                }

                Qbs[currQmsPosition % QbsLen][i] = Qb[i];

                // // TODO: should framesLeftMargin be subtracted here??
                // calc against updated Qb
                a = (trace(t, i) - Aglobal[t - frameInputStart]) * Ascale - Qb[i];

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
                    else if (a < AHPthr * Qv[i])
                    {
                        // // check whether it does repolarize
                        AHP[i] = true;
                    }

                    if ((Sl[i] == maxSl) && AHP[i])
                    {
                        if (2 * SpkArea[i] > minSl * minAvgAmp * Qv[i])
                        {
                            int tSpike = t - maxSl + 1;
                            Spike spike = Spike(tSpike, i, Amp[i]);
                            if (tSpike - frameInputStart > 0)
                            {
                                spike.aGlobal = Aglobal[tSpike - frameInputStart];
                            }
                            else
                            {
                                // TODO: what if Sl carried to the next chunk?
                                spike.aGlobal = Aglobal[t - frameInputStart];
                            }
                            int *tmp = Qbs[(currQmsPosition - (maxsl + spikePeakDuration - 1) + QbsLen) % QbsLen];
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
                else if (a > threshold * Qv[i] / 2) // TODO: why /2
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
    int Detection::FinishDetection()
    {
        pQueue->close();
        return result.size();
    }

    char *Detection::Get()
    {
        return result.data();
    }

} // namespace HSDetection
