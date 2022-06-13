#ifndef LOCALIZESPIKES_H
#define LOCALIZESPIKES_H

#include "Spike.h"
#include <tuple>
#include <deque>

namespace LocalizeSpikes
{

    std::tuple<float, float> centerOfMass(std::deque<std::tuple<int, int>> centered_amps);
    std::tuple<float, float> localizeSpike(Spike spike_to_be_localized);
    std::tuple<float, float> reweightedCenterOfMass(std::deque<std::tuple<std::tuple<float, float>, int>> com_positions_amps);

};

#endif
