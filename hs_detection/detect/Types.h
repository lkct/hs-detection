#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <limits>

#define MAX_FRAME std::numeric_limits<IntFrame>::max()

namespace HSDetection
{
    typedef int32_t IntFrame;
    typedef int32_t IntChannel;
    typedef int16_t IntVolt;
    typedef float FloatGeom; // correspond to np.single and cython.float

    typedef int64_t IntFxC;
    typedef int32_t IntFxV;
    typedef int32_t IntCxV;
    typedef int64_t IntFCV;
    typedef int32_t IntResult; // TODO: ??? or FxC

} // namespace HSDetection

#endif
