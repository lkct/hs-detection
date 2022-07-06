#include <algorithm>
#include <numeric>

#include "Detection.h"

using namespace std;

namespace HSDetection
{
    Detection::Detection(IntChannel numChannels, IntFrame chunkSize, IntFrame chunkLeftMargin,
                         bool rescale, const FloatRaw *scale, const FloatRaw *offset,
                         bool medianReference, bool averageReference,
                         IntFrame spikeDur, IntFrame ampAvgDur,
                         FloatRatio threshold, FloatRatio minAvgAmp, FloatRatio maxAHPAmp,
                         const FloatGeom *channelPositions, FloatGeom neighborRadius, FloatGeom innerRadius,
                         IntFrame jitterTol, IntFrame riseDur,
                         bool decayFiltering, FloatRatio decayRatio, bool localize,
                         bool saveShape, string filename, IntFrame cutoutStart, IntFrame cutoutEnd)
        : traceRaw(chunkLeftMargin, numChannels, chunkSize),
          numChannels(numChannels), chunkSize(chunkSize), chunkLeftMargin(chunkLeftMargin),
          rescale(rescale),
          scale(new (align_val_t(channelAlign * sizeof(IntVolt))) FloatRaw[alignChannel(numChannels)]),
          offset(new (align_val_t(channelAlign * sizeof(IntVolt))) FloatRaw[alignChannel(numChannels)]),
          trace(chunkSize + chunkLeftMargin, alignChannel(numChannels)),
          medianReference(medianReference), averageReference(averageReference),
          commonRef(chunkSize + chunkLeftMargin, 1),
          runningBaseline(chunkSize + chunkLeftMargin, alignChannel(numChannels)),
          runningDeviation(chunkSize + chunkLeftMargin, alignChannel(numChannels)),
          spikeTime(new IntFrame[numChannels]), spikeAmp(new IntVolt[numChannels]),
          spikeArea(new IntCalc[numChannels]), hasAHP(new bool[numChannels]),
          spikeDur(spikeDur), ampAvgDur(ampAvgDur), threshold(threshold * thrQuant),
          minAvgAmp(minAvgAmp * thrQuant), maxAHPAmp(maxAHPAmp * thrQuant),
          probeLayout(numChannels, channelPositions, neighborRadius, innerRadius),
          result(), jitterTol(jitterTol), riseDur(riseDur),
          decayFilter(decayFiltering), decayRatio(decayRatio), localize(localize),
          saveShape(saveShape), filename(filename), cutoutStart(cutoutStart), cutoutEnd(cutoutEnd)
    {
        fill_n(this->scale, alignChannel(numChannels), (FloatRaw)1);
        fill_n(this->offset, alignChannel(numChannels), (FloatRaw)0);

        if (rescale)
        {
            copy_n(scale, numChannels, this->scale);
            copy_n(offset, numChannels, this->offset);
        }

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

        delete[] scale;
        delete[] offset;
    }

    void Detection::step(FloatRaw *traceBuffer, IntFrame chunkStart, IntFrame chunkLen)
    {
        traceRaw.updateChunk(traceBuffer);

        if (rescale)
        {
            traceScaleCast(chunkStart, chunkLen);
        }
        else
        {
            traceCast(chunkStart, chunkLen);
        }

        if (medianReference)
        {
            commonMedian(chunkStart, chunkLen);
        }
        else if (averageReference)
        {
            commonAverage(chunkStart, chunkLen);
        }

        runningEstimation(chunkStart, chunkLen);

        detectSpikes(chunkStart, chunkLen);
    }

    IntResult Detection::finish()
    {
        pQueue->finalize();
        return result.size();
    }

    const Spike *Detection::getResult() const
    {
        return result.data();
    }

