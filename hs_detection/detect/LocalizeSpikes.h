#ifndef LOCALIZESPIKES_H
#define LOCALIZESPIKES_H

#include "Spike.h"
#include <tuple>
#include <vector>
#include "Point.h"

namespace LocalizeSpikes
{

    Point centerOfMass(std::vector<std::tuple<int, int>> centered_amps);
    Point localizeSpike(Spike spike_to_be_localized);
    Point reweightedCenterOfMass(std::vector<std::tuple<Point, int>> com_positions_amps);

};

#endif
