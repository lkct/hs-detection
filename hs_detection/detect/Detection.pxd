# distutils: language=c++
# cython: language_level=3

from libcpp cimport bool
from libcpp.string cimport string


cdef extern from "Detection.h" namespace "HSDetection":
    cdef cppclass Detection:
        Detection(int chunkSize, int *positionMatrix, int *neighborMatrix,
                  int nChannels, int spikePeakDuration, string filename,
                  int noiseDuration, float noiseAmpPercent, float innerRadius,
                  int maxNeighbors, int numComCenters, bool localize,
                  int threshold, int cutoutStart, int cutoutEnd, int minAvgAmp,
                  int ahpthr, int maxSl, int minSl, bool decayFiltering,
                  int tCut) except +
        void Iterate(short *traceBuffer, long t0, int tInc)
        void FinishDetection()
