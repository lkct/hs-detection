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
        Detection(int numChannels,
                  int chunkSize, 
                  int chunkLeftMargin,
                  int spikeLen,
                  int peakLen, 
                  int threshold,
                  int minAvgAmp,
                  int maxAHPAmp,
                  int *channelPositions, 
                  float neighborRadius,
                  float innerRadius,
                  int noiseDuration,
                  int spikePeakDuration,
                  bool decayFiltering,
                  float noiseAmpPercent,
                  bool localize, 
                  int numCoMCenters,
                  bool saveShape, 
                  string filename,
                  int cutoutStart,
                  int cutoutLen) except +
        void step(short *traceBuffer, int chunkStart, int chunkLen) except +
        int finish() except +
        const Spike *getResult() except +
