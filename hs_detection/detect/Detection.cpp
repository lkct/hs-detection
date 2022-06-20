#include <algorithm>
#include <cstring>
#include "Detection.h"

using namespace std;

namespace HSDetection
{
    Detection::Detection(int numChannels, int chunkSize, int chunkLeftMargin,
                         int spikeLen, int peakLen, int threshold, int minAvgAmp, int maxAHPAmp,
                         int *channelPositions, float neighborRadius, float innerRadius,
                         int noiseDuration, int spikePeakDuration,
                         bool decayFiltering, float noiseAmpPercent,
                         bool localize, int numCoMCenters,
                         bool saveShape, string filename, int cutoutStart, int cutoutLen)
        : trace(chunkLeftMargin, numChannels, chunkSize),
          commonRef(chunkLeftMargin, 1, chunkSize),
          runningBaseline(numChannels), runningVariance(numChannels),
          _commonRef(new short[chunkSize + chunkLeftMargin]),
          numChannels(numChannels), chunkSize(chunkSize), chunkLeftMargin(chunkLeftMargin),
          spikeTime(new int[numChannels]), spikeAmp(new int[numChannels]),
          spikeArea(new int[numChannels]), hasAHP(new bool[numChannels]),
          spikeLen(spikeLen), peakLen(peakLen), threshold(threshold),
          minAvgAmp(minAvgAmp), maxAHPAmp(maxAHPAmp), // pQueue not ready
          probeLayout(numChannels, channelPositions, neighborRadius, innerRadius),
          result(), noiseDuration(noiseDuration), spikePeakDuration(spikePeakDuration),
          decayFilter(decayFiltering), noiseAmpPercent(noiseAmpPercent),
          localize(localize), numCoMCenters(numCoMCenters),
          saveShape(saveShape), filename(filename),
          cutoutStart(cutoutStart), cutoutLen(cutoutLen)
    {
        fill_n(runningBaseline[-1], numChannels, Voffset);
        fill_n(runningVariance[-1], numChannels, Qvstart);

        fill_n(spikeTime, numChannels, -1);

        pQueue = new SpikeQueue(this); // all the params should be ready
    }

    Detection::~Detection()
    {
        delete pQueue;

        delete[] spikeTime;
        delete[] spikeAmp;
        delete[] spikeArea;
        delete[] hasAHP;

        delete[] _commonRef;
    }

    void Detection::commonMedian(int chunkStart, int chunkLen) // TODO: add, use?
    {
        // // TODO: if median takes too long...
        // // or there are only few
        // // channnels (?)
        // // then use mean
    }

    void Detection::commonAverage(int chunkStart, int chunkLen)
    {
        copy_n(commonRef[chunkStart + chunkSize - chunkLeftMargin], chunkLeftMargin,
               commonRef[chunkStart - chunkLeftMargin]);

        for (int t = chunkStart; t < chunkStart + chunkLen; t++)
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
        for (int t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            short *input = trace[t];
            short ref = commonRef(t, 0);
            short *QbPrev = runningBaseline[t - 1]; // TODO: name?
            short *QvPrev = runningVariance[t - 1];
            short *Qb = runningBaseline[t];
            short *Qv = runningVariance[t];

            for (int i = 0; i < numChannels; i++)
            {
                short volt = input[i] - ref - QbPrev[i];

                short dltBase = 0;
                dltBase = (QvPrev[i] < volt) ? QvPrev[i] / Tau_m0 : dltBase;
                dltBase = (volt < -QvPrev[i]) ? -QvPrev[i] / (Tau_m0 * 2) : dltBase;
                Qb[i] = QbPrev[i] + dltBase;

                short dltVar = 0;
                dltVar = (QvPrev[i] < volt && volt < 5 * QvPrev[i]) ? QvChange : dltVar;
                dltVar = ((0 < volt && volt <= QvPrev[i]) || 6 * QvPrev[i] < volt) ? -QvChange : dltVar;
                short Qvi = QvPrev[i] + dltVar;
                Qv[i] = (Qvi < Qvmin) ? Qvmin : Qvi; // clamp Qv at Qvmin
            }
        }

        for (int t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            short *input = trace[t];
            short ref = commonRef(t, 0);
            short *Qb = runningBaseline[t];
            short *Qv = runningVariance[t];

            for (int i = 0; i < numChannels; i++)
            {
                // // TODO: should chunkLeftMargin be subtracted here??
                int volt = input[i] - ref - Qb[i]; // calc against updated Qb
                int Qvi = Qv[i];

                if (spikeTime[i] < 0) // not in spike
                {
                    if (volt > threshold * Qvi / 2) // threshold crossing // TODO: why /2
                    {
                        spikeTime[i] = 0;
                        spikeAmp[i] = volt;
                        spikeArea[i] = volt;
                        hasAHP[i] = false;
                    }
                    continue;
                }
                // else: during a spike
                spikeTime[i]++;
                // 1 <= spikeTime[i]

                if (spikeTime[i] < peakLen - 1) // sum up area in peakLen
                {
                    spikeArea[i] += volt;   // TODO: ??? added twice
                    if (spikeAmp[i] < volt) // larger amp found
                    {
                        spikeTime[i] = 0; // reset peak to current
                        spikeAmp[i] = volt;
                        spikeArea[i] += volt; // but accumulate area
                        hasAHP[i] = false;
                    }
                    continue;
                }
                // else: peakLen - 1 <= spikeTime[i]

                if (spikeTime[i] < spikeLen - 1)
                {
                    if (volt < maxAHPAmp * Qvi) // AHP found
                    {
                        hasAHP[i] = true;
                    }
                    else if (spikeAmp[i] < volt) // larger amp found
                    {
                        spikeTime[i] = 0; // reset peak to current
                        spikeAmp[i] = volt;
                        spikeArea[i] += volt; // but accumulate area
                        hasAHP[i] = false;
                    }
                    continue;
                }
                // else: spikeTime[i] == spikeLen - 1, spike end

                // TODO: if not AHP, whether connect spike?
                if (2 * spikeArea[i] > peakLen * minAvgAmp * Qvi && // reach min area // TODO: why *2
                    (hasAHP[i] || volt < maxAHPAmp * Qvi))          // AHP exist
                {
                    pQueue->push_back(Spike(t - spikeLen + 1, i, spikeAmp[i]));
                }

                spikeTime[i] = -1;

            } // for i

        } // for t

    } // Detection::step

    int Detection::finish()
    {
        pQueue->finalize();
        return result.size();
    }

    const Spike *Detection::getResult() const
    {
        return result.data();
    }

} // namespace HSDetection
