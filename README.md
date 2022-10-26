# hs-detection

This is the package implementing the spike detection algorithm in Herding Spikes, adapted from the codebase at [HS2](https://github.com/mhhennig/HS2). The algorithm is described in the following paper:

J.-O. Muthmann, H. Amin, E. Sernagor, A. Maccione, D. Panas, L. Berdondini, U. S. Bhalla, M. H. Hennig. "Spike Detection for Large Neural Populations Using High Density Multielectrode Arrays". In: *Frontiers in Neuroinformatics* 9 (2015). doi: [10.3389/fninf.2015.00028](https://doi.org/10.3389/fninf.2015.00028).

This package is to be used as a backend for Spikeinterface's [sortingcomponents](https://github.com/SpikeInterface/spikeinterface/tree/master/spikeinterface/sortingcomponents). The implementation is further optimized based on [HS2](https://github.com/mhhennig/HS2) to handle large-scale recordings and can achieve real-time detection and localization on 6144-channel, 32 kHz data on a Haswell server CPU (single core). Parallelization on multiple cores can deliver an additional 2x speedup.

The software was first developed as part of the MSc project "Re-implementation and Optimization of a Scalable Spike Detection Algorithm for Large-Scale Extracellular Recordings" by Rickey K. Liang (supervisor [Matthias H. Hennig](https://github.com/mhhennig)), and the code is released under the GPL-3.0 license, inheriting [HS2](https://github.com/mhhennig/HS2).

## Installation

This package is to be invoked from [SpikeInterface](https://github.com/SpikeInterface/spikeinterface); please refer to the installation guide there to set up the environment. Python 3.9+ is suggested for development and testing, but Python 3.8 is also supported for users invoking from SpikeInterface.

The parallelization is controlled by env var `OMP_NUM_THREADS`. Set to 1 for single core running. 2x speedup achieved by 4~6 (depending on platform).

Please note that a C++ compiler (requires C++17 compatibility) should be properly configured to build the C++ extension for Python.

#### Via `pip`

```shell
pip install hs-detection
```

#### From source

```shell
git clone https://github.com/lkct/hs-detection.git
cd hs-detection
# pip install .  # for install
pip install -e .  # for develop
cd ..
```

## Versions

#### 0.3.1

The first version officially released to the community--the version for the MSc Dissertation with updated meta-info and readme.

## Known issues

The following are performance issues not addressed in the current implementation, but could lead to significant differences in corresponding cases.

- The common median reference (CMR) runs too slow compared to common average reference (CAR) which can reach real-time.
- Shortcuts are not set for code paths other than scaling&CAR ([ref](./hs_detection/detect/Detection.cpp#L103-L107)). A +20% speed in C++ code was gained for the scaling&CAR shortcut.
- `int16` data without the need to rescale could be expected as input from SI, but extra type-casts to and from `float32` are now enforced.

## Contact

The code was developed when the author was an MSc student at the School of Informatics, University of Edinburgh. Please reach out to [Dr MH Hennig](https://homepages.inf.ed.ac.uk/mhennig/contact/) who was the project supervisor.
