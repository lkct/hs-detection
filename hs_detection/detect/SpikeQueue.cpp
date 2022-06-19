#include <algorithm>
#include <limits>

#include "SpikeQueue.h"
#include "Detection.h"
#include "QueueProcessor/FirstElemProcessor.h"
#include "QueueProcessor/SpikeDecayFilterer.h"
#include "QueueProcessor/SpikeFilterer.h"
#include "SpikeProcessor/SpikeLocalizer.h"
#include "SpikeProcessor/SpikeSaver.h"
#include "SpikeProcessor/SpikeWriter.h"

using namespace std;

// TODO: batch replace with int32_t?
#define MAX_INT numeric_limits<int>::max()

#define addFirstElemProc(pSpkProc)                   \
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
            pSpkProc = new SpikeLocalizer(&pDet->probeLayout, pDet->noise_duration,
                                          pDet->spike_peak_duration, pDet->num_com_centers,
                                          &pDet->trace, &pDet->AGlobal, &pDet->QBs);
            addFirstElemProc(pSpkProc);
        }

        pSpkProc = new SpikerSaver(&pDet->result);
        addFirstElemProc(pSpkProc);

        if (pDet->saveShape)
        {
            pSpkProc = new SpikeWriter(pDet->filename, &pDet->trace, pDet->cutout_start, pDet->cutout_end);
            addFirstElemProc(pSpkProc);
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
            int last_frame = queue.front().frame;

            while (!queue.empty() && queue.front().frame < last_frame + framesToContinue)
            {
                last_frame = queue.front().frame;

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