    void Detection::traceScaleCast(IntFrame chunkStart, IntFrame chunkLen)
    {
        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            const FloatRaw *input = traceRaw[t];
            IntVolt *trace = this->trace[t];
            IntChannel alignedChannels = alignChannel(numChannels);

            for (IntChannel i = 0; i < alignedChannels; i++)
            {
                trace[i] = input[i] * scale[i] + offset[i];
            }
        }
    }

    void Detection::traceCast(IntFrame chunkStart, IntFrame chunkLen)
    {
        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            const FloatRaw *input = traceRaw[t];
            IntVolt *trace = this->trace[t];
            IntChannel alignedChannels = alignChannel(numChannels);

            for (IntChannel i = 0; i < alignedChannels; i++)
            {
                trace[i] = input[i];
            }
        }
    }

    void Detection::commonMedian(IntFrame chunkStart, IntFrame chunkLen)
    {
        IntVolt *frame = new IntVolt[numChannels]; // nth_element modifies container
        IntChannel mid = numChannels / 2;

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
        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            commonRef(t, 0) = accumulate(trace[t], trace[t] + numChannels, (IntCalc)0,
                                         [](IntCalc sum, IntVolt data)
                                         { return sum + data; }) /
                              numChannels;
        }
    }

    void Detection::runningEstimation(IntFrame chunkStart, IntFrame chunkLen)
    {
        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            const IntVolt *trace = this->trace[t];
            IntVolt ref = commonRef(t, 0);
            const IntVolt *basePrev = runningBaseline[t - 1];
            const IntVolt *devPrev = runningDeviation[t - 1];
            IntVolt *baselines = runningBaseline[t];
            IntVolt *deviations = runningDeviation[t];
            IntChannel alignedChannels = alignChannel(numChannels);

            for (IntChannel i = 0; i < alignedChannels; i++)
            {
                IntVolt volt = trace[i] - ref - basePrev[i];

                IntVolt dltBase = 0;
                dltBase = (devPrev[i] < volt) ? devPrev[i] / tauBase : dltBase;
                dltBase = (volt < -devPrev[i]) ? -devPrev[i] / (tauBase * 2) : dltBase;
                baselines[i] = basePrev[i] + dltBase;

                IntVolt dltDev = 0;
                dltDev = (devPrev[i] < volt && volt < 5 * devPrev[i]) ? devChange : dltDev;
                dltDev = ((0 < volt && volt <= devPrev[i]) || 6 * devPrev[i] < volt) ? -devChange : dltDev;
                IntVolt dev = devPrev[i] + dltDev;
                deviations[i] = (dev < minDev) ? minDev : dev; // clamp deviations at minDev
            }
        }
    }

    void Detection::detectSpikes(IntFrame chunkStart, IntFrame chunkLen)
    {
        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            while (pQueue->checkDelay(t))
            {
                pQueue->process();
            }

            const IntVolt *trace = this->trace[t];
            IntVolt ref = commonRef(t, 0);
            const IntVolt *baselines = runningBaseline[t];
            const IntVolt *deviations = runningDeviation[t];

            for (IntChannel i = 0; i < numChannels; i++)
            {
                IntVolt volt = trace[i] - ref - baselines[i]; // calc against updated baselines
                IntVolt dev = deviations[i];

                IntCalc voltThr = volt * thrQuant;
                IntCalc thr = threshold * dev;
                IntCalc minAvg = minAvgAmp * dev;
                IntCalc maxAHP = maxAHPAmp * dev;

                if (spikeTime[i] < 0) // not in spike
                {
                    if (voltThr > thr) // threshold crossing
                    {
                        spikeTime[i] = 0;
                        spikeAmp[i] = volt;
                        spikeArea[i] = voltThr;
                        hasAHP[i] = false;
                    }
                    continue;
                }
                // else: during a spike
                spikeTime[i]++;
                // 1 <= spikeTime[i]

                if (spikeTime[i] < ampAvgDur) // sum up area in ampAvgDur
                {
                    spikeArea[i] += voltThr;
                    if (spikeAmp[i] < volt) // larger amp found
                    {
                        spikeTime[i] = 0; // reset peak to current
                        spikeAmp[i] = volt;
                        // but accumulate area (already added)
                        hasAHP[i] = false;
                    }
                    continue;
                }
                // else: ampAvgDur <= spikeTime[i]

                if (spikeTime[i] < spikeDur)
                {
                    if (voltThr < maxAHP) // AHP found
                    {
                        hasAHP[i] = true;
                    }
                    else if (spikeAmp[i] < volt) // larger amp found
                    {
                        spikeTime[i] = 0; // reset peak to current
                        spikeAmp[i] = volt;
                        spikeArea[i] += voltThr; // but accumulate area
                        hasAHP[i] = false;
                    }
                    continue;
                }
                // else: spikeTime[i] == spikeDur, spike end

                if (spikeArea[i] > minAvg * ampAvgDur && // reach min area
                    (hasAHP[i] || voltThr < maxAHP))     // AHP exist
                {
                    pQueue->push_back(Spike(t - spikeDur, i, spikeAmp[i]));
                }

                spikeTime[i] = -1; // reset counter even if not spike

            } // for i

        } // for t

    } // Detection::detectSpikes

} // namespace HSDetection
