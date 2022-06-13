# distutils: language=c++
# cython: language_level=3

from libcpp cimport bool
from libcpp.string cimport string


cdef extern from "SpkDonline.h" namespace "SpkDonline":
    cdef cppclass Detection:
        Detection() except +
        void InitDetection(int sf, int NCh, long ti, long int * Indices, int agl)
        void SetInitialParams(int * pos_mtx, int * neigh_mtx, int num_channels,
                              int spike_peak_duration, string file_name, int noise_duration,
                              float noise_amp_percent, float inner_radius,
                              int max_neighbors, int num_com_centers, bool to_localize, int thres, int cutout_start, int cutout_end,
                              int maa, int ahpthr, int maxsl, int minsl, bool decay_filtering)
        void MedianVoltage(short * vm)
        void MeanVoltage(short * vm, int tInc, int tCut)
        void Iterate(short *vm, long t0, int tInc, int tCut, int tCut2, int maxFramesProcessed)
        void FinishDetection()
