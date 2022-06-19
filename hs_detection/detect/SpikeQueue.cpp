#include <algorithm>

#include "SpikeQueue.h"
#include "Detection.h"
#include "QueueProcessor/FirstElemProcessor.h"
#include "QueueProcessor/SpikeDecayFilterer.h"
#include "QueueProcessor/SpikeFilterer.h"
#include "SpikeProcessor/SpikeLocalizer.h"
#include "SpikeProcessor/SpikeSaver.h"
#include "SpikeProcessor/SpikeWriter.h"

using namespace std;

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
            queProcs.push_back(pQueProc);
        }
        else
        {
            pQueProc = new SpikeFilterer(&pDet->probeLayout, pDet->noise_duration);
            queProcs.push_back(pQueProc);
        }

        if (pDet->to_localize)
        {
            pSpkProc = new SpikeLocalizer(&pDet->probeLayout, pDet->noise_duration,
                                          pDet->spike_peak_duration, pDet->num_com_centers,
                                          &pDet->trace, &pDet->AGlobal, &pDet->QBs);
            spkProcs.push_back(pSpkProc);
            pQueProc = new FirstElemProcessor(pSpkProc);
            queProcs.push_back(pQueProc);
        }

        pSpkProc = new SpikerSaver(&pDet->result);
        spkProcs.push_back(pSpkProc);
        pQueProc = new FirstElemProcessor(pSpkProc);
        queProcs.push_back(pQueProc);

        if (pDet->saveShape)
        {
            pSpkProc = new SpikeWriter(pDet->filename, &pDet->trace, pDet->cutout_start, pDet->cutout_end);
            spkProcs.push_back(pSpkProc);
            pQueProc = new FirstElemProcessor(pSpkProc);
            queProcs.push_back(pQueProc);
        }
    }

    SpikeQueue::~SpikeQueue()
    {
        for_each(queProcs.begin(), queProcs.end(),
                 [](QueueProcessor *pQueProc)
                 { delete pQueProc; });
        for_each(spkProcs.begin(), spkProcs.end(),
                 [](SpikeProcessor *pSpkProc)
                 { delete pSpkProc; });
    }

    void SpikeQueue::add(Spike &spike)
    {
        process(spike.frame - framesInQueue);

        queue.push_back(spike);
    }

    void SpikeQueue::close()
    {
        process();
    }

    void SpikeQueue::process(int frameBound)
    {
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

} // namespace HSDetection
