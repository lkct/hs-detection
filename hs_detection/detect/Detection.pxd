# distutils: language=c++
# cython: language_level=3

from libcpp cimport bool
from libcpp.string cimport string


cdef extern from "Detection.h" namespace "HSDetection":
    cdef cppclass Detection:
        Detection(int chunkSize, int *positionMatrix,
                  int nChannels, int spikePeakDuration, string filename,
                  int noiseDuration, float noiseAmpPercent, float neighborRadius, float innerRadius,
                  int numComCenters, bool localize,
                  int threshold, int cutoutStart, int cutoutEnd, int minAvgAmp,
                  int ahpthr, int maxSl, int minSl, bool decayFiltering, bool saveShape,
                  int tCut) except +
        void step(short *traceBuffer, int chunkStart, int chunkLen)
        int finish()
        char *getResult()
