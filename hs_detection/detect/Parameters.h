#ifndef PARAMETERS_H
#define PARAMETERS_H

// Contains all parameters and libraries for running the SpikeHandler Methods

#include <deque>
#include "Spike.h"

namespace Parameters
{

    extern int **neighbor_matrix;       /*Indexed by the channel number starting at 0 and going up to num_channels - 1. Each
                                         index contains pointer to another array which contains channel number of all its neighbors.
                                         User creates this before calling SpikeHandler. Each column has size equal to max neighbors where
                                         any channels that have less neighbors fills the rest with -1 (important). */
    extern int **inner_neighbor_matrix; /*Indexed by the channel number starting at 0 and going up to num_channels - 1. Each
                                  index contains pointer to another array which contains channel number of all its inner neighbors.
                                  Created by SpikeHandler; */
    extern int **outer_neighbor_matrix; /*Indexed by the channel number starting at 0 and going up to num_channels - 1. Each
                                        index contains pointer to another array which contains channel number of all its outer neighbors.
                                        Created by SpikeHandler; */

    extern float **channel_positions;           /*Indexed by the channel number starting at 0 and going up to num_channels - 1. Each
                                             index contains pointer to another array which contains X and Y position of the channel. User creates
                                             this before calling SpikeHandler. */
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
    extern int event_number;

};

#endif
