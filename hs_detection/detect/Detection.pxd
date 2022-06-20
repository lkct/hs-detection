# distutils: language=c++
# cython: language_level=3

from libcpp cimport bool
from libcpp.string cimport string

cdef extern from "Point.h" namespace "HSDetection":
    cdef cppclass Point:
        float x
        float y

cdef extern from "Spike.h" namespace "HSDetection":
    cdef cppclass Spike:
        int frame
        int channel
        int amplitude
        Point position

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
        const Spike *getResult() const
