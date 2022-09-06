## tests and utils

This is intended for development and includes unit tests and profiling utils. The code relies on a proper version of SpikeInterface with hs-detection integrated.

The test on correctness is performed using a small dataset, the same as the one used in SI tests. The identical results compared to HS2 are not enforced due to behaviour differences, but could be helpful in development.

The performance profiling uses different profiler packages and produces different kinds of reports. The `PROFILE` flag in [setup.py](../setup.py#L33) should be set properly for Cython profiling: 0 to turn off, 1 to profile function calls, and 2 to profile line traces.

The script [useful.sh](./useful.sh) contains some command lines useful to development, and are used to inspect and optimize.

The code in [cpp_mode](./cpp_mode) builds to a pure C++ program running the detection algorithm. Without the Python overhead, it's easier to profile performance bottlenecks in detail. In the Dissertation, the Intel Vtune Profiler is employed to perform event-based profiling.
