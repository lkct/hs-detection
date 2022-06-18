#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <utility>
#include "Spike.h"

using HSDetection::Spike;

namespace Utils
{

    void storeWaveformCutout(std::vector<int>&written_cutout, int frame, int channel);
    Spike storeCOMWaveformsCounts(Spike curr_spike);

};

#endif
