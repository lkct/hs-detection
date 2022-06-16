#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <utility>
#include "Spike.h"

using HSDetection::Spike;

namespace Utils
{

    Spike storeWaveformCutout(Spike curr_spike);
    Spike storeCOMWaveformsCounts(Spike curr_spike);

};

#endif
