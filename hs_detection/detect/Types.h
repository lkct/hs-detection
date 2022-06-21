#ifndef TYPES_H
#define TYPES_H

// #include <cstdint>
#include <limits>

// TODO: replace with int32_t?
#define MAX_FRAME std::numeric_limits<IntFrame>::max()

namespace HSDetection
{
    typedef int IntFrame;
    typedef int IntChannel;
    typedef short IntVolt;
    typedef float FloatGeom; // TODO: float32? maybe no need

    typedef long IntFxC;
    typedef int IntFxV;
    typedef int IntCxV;
    typedef int IntFCV;
    typedef int IntResult; // TODO: ??? or FxC

} // namespace HSDetection

#endif
