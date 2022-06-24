#include <algorithm>
#include <numeric>

#include "Detection.h"

using namespace std;

namespace HSDetection
{
    Detection::Detection(IntChannel numChannels, IntFrame chunkSize, IntFrame chunkLeftMargin, bool medianReference,
                         IntFrame spikeDur, IntFrame ampAvgDur, IntVolt threshold, IntVolt minAvgAmp, IntVolt maxAHPAmp,
                         const FloatGeom *channelPositions, FloatGeom neighborRadius, FloatGeom innerRadius,
                         IntFrame jitterTol, IntFrame peakDur,
                         bool decayFiltering, FloatRatio decayRatio, bool localize,
                         bool saveShape, string filename, IntFrame cutoutStart, IntFrame cutoutLen)
        : trace(chunkLeftMargin, numChannels, chunkSize),
          numChannels(numChannels), chunkSize(chunkSize), chunkLeftMargin(chunkLeftMargin),
          medianReference(medianReference), commonRef(chunkLeftMargin, 1, chunkSize),
          _commonRef(new IntVolt[chunkSize + chunkLeftMargin]),
          runningBaseline(numChannels), runningDeviation(numChannels),
          spikeTime(new IntFrame[numChannels]), spikeAmp(new IntVolt[numChannels]),
          spikeArea(new IntFxV[numChannels]), hasAHP(new bool[numChannels]),
          spikeDur(spikeDur), ampAvgDur(ampAvgDur), threshold(threshold),
          minAvgAmp(minAvgAmp), maxAHPAmp(maxAHPAmp), // pQueue not ready
          probeLayout(numChannels, channelPositions, neighborRadius, innerRadius),
          result(), jitterTol(jitterTol), peakDur(peakDur),
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

    void Detection::commonMedian(IntFrame chunkStart, IntFrame chunkLen)
    {
        copy_n(commonRef[chunkStart + chunkSize - chunkLeftMargin], chunkLeftMargin,
               commonRef[chunkStart - chunkLeftMargin]);

        IntVolt *frame = new IntVolt[numChannels]; // nth_element modifies container
        IntChannel mid = numChannels / 2 - 1;      // TODO: no need - 1

        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            copy_n(trace[t], numChannels, frame);

            nth_element(frame, frame + mid, frame + numChannels);

            commonRef(t, 0) = frame[mid];
        }

        delete[] frame;
    }

    void Detection::commonAverage(IntFrame chunkStart, IntFrame chunkLen)
    {
        copy_n(commonRef[chunkStart + chunkSize - chunkLeftMargin], chunkLeftMargin,
               commonRef[chunkStart - chunkLeftMargin]);

        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            commonRef(t, 0) = accumulate(trace[t], trace[t] + numChannels, (IntCxV)0, // TODO: 64?
                                         [](IntCxV sum, IntVolt data)
                                         { return sum + data / 64; }) /
                              numChannels * 64;
        }
    }

    void Detection::step(IntVolt *traceBuffer, IntFrame chunkStart, IntFrame chunkLen)
    {
        trace.updateChunk(traceBuffer);
        commonRef.updateChunk(_commonRef);

        if (numChannels >= 20) // TODO: magic number?
        {
            if (medianReference)
            {
                commonMedian(chunkStart, chunkLen);
            }
            else
            {
                commonAverage(chunkStart, chunkLen);
            }
        }

        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            const IntVolt *input = trace[t];
            IntVolt ref = commonRef(t, 0);
            const IntVolt *basePrev = runningBaseline[t - 1];
            const IntVolt *devPrev = runningDeviation[t - 1];
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
            const IntVolt *input = trace[t];
            IntVolt ref = commonRef(t, 0);
            const IntVolt *baselines = runningBaseline[t];
            const IntVolt *deviations = runningDeviation[t];

            for (IntChannel i = 0; i < numChannels; i++)
            {
                IntVolt volt = input[i] - ref - baselines[i]; // calc against updated baselines
                IntVolt devI = deviations[i];

                if (spikeTime[i] < 0) // not in spike
                {
                    if (2 * volt > threshold * devI) // threshold crossing, *2 for precision
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

                if (spikeTime[i] < ampAvgDur - 1) // sum up area in ampAvgDur // TODO: inconsistent - 1
                {
                    spikeArea[i] += volt;
                    if (spikeAmp[i] < volt) // larger amp found
                    {
                        spikeTime[i] = 0; // reset peak to current
                        spikeAmp[i] = volt;
                        spikeArea[i] += volt; // but accumulate area // TODO: should not add twice
                        hasAHP[i] = false;
                    }
                    continue;
                }
                // else: ampAvgDur - 1 <= spikeTime[i]

                if (spikeTime[i] < spikeDur - 1) // TODO: inconsistent - 1
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
                // else: spikeTime[i] == spikeDur - 1, spike end

                // TODO: if not AHP, whether connect spike?
                if (2 * spikeArea[i] > (IntFxV)ampAvgDur * minAvgAmp * devI && // reach min area, *2 for precision
                    (hasAHP[i] || volt < maxAHPAmp * devI))                    // AHP exist
                {
                    pQueue->push_back(Spike(t - (spikeDur - 1), i, spikeAmp[i]));
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
