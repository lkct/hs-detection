# distutils: language=c++
# cython: language_level=3

import cython
import numpy as np

from ..hs2 import HSDetection


def detectData(probe: HSDetection,
               file_name: str,
               to_localize: bool,
               fps: int,
               thres: int,
               cutout_start: int,
               cutout_end: int,
               maa: int,
               maxsl: int,
               minsl: int,
               ahpthr: int,
               num_com_centers: int,
               decay_filtering: bool,
               verbose: bool,
               num_frames: int,
               t_inc: int
               ) -> None:
    det: cython.pointer(Detection) = new Detection()

    t_cut = cutout_start + maxsl
    t_cut2 = cutout_end + maxsl
    t_inc = min(t_inc, num_frames - t_cut - t_cut2)
    t_inc_orig = t_inc

    channel_indices: cython.long[:] = np.arange(
        probe.num_channels, dtype=np.int64)
    det.InitDetection(num_frames, fps, probe.num_channels, t_inc,
                      cython.address(channel_indices[0]), 0)

    position_matrix: cython.int[:, :] = np.ascontiguousarray(
        probe.positions, dtype=np.int32)
    neighbor_matrix: cython.int[:, :] = np.ascontiguousarray(
        probe.neighbors, dtype=np.int32)
    masked_channels: cython.int[:] = np.ones(
        probe.num_channels, dtype=np.int32)
    det.SetInitialParams(cython.address(position_matrix[0, 0]),
                         cython.address(neighbor_matrix[0, 0]),
                         probe.num_channels,
                         probe.spike_peak_duration,
                         file_name.encode(),
                         probe.noise_duration,
                         probe.noise_amp_percent,
                         probe.inner_radius,
                         cython.address(masked_channels[0]),
                         probe.max_neighbors,
                         num_com_centers,
                         to_localize,
                         thres,
                         cutout_start,
                         cutout_end,
                         maa,
                         ahpthr,
                         maxsl,
                         minsl,
                         decay_filtering,
                         verbose)

    vm: cython.short[:] = np.zeros(
        probe.num_channels * (t_inc + t_cut + t_cut2), dtype=np.int16)

    t0 = 0
    while t0 + t_inc + t_cut2 <= num_frames:
        t1 = t0 + t_inc
        if verbose:
            print(f'HSDetection: Analysing frames from {t0:8d} to {t1:8d} '
                  f' ({100 * t0 / num_frames:.1f}%)')

        vm = probe.get_traces(
            segment_index=0, start_frame=t0 - t_cut, end_frame=t1 + t_cut2)
        if probe.num_channels >= 20:
            det.MeanVoltage(cython.address(vm[0]), t_inc, t_cut)
        det.Iterate(cython.address(vm[0]), t0,
                    t_inc, t_cut, t_cut2, t_inc_orig)

        t0 += t_inc
        if t0 < num_frames - t_cut2:
            t_inc = min(t_inc, num_frames - t_cut2 - t0)

    det.FinishDetection()
    del det
