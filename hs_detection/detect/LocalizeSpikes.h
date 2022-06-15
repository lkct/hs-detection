#ifndef LOCALIZESPIKES_H
#define LOCALIZESPIKES_H

#include "Spike.h"
#include <utility>
#include <vector>
#include "Point.h"

namespace LocalizeSpikes
{

    Point localizeSpike(Spike spike_to_be_localized);

};

#endif
