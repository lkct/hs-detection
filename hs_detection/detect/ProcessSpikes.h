#ifndef PROCESSSPIKES_H
#define PROCESSSPIKES_H

#include <fstream>

namespace ProcessSpikes
{

    void filterSpikes(std::ofstream &spikes_filtered_file);
    void filterLocalizeSpikes(std::ofstream &spikes_filtered_file);
};

#endif
