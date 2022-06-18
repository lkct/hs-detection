#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <utility>
#include "Spike.h"

using HSDetection::Spike;

namespace Utils
{

    void storeCOMWaveformsCounts(std::vector<std::vector<std::pair<int, int>>> &chAmps, int frame, int channel);

};

#endif
