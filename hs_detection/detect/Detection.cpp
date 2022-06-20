#include <algorithm>
#include <cstring>
#include "Detection.h"

using namespace std;

namespace HSDetection
{
    Detection::Detection(int chunkSize, int *positionMatrix,
                         int numChannels, int spikePeakDuration, string filename,
                         int noiseDuration, float noiseAmpPercent, float neighborRadius, float innerRadius,
                         int numComCenters, bool localize,
                         int threshold, int cutoutStart, int cutoutEnd, int minAvgAmp,
                         int ahpthr, int spikeLen, int peakLen, bool decayFiltering, bool saveShape,
                         int chunkLeftMargin)
        : numChannels(numChannels), threshold(threshold), minAvgAmp(minAvgAmp),
          maxAHPAmp(ahpthr), spikeLen(spikeLen), peakLen(peakLen),
          chunkLeftMargin(chunkLeftMargin), filename(filename), result(),
          localize(localize), saveShape(saveShape), cutoutStart(cutoutStart),
          cutoutLen(cutoutStart + cutoutEnd + 1), // TODO: +1?
          noiseDuration(noiseDuration), spikePeakDuration(spikePeakDuration),
          numCoMCenters(numComCenters),
          probeLayout(numChannels, positionMatrix, neighborRadius, innerRadius),
          trace(chunkLeftMargin, numChannels, chunkSize),
          commonRef(chunkLeftMargin, 1, chunkSize),
          runningBaseline(numChannels), runningVariance(numChannels),
          decayFilter(decayFiltering), noiseAmpPercent(noiseAmpPercent)
    {
        spikeTime = new int[numChannels];
        hasAHP = new bool[numChannels];
        spikeAmp = new int[numChannels];
        spikeArea = new int[numChannels];

        _commonRef = new short[chunkSize + chunkLeftMargin];

        fill_n(runningBaseline[-1], numChannels, Voffset);
        fill_n(runningVariance[-1], numChannels, Qvstart);

        memset(spikeTime, 0, numChannels * sizeof(int));      // TODO: 0 init?
        memset(hasAHP, 0, numChannels * sizeof(bool));    // TODO: 0 init?
        memset(spikeAmp, 0, numChannels * sizeof(int));     // TODO: 0 init?
        memset(spikeArea, 0, numChannels * sizeof(int)); // TODO: 0 init?

        memset(_commonRef, 0, (chunkSize + chunkLeftMargin) * sizeof(short)); // TODO: 0 init? // really need?

        pQueue = new SpikeQueue(this); // all the params should be ready
    }

    Detection::~Detection()
    {
        delete[] spikeTime;
        delete[] hasAHP;
        delete[] spikeAmp;
        delete[] spikeArea;

        delete[] _commonRef;

        delete pQueue;
    }

    void Detection::commonMedian(int chunkStart, int chunkLen) // TODO: add, use?
    {
    }

    void Detection::commonAverage(int chunkStart, int chunkLen)
    {
        // // TODO: if median takes too long...
        // // or there are only few
        // // channnels (?)
        for (int t = chunkStart - chunkLeftMargin; t < chunkStart + chunkLen; t++)
        {
            int sum = 0;
            for (int i = 0; i < numChannels; i++)
            {
                sum += trace(t, i) / 64; // TODO: no need to scale
            }
            commonRef(t, 0) = sum / (numChannels + 1) * 64; // TODO: no need +1
        }
        // TODO: ???
        // for (int t = chunkStart - 1; t >= chunkStart - chunkLeftMargin; t--)
        // {
        //     // TODO: why? but keep behaviour
        //     // TODO: but exactly kept at t==chunkStart
        //     commonRef(t, 0) = commonRef(t + spikeLen - 1, 0);
        // }
    }

    void Detection::step(short *traceBuffer, int chunkStart, int chunkLen)
    {
        trace.updateChunk(traceBuffer);
        commonRef.updateChunk(_commonRef);

        if (numChannels >= 20) // TODO: magic number?
        {
            commonAverage(chunkStart, chunkLen);
        }

        // // TODO: Does this need to end at chunkLen + chunkLeftMargin? (Cole+Martino)
        for (int t = chunkStart; t - chunkStart < chunkLen; t++)
        {
            short *QbPrev = runningBaseline[t - 1];
            short *QvPrev = runningVariance[t - 1];
            short *Qb = runningBaseline[t];
            short *Qv = runningVariance[t];
            short *input = trace[t];
            int aglobal = commonRef(t, 0);
            for (int i = 0; i < numChannels; i++)
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

        // // TODO: Does this need to end at chunkLen + chunkLeftMargin? (Cole+Martino)
        for (int t = chunkStart; t - chunkStart < chunkLen; t++)
        {
            short *Qb = runningBaseline[t];
            short *Qv = runningVariance[t];
            for (int i = 0; i < numChannels; i++)
            {
                // // TODO: should chunkLeftMargin be subtracted here??
                // calc against updated Qb
                int a = trace(t, i) - commonRef(t, 0) - Qb[i];
                int Qvv = Qv[i];

                if (a > threshold * Qvv / 2 && spikeTime[i] == 0) // TODO: why /2
                {
                    // // check for threshold crossings
                    spikeTime[i] = 1;
                    spikeAmp[i] = a;
                    hasAHP[i] = false;
                    spikeArea[i] = a;
                }
                else if (spikeTime[i] > 0)
                {
                    // // spikeTime frames after peak value
                    // // increment spikeTime[i]
                    spikeTime[i]++;
                    if (spikeTime[i] < peakLen)
                    {
                        // // calculate area under first and second frame
                        // // after spike
                        spikeArea[i] += a;
                    }
                    else if (a < maxAHPAmp * Qvv)
                    {
                        // // check whether it does repolarize
                        hasAHP[i] = true;
                    }

                    if ((spikeTime[i] == spikeLen) && hasAHP[i])
                    {
                        if (2 * spikeArea[i] > peakLen * minAvgAmp * Qvv) // TODO: why *2
                        {
                            pQueue->push_back(Spike(t - spikeLen + 1, i, spikeAmp[i]));
                        }
                        spikeTime[i] = 0;
                    }
                    else if (spikeAmp[i] < a)
                    {
                        // // check whether current ADC count is higher
                        spikeTime[i] = 1; // // reset peak value
                        spikeAmp[i] = a;
                        hasAHP[i] = false;  // // reset hasAHP
                        spikeArea[i] += a; // // not resetting this one (anyway don't need to care if the spike is wide)
                    }
                }

            } // for i

        } // for t

    } // Detection::step

    int Detection::finish()
    {
        pQueue->finalize();
        return result.size();
    }

    char *Detection::getResult()
    {
        return result.data();
    }

} // namespace HSDetection
