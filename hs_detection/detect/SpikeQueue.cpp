#include <algorithm>
#include <limits>

#include "SpikeQueue.h"
#include "Detection.h"
#include "QueueProcessor/FirstElemProcessor.h"
#include "QueueProcessor/SpikeDecayFilterer.h"
#include "QueueProcessor/SpikeFilterer.h"
#include "SpikeProcessor/SpikeLocalizer.h"
#include "SpikeProcessor/SpikeSaver.h"
#include "SpikeProcessor/SpikeShapeSaver.h"

using namespace std;

// TODO: batch replace with int32_t?
#define MAX_INT numeric_limits<int>::max()

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
        : queue(), queProcs(), spkProcs(),
          framesInQueue(pDet->noise_duration + pDet->spike_peak_duration),
          framesToContinue(pDet->noise_duration + 1)
    {
        SpikeProcessor *pSpkProc;
        QueueProcessor *pQueProc;

        if (pDet->decay_filtering)
        {
            pQueProc = new SpikeDecayFilterer(&pDet->probeLayout, pDet->noise_duration, pDet->noise_amp_percent);
        }
        else
        {
            pQueProc = new SpikeFilterer(&pDet->probeLayout, pDet->noise_duration);
        }
        queProcs.push_back(pQueProc);

        if (pDet->to_localize)
        {
            pSpkProc = new SpikeLocalizer(&pDet->probeLayout, &pDet->trace, &pDet->AGlobal, &pDet->QBs,
                                          pDet->num_com_centers, pDet->noise_duration, pDet->spike_peak_duration);
            pushFirstElemProc(pSpkProc);
        }

        pSpkProc = new SpikerSaver(&pDet->result);
        pushFirstElemProc(pSpkProc);

        if (pDet->saveShape)
        {
            pSpkProc = new SpikeShapeSaver(pDet->filename, &pDet->trace, pDet->cutout_start, pDet->cutout_size);
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

    void SpikeQueue::process(int frameBound)
    {
        // TODO: one loop?
        while (!queue.empty() && queue.front().frame < frameBound)
        {
            int lastFrame = queue.front().frame;

            while (!queue.empty() && queue.front().frame < lastFrame + framesToContinue)
            {
                lastFrame = queue.front().frame;

                for_each(queProcs.begin(), queProcs.end(),
                         [this](QueueProcessor *pQueProc)
                         { (*pQueProc)(this); });

                queue.erase(queue.begin());
            }
        }
    }

    void SpikeQueue::finalize()
    {
        process(MAX_INT);
    }

} // namespace HSDetection
