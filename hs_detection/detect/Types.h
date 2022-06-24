#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <limits>

#define MAX_FRAME std::numeric_limits<IntFrame>::max()

namespace HSDetection
{
    // used in interface
    typedef int32_t IntFrame;   // number of frames
    typedef int32_t IntChannel; // number of channels
    typedef int16_t IntVolt;    // quantized voltage
    typedef float FloatGeom;    // spatial dimension, correspond to np.single and cython.float
    typedef float FloatRatio;   // 0.0~1.0
    typedef int32_t IntResult;  // expected number of spikes

    // used only internally
    typedef intmax_t IntMax;

} // namespace HSDetection

#endif
