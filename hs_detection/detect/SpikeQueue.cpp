#include <algorithm>

#include "SpikeQueue.h"
#include "Detection.h"
#include "QueueProcessor/FirstElemProcessor.h"
#include "QueueProcessor/MaxSpikeFinder.h"
#include "QueueProcessor/SpikeDecayFilterer.h"
#include "QueueProcessor/SpikeFilterer.h"
#include "SpikeProcessor/SpikeLocalizer.h"
#include "SpikeProcessor/SpikeShapeWriter.h"

using namespace std;

#define pushFirstElemProc(pSpkProc)                  \
    do                                               \
    {                                                \
        spkProcs.push_back(pSpkProc);                \
        pQueProc = new FirstElemProcessor(pSpkProc); \
        queProcs.push_back(pQueProc);                \
    } while (false)

namespace HSDetection
{
    SpikeQueue::SpikeQueue(Detection *pDet)
        : queue(), queProcs(), spkProcs(), result(&pDet->result),
          framesInQueue(pDet->noiseDuration + pDet->spikePeakDuration),
          framesToContinue(pDet->noiseDuration + 1)
    {
        SpikeProcessor *pSpkProc;
        QueueProcessor *pQueProc;

        pQueProc = new MaxSpikeFinder(&pDet->probeLayout, pDet->noiseDuration);
        queProcs.push_back(pQueProc);

        if (pDet->decayFilter)
        {
            pQueProc = new SpikeDecayFilterer(&pDet->probeLayout, pDet->noiseDuration, pDet->noiseAmpPercent);
        }
        else
        {
            pQueProc = new SpikeFilterer(&pDet->probeLayout);
        }
        queProcs.push_back(pQueProc);

        if (pDet->localize)
        {
            pSpkProc = new SpikeLocalizer(&pDet->probeLayout, &pDet->trace, &pDet->commonRef, &pDet->runningBaseline,
                                          pDet->numCoMCenters, pDet->noiseDuration, pDet->spikePeakDuration);
            pushFirstElemProc(pSpkProc);
        }

        if (pDet->saveShape)
        {
            pSpkProc = new SpikeShapeWriter(pDet->filename, &pDet->trace, pDet->cutoutStart, pDet->cutoutLen);
            pushFirstElemProc(pSpkProc);
        }
    }

    SpikeQueue::~SpikeQueue()
    {
        // should release QueueProc first because SpikeProc can be wrapped inside
        for_each(queProcs.begin(), queProcs.end(),
                 [](QueueProcessor *pQueProc)
                 { delete pQueProc; });
        for_each(spkProcs.begin(), spkProcs.end(),
                 [](SpikeProcessor *pSpkProc)
                 { delete pSpkProc; });
    }

    void SpikeQueue::process(IntFrame frameBound)
    {
        // TODO: one loop?
        while (!queue.empty() && queue.front().frame < frameBound)
        {
            IntFrame lastFrame = queue.front().frame;

            while (!queue.empty() && queue.front().frame < lastFrame + framesToContinue)
            {
                lastFrame = queue.front().frame;

                for_each(queProcs.begin(), queProcs.end(),
                         [this](QueueProcessor *pQueProc)
                         { (*pQueProc)(this); });

                result->push_back(move(*queue.begin()));
                queue.erase(queue.begin());
            }
        }
    }

    void SpikeQueue::finalize()
    {
        process(MAX_FRAME);
    }

} // namespace HSDetection
