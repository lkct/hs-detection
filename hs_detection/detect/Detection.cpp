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
          numChannels(numChannels), alignedChannels(alignChannel(numChannels)),
          chunkSize(chunkSize), chunkLeftMargin(chunkLeftMargin), rescale(rescale),
          scale(new (align_val_t(channelAlign * sizeof(IntVolt))) FloatRaw[alignedChannels * channelAlign]),
          offset(new (align_val_t(channelAlign * sizeof(IntVolt))) FloatRaw[alignedChannels * channelAlign]),
          trace(chunkSize + chunkLeftMargin, alignedChannels * channelAlign),
          medianReference(medianReference), averageReference(averageReference),
          commonRef(chunkSize + chunkLeftMargin, 1),
          runningBaseline(chunkSize + chunkLeftMargin, alignedChannels * channelAlign),
          runningDeviation(chunkSize + chunkLeftMargin, alignedChannels * channelAlign),
          spikeTime(new IntFrame[numChannels]), spikeAmp(new IntVolt[numChannels]),
          spikeArea(new IntCalc[numChannels]), hasAHP(new bool[numChannels]),
          spikeDur(spikeDur), ampAvgDur(ampAvgDur), threshold(threshold * thrQuant),
          minAvgAmp(minAvgAmp * thrQuant), maxAHPAmp(maxAHPAmp * thrQuant),
          probeLayout(numChannels, channelPositions, neighborRadius, innerRadius),
          result(), jitterTol(jitterTol), riseDur(riseDur),
          decayFilter(decayFiltering), decayRatio(decayRatio), localize(localize),
          saveShape(saveShape), filename(filename), cutoutStart(cutoutStart), cutoutEnd(cutoutEnd)
    {
        fill_n(this->scale, alignedChannels * channelAlign, (FloatRaw)1);
        fill_n(this->offset, alignedChannels * channelAlign, (FloatRaw)0);
        if (rescale)
        {
            copy_n(scale, numChannels, this->scale);
            copy_n(offset, numChannels, this->offset);
        }

        fill_n(runningBaseline[-1], alignedChannels * channelAlign, initBase);
        fill_n(runningDeviation[-1], alignedChannels * channelAlign, initDev);

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

        operator delete[](scale, align_val_t(channelAlign * sizeof(IntVolt)));
        operator delete[](offset, align_val_t(channelAlign * sizeof(IntVolt)));
    }

    void Detection::step(FloatRaw *traceBuffer, IntFrame chunkStart, IntFrame chunkLen)
    {
        traceRaw.updateChunk(traceBuffer);

        castAndCommonref(chunkStart, chunkLen);

        estimateAndDetect(chunkStart, chunkLen);

        pQueue->process();
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

    void Detection::castAndCommonref(IntFrame chunkStart, IntFrame chunkLen)
    {
        if (rescale)
        {
            for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
            {
                scaleCast(trace[t], traceRaw[t]);
            }
        }
        else
        {
            for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
            {
                noscaleCast(trace[t], traceRaw[t]);
            }
        }

        if (medianReference)
        {
            IntVolt *buffer = new IntVolt[numChannels]; // nth_element modifies container
            IntChannel mid = numChannels / 2;

            for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
            {
                commonMedian(commonRef[t], trace[t], buffer, mid);
            }

            delete[] buffer;
        }
        else if (averageReference)
        {
            for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
            {
                commonAverage(commonRef[t], trace[t]);
            }
        }
    }

    void Detection::scaleCast(IntVolt *trace, const FloatRaw *input)
    {
        for (IntChannel i = 0; i < alignedChannels * channelAlign; i++)
        {
            trace[i] = input[i] * scale[i] + offset[i];
        }
    }

    void Detection::noscaleCast(IntVolt *trace, const FloatRaw *input)
    {
        for (IntChannel i = 0; i < alignedChannels * channelAlign; i++)
        {
            trace[i] = input[i];
        }
    }

    void Detection::commonMedian(IntVolt *ref, const IntVolt *trace, IntVolt *buffer, IntChannel mid)
    {
        copy_n(trace, numChannels, buffer);
        nth_element(buffer, buffer + mid, buffer + numChannels);
        *ref = buffer[mid];
    }

    void Detection::commonAverage(IntVolt *ref, const IntVolt *trace)
    {
        IntCalc sum = accumulate(trace, trace + numChannels, (IntCalc)0,
                                 [](IntCalc sum, IntVolt data)
                                 { return sum + data; });
        *ref = sum / numChannels;
    }

    void Detection::estimateAndDetect(IntFrame chunkStart, IntFrame chunkLen)
    {
        for (IntFrame t = chunkStart; t < chunkStart + chunkLen; t++)
        {
            const IntVolt *trace = this->trace[t];
            IntVolt ref = commonRef(t, 0);
            const IntVolt *basePrev = runningBaseline[t - 1];
            const IntVolt *devPrev = runningDeviation[t - 1];
            IntVolt *baselines = runningBaseline[t];
            IntVolt *deviations = runningDeviation[t];

            for (IntChannel i = 0; i < alignedChannels * channelAlign; i++)
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
                    pQueue->addSpike(Spike(t - spikeDur, i, spikeAmp[i]));
                }

                spikeTime[i] = -1; // reset counter even if not spike

            } // second for i

        } // for t

    } // Detection::estimateAndDetect

} // namespace HSDetection
