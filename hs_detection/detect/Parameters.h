#ifndef PARAMETERS_H
#define PARAMETERS_H

// Contains all parameters and libraries for running the SpikeHandler Methods

#include <deque>
#include "Spike.h"

namespace Parameters
{

    extern int aGlobal;                         // Global noise
    extern int **baselines;                     // Contains spike_delay number of frames of median baseline values. Updated by user at every frame.
    extern std::deque<Spike> spikes_to_be_processed; // Contains all spikes to be proccessed when spike_peak_duration number of frames is stored.
    extern short *raw_data;                     // raw data passed in for current iteration
    extern int index_data;                      // The index given to start accessing the raw data. To account for extra data tacked on for cutout purposes.
    extern int index_baselines;                 /*The index given to start accessing the baseline array since baseline array is size 5 and location of
                                                  oldest baseline is constantly changing*/
    extern int max_frames_processed;            // The number of frames passed into loadRawData EXCLUDING the buffer frames.
    extern int before_chunk;                    // The number of buffer frames before chunk
    extern int after_chunk;                     // The number of buffer frames after chunk
    extern int iterations;                      // Number of current iterations of raw data passed in. User starts this at 0 and increments it for each chunk of data;
    extern int end_raw_data;                    // index of the end of the raw data

};

#endif
