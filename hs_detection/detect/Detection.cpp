#include <algorithm>
#include <cstring>
#include <utility>
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

    VoltTrace Detection::trace(0, 0, 0);
    VoltTrace Detection::AGlobal(0, 0, 0);
    ProbeLayout Detection::probeLayout;
    RollingArray Detection::QBs(0);
    RollingArray Detection::QVs(0);

    Detection::Detection(int chunkSize, int *positionMatrix,
                         int nChannels, int spikePeakDuration, string filename,
                         int noiseDuration, float noiseAmpPercent, float neighborRadius, float innerRadius,
                         int numComCenters, bool localize,
                         int threshold, int cutoutStart, int cutoutEnd, int minAvgAmp,
                         int ahpthr, int maxSl, int minSl, bool decayFiltering, bool saveShape,
                         int framesLeftMargin)
        : nChannels(nChannels), threshold(threshold), minAvgAmp(minAvgAmp),
          AHPthr(ahpthr), maxSl(maxSl), minSl(minSl),
          spikePeakDuration(spikePeakDuration),
          framesLeftMargin(framesLeftMargin), filename(filename), result(),
          saveShape(saveShape), cutout_start(cutoutStart), cutout_end(cutoutEnd)
    {
        QBs = move(RollingArray(nChannels));
        QVs = move(RollingArray(nChannels));

        Sl = new int[nChannels];
        AHP = new bool[nChannels];
        Amp = new int[nChannels];
        SpkArea = new int[nChannels];

        _Aglobal = new short[chunkSize + framesLeftMargin];

        fill_n(QBs[-1], nChannels, Voffset);
        fill_n(QVs[-1], nChannels, 400); // TODO: magic number?

        memset(Sl, 0, nChannels * sizeof(int));      // TODO: 0 init?
        memset(AHP, 0, nChannels * sizeof(bool));    // TODO: 0 init?
        memset(Amp, 0, nChannels * sizeof(int));     // TODO: 0 init?
        memset(SpkArea, 0, nChannels * sizeof(int)); // TODO: 0 init?

        memset(_Aglobal, 0, (chunkSize + framesLeftMargin) * sizeof(short)); // TODO: 0 init? // really need?
        AGlobal = VoltTrace(framesLeftMargin, 1, chunkSize);

        // TODO: error check?
        num_com_centers = numComCenters;
        num_channels = nChannels;
        spike_peak_duration = spikePeakDuration;
        noise_duration = noiseDuration;
        noise_amp_percent = noiseAmpPercent;
        to_localize = localize;
        decay_filtering = decayFiltering;

        trace = VoltTrace(framesLeftMargin, nChannels, chunkSize);

        probeLayout = ProbeLayout(nChannels, positionMatrix, neighborRadius, innerRadius);

        pQueue = new SpikeQueue(this); // all the params should be ready
    }

    Detection::~Detection()
    {
        delete[] Sl;
        delete[] AHP;
        delete[] Amp;
        delete[] SpkArea;

        delete[] _Aglobal;

        delete pQueue;
    }

    void Detection::MedianVoltage(short *traceBuffer) // TODO: add, use?
    {
    }

    void Detection::MeanVoltage(short *traceBuffer, int frameInputStart, int framesInputLen)
    {
        // // TODO: if median takes too long...
        // // or there are only few
        // // channnels (?)
        for (int t = frameInputStart - framesLeftMargin; t < frameInputStart + framesInputLen; t++)
        {
            int sum = 0;
            for (int i = 0; i < nChannels; i++)
            {
                sum += trace(t, i) / 64; // TODO: no need to scale
            }
            AGlobal(t, 0) = sum / (nChannels + 1) * 64; // TODO: no need +1
        }
        // TODO: ???
        // for (int t = frameInputStart - 1; t >= frameInputStart - framesLeftMargin; t--)
        // {
        //     // TODO: why? but keep behaviour
        //     // TODO: but exactly kept at t==frameInputStart
        //     AGlobal(t, 0) = AGlobal(t + maxSl - 1, 0);
        // }
    }

    void Detection::Iterate(short *traceBuffer, int frameInputStart, int framesInputLen)
    {
        trace.updateChunk(traceBuffer);
        AGlobal.updateChunk(_Aglobal);

        if (nChannels >= 20) // TODO: magic number?
        {
            MeanVoltage(traceBuffer, frameInputStart, framesInputLen);
        }

        // // TODO: Does this need to end at framesInputLen + framesLeftMargin? (Cole+Martino)
        for (int t = frameInputStart; t - frameInputStart < framesInputLen; t++)
        {
            short *QbPrev = QBs[t - 1];
            short *QvPrev = QVs[t - 1];
            short *Qb = QBs[t];
            short *Qv = QVs[t];
            short *input = trace[t];
            int aglobal = AGlobal(t, 0);
            for (int i = 0; i < nChannels; i++)
            {
                int a = input[i] - aglobal - QbPrev[i];

                int dltQb = 0;
                if (a < -QvPrev[i])
                {
                    dltQb = -QvPrev[i] / (Tau_m0 * 2);
                }
                else if (QvPrev[i] < a)
                {
                    dltQb = QvPrev[i] / Tau_m0;
                }

                int dltQv = 0;
                if (QvPrev[i] < a && a < 5 * QvPrev[i])
                {
                    dltQv = QvChange;
                }
                else if ((0 < a && a <= QvPrev[i]) || 6 * QvPrev[i] < a)
                {
                    dltQv = -QvChange;
                }

                Qb[i] = QbPrev[i] + dltQb;
                Qv[i] = QvPrev[i] + dltQv;

                // set a minimum level for Qv
                if (Qv[i] < Qvmin)
                {
                    Qv[i] = Qvmin;
                }
            } // for i

        } // for t

        // // TODO: Does this need to end at framesInputLen + framesLeftMargin? (Cole+Martino)
        for (int t = frameInputStart; t - frameInputStart < framesInputLen; t++)
        {
            short *Qb = QBs[t];
            short *Qv = QVs[t];
            for (int i = 0; i < nChannels; i++)
            {
                // // TODO: should framesLeftMargin be subtracted here??
                // calc against updated Qb
                int a = trace(t, i) - AGlobal(t, 0) - Qb[i];
                int Qvv = Qv[i];

                if (a > threshold * Qvv / 2 && Sl[i] == 0) // TODO: why /2
                {
                    // // check for threshold crossings
                    Sl[i] = 1;
                    Amp[i] = a;
                    AHP[i] = false;
                    SpkArea[i] = a;
                }
                else if (Sl[i] > 0)
                {
                    // // Sl frames after peak value
                    // // increment Sl[i]
                    Sl[i]++;
                    if (Sl[i] < minSl)
                    {
                        // // calculate area under first and second frame
                        // // after spike
                        SpkArea[i] += a;
                    }
                    else if (a < AHPthr * Qvv)
                    {
                        // // check whether it does repolarize
                        AHP[i] = true;
                    }

                    if ((Sl[i] == maxSl) && AHP[i])
                    {
                        if (2 * SpkArea[i] > minSl * minAvgAmp * Qvv)
                        {
                            Spike spike = Spike(t - maxSl + 1, i, Amp[i]);
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

            } // for i

        } // for t

    } // Detection::Iterate

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
