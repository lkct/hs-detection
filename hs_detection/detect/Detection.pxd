# distutils: language=c++
# cython: language_level=3

from libcpp cimport bool
from libcpp.string cimport string


cdef extern from "Detection.h" namespace "HSDetection":
    cdef cppclass Detection:
        Detection(int tInc, int *positionMatrix, int *neighborMatrix,
                  int nChannels, int spikePeakDuration, string filename,
                  int noiseDuration, float noiseAmpPercent, float innerRadius,
                  int maxNeighbors, int numComCenters, bool localize,
                  int threshold, int cutoutStart, int cutoutEnd, int minAvgAmp,
                  int ahpthr, int maxSl, int minSl, bool decayFiltering) except +
        void MedianVoltage(short * vm)
        void MeanVoltage(short * vm, int tInc, int tCut)
        void Iterate(short *vm, long t0, int tInc, int tCut, int tCut2)
        void FinishDetection()
