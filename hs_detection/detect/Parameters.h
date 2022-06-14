#ifndef PARAMETERS_H
#define PARAMETERS_H

// Contains all parameters and libraries for running the SpikeHandler Methods

#include <deque>
#include "Spike.h"

namespace Parameters
{

    extern int aGlobal;                         // Global noise
    extern int **baselines;                     // Contains spike_delay number of frames of median baseline values. Updated by user at every frame.
    extern int index_baselines;                 /*The index given to start accessing the baseline array since baseline array is size 5 and location of
                                                  oldest baseline is constantly changing*/

};

#endif
