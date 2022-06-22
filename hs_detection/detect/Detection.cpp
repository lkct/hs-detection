#include <algorithm>

#include "Detection.h"

using namespace std;

namespace HSDetection
{
    Detection::Detection(IntChannel numChannels, IntFrame chunkSize, IntFrame chunkLeftMargin,
                         IntFrame spikeLen, IntFrame ampAvgLen, IntVolt threshold, IntVolt minAvgAmp, IntVolt maxAHPAmp,
                         FloatGeom *channelPositions, FloatGeom neighborRadius, FloatGeom innerRadius,
                         IntFrame jitterTol, IntFrame peakLen,
                         bool decayFiltering, float decayRatio, bool localize,
                         bool saveShape, string filename, IntFrame cutoutStart, IntFrame cutoutLen)
        : trace(chunkLeftMargin, numChannels, chunkSize),
          commonRef(chunkLeftMargin, 1, chunkSize),
          runningBaseline(numChannels), runningDeviation(numChannels),
          _commonRef(new IntVolt[chunkSize + chunkLeftMargin]),
          numChannels(numChannels), chunkSize(chunkSize), chunkLeftMargin(chunkLeftMargin),
          spikeTime(new IntFrame[numChannels]), spikeAmp(new IntVolt[numChannels]),
          spikeArea(new IntFxV[numChannels]), hasAHP(new bool[numChannels]),
          spikeLen(spikeLen), ampAvgLen(ampAvgLen), threshold(threshold),
          minAvgAmp(minAvgAmp), maxAHPAmp(maxAHPAmp), // pQueue not ready
          probeLayout(numChannels, channelPositions, neighborRadius, innerRadius),
          result(), jitterTol(jitterTol), peakLen(peakLen),
          decayFilter(decayFiltering), decayRatio(decayRatio),
          localize(localize), saveShape(saveShape), filename(filename),
          cutoutStart(cutoutStart), cutoutLen(cutoutLen)
    {
        fill_n(runningBaseline[-1], numChannels, initBase);
        fill_n(runningDeviation[-1], numChannels, initDev);

        fill_n(spikeTime, numChannels, (IntFrame)-1);

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

    void Detection::commonMedian(IntFrame chunkStart, IntFrame chunkLen) // TODO: add, use?
    {
        // // TODO: if median takes too long...
        // // or there are only few
        // // channnels (?)
        // // then use mean
    }

    void Detection::commonAverage(IntFrame chunkStart, IntFrame chunkLen)
    {
        copy_n(commonRef[chunkStart + chunkSize - chunkLeftMargin], chunkLeftMargin,
               commonRef[chunkStart - chunkLeftMargin]);

        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            IntCxV sum = 0;
            for (IntChannel i = 0; i < numChannels; i++)
            {
                sum += trace(t, i) / 64; // TODO: no need to scale
            }
            commonRef(t, 0) = sum / numChannels * 64;
        }
    }

    void Detection::step(IntVolt *traceBuffer, IntFrame chunkStart, IntFrame chunkLen)
    {
        trace.updateChunk(traceBuffer);
        commonRef.updateChunk(_commonRef);

        if (numChannels >= 20) // TODO: magic number?
        {
            commonAverage(chunkStart, chunkLen);
        }

        // // TODO: Does this need to end at chunkLen + chunkLeftMargin? (Cole+Martino)
        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            IntVolt *input = trace[t];
            IntVolt ref = commonRef(t, 0);
            IntVolt *basePrev = runningBaseline[t - 1]; // TODO: name?
            IntVolt *devPrev = runningDeviation[t - 1];
            IntVolt *baselines = runningBaseline[t];
            IntVolt *deviations = runningDeviation[t];

            for (IntChannel i = 0; i < numChannels; i++)
            {
                IntVolt volt = input[i] - ref - basePrev[i];

                IntVolt dltBase = 0;
                dltBase = (devPrev[i] < volt) ? devPrev[i] / tauBase : dltBase;
                dltBase = (volt < -devPrev[i]) ? -devPrev[i] / (tauBase * 2) : dltBase;
                baselines[i] = basePrev[i] + dltBase;

                IntVolt dltDev = 0;
                dltDev = (devPrev[i] < volt && volt < 5 * devPrev[i]) ? devChange : dltDev;
                dltDev = ((0 < volt && volt <= devPrev[i]) || 6 * devPrev[i] < volt) ? -devChange : dltDev; // TODO: split two cmov?
                IntVolt devI = devPrev[i] + dltDev;
                deviations[i] = (devI < minDev) ? minDev : devI; // clamp deviations at minDev
            }
        }

        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            IntVolt *input = trace[t];
            IntVolt ref = commonRef(t, 0);
            IntVolt *baselines = runningBaseline[t];
            IntVolt *deviations = runningDeviation[t];

            for (IntChannel i = 0; i < numChannels; i++)
            {
                // // TODO: should chunkLeftMargin be subtracted here??
                IntVolt volt = input[i] - ref - baselines[i]; // calc against updated baselines
                IntVolt devI = deviations[i];

                if (spikeTime[i] < 0) // not in spike
                {
                    if (volt > threshold * devI / 2) // threshold crossing // TODO: why /2
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

                if (spikeTime[i] < ampAvgLen - 1) // sum up area in ampAvgLen // TODO: inconsistent - 1
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
                // else: ampAvgLen - 1 <= spikeTime[i]

                if (spikeTime[i] < spikeLen - 1) // TODO: inconsistent - 1
                {
                    if (volt < maxAHPAmp * devI) // AHP found
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
                if (2 * spikeArea[i] > (IntFxV)ampAvgLen * minAvgAmp * devI && // reach min area // TODO: why *2
                    (hasAHP[i] || volt < maxAHPAmp * devI))                    // AHP exist
                {
                    pQueue->push_back(Spike(t - (spikeLen - 1), i, spikeAmp[i]));
                }

                spikeTime[i] = -1;

            } // for i

        } // for t

    } // Detection::step

    IntResult Detection::finish()
    {
        pQueue->finalize();
        return result.size();
    }

    const Spike *Detection::getResult() const
    {
        return result.data();
    }

} // namespace HSDetection
