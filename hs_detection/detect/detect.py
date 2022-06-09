# distutils: language=c++
# cython: language_level=3

import warnings
from pathlib import Path
from typing import Any, Mapping, Sequence, Union

import cython
import numpy as np
from numpy.typing import NDArray

from ..recording import RealArray, Recording


class HSDetection(object):
    """TODO:
    """

    def __init__(self, recording: Recording, params: Mapping[str, Any]) -> None:
        self.recording = recording
        self.num_channels = recording.get_num_channels()
        self.fps = recording.get_sampling_frequency()
        self.num_segments = recording.get_num_segments()  # TODO: seg proc
        self.num_frames = [recording.get_num_samples(seg)
                           for seg in range(self.num_segments)]

        positions: NDArray[np.float64] = np.array([
            recording.get_channel_property(ch, 'location') for ch in recording.get_channel_ids()])
        if positions.shape[1] > 2:
            warnings.warn(f'Channel locations have {positions.shape[1]} dimensions, '
                          'using the last two.')
            positions = positions[:, -2:]
        self.positions = positions.astype(np.int32)

        # using Euclidean distance, also possible to use Manhattan (ord=1)
        distances: NDArray[np.float64] = np.linalg.norm(
            positions[:, None] - positions[None, :], axis=2, ord=2)
        neighbors: list[NDArray[np.int64]] = [
            np.nonzero(dist < params['neighbor_radius'])[0] for dist in distances]
        self.max_neighbors = max([n.shape[0] for n in neighbors])
        self.neighbors: NDArray[np.int32] = np.full(
            (self.num_channels, self.max_neighbors), -1, dtype=np.int32)
        for i, n in enumerate(neighbors):
            self.neighbors[i, :n.shape[0]] = n

        self.spike_peak_duration = int(
            params['event_length'] * self.fps / 1000)
        self.noise_duration = int(params['peak_jitter'] * self.fps / 1000)
        self.noise_amp_percent: float = params['noise_amp_percent']
        self.inner_radius: float = params['inner_radius']

        self.cutout_start = int(
            params['left_cutout_time'] * self.fps / 1000 + 0.5)
        self.cutout_end = int(
            params['right_cutout_time'] * self.fps / 1000 + 0.5)
        self.minsl = int(params['amp_evaluation_time'] * self.fps / 1000 + 0.5)
        self.maxsl = int(params['spk_evaluation_time'] * self.fps / 1000 + 0.5)
        self.cutout_length = self.cutout_start + self.cutout_end + 1

        self.chunk_size: int = params['chunk_size']
        self.threshold: int = params['threshold']
        self.num_com_centers: int = params['num_com_centers']
        self.maa: int = params['maa']
        self.ahpthr: int = params['ahpthr']
        self.decay_filtering: bool = params['decay_filtering']

        self.localize: bool = params['localize']
        self.save_shape: bool = params['save_shape']
        self.verbose: bool = params['verbose']

        out_file: Union[str, Path] = params['out_file']
        if isinstance(out_file, str):
            out_file = Path(out_file)
        out_file.parent.mkdir(parents=True, exist_ok=True)
        if out_file.suffix != '.bin':
            out_file = out_file.with_suffix('.bin')
        self.out_file = out_file

    def get_traces(self, segment_index: int, start_frame: int, end_frame: int) -> NDArray[np.int16]:
        if start_frame < 0:
            lpad = -start_frame * self.num_channels
            start_frame = 0
        else:
            lpad = 0
        if end_frame > self.num_frames[segment_index]:
            rpad = (
                end_frame - self.num_frames[segment_index]) * self.num_channels
            end_frame = self.num_frames[segment_index]
        else:
            rpad = 0

        traces: NDArray[np.int16] = self.recording.get_traces(
            segment_index=segment_index, start_frame=start_frame, end_frame=end_frame
        ).astype(np.int16, copy=False).reshape(-1)

        if lpad or rpad:
            traces = np.pad(traces, (lpad, rpad),
                            mode='constant', constant_values=0)

        return traces

    def detect(self) -> Sequence[Mapping[str, RealArray]]:
        det: cython.pointer(Detection) = new Detection()

        t_cut = self.cutout_start + self.maxsl
        t_cut2 = self.cutout_end + self.maxsl
        t_inc = min(self.chunk_size, self.num_frames[0] - t_cut - t_cut2)
        t_inc_orig = t_inc

        channel_indices: cython.long[:] = np.arange(
            self.num_channels, dtype=np.int64)
        det.InitDetection(self.num_frames[0], int(self.fps), self.num_channels, t_inc,
                          cython.address(channel_indices[0]), 0)

        position_matrix: cython.int[:, :] = np.ascontiguousarray(
            self.positions, dtype=np.int32)
        neighbor_matrix: cython.int[:, :] = np.ascontiguousarray(
            self.neighbors, dtype=np.int32)
        masked_channels: cython.int[:] = np.ones(
            self.num_channels, dtype=np.int32)
        det.SetInitialParams(cython.address(position_matrix[0, 0]),
                             cython.address(neighbor_matrix[0, 0]),
                             self.num_channels,
                             self.spike_peak_duration,
                             str(self.out_file.with_suffix('')).encode(),
                             self.noise_duration,
                             self.noise_amp_percent,
                             self.inner_radius,
                             cython.address(masked_channels[0]),
                             self.max_neighbors,
                             self.num_com_centers,
                             self.localize,
                             self.threshold,
                             self.cutout_start,
                             self.cutout_end,
                             self.maa,
                             self.ahpthr,
                             self.maxsl,
                             self.minsl,
                             self.decay_filtering,
                             False)

        vm: cython.short[:] = np.zeros(
            self.num_channels * (t_inc + t_cut + t_cut2), dtype=np.int16)

        t0 = 0
        while t0 + t_inc + t_cut2 <= self.num_frames[0]:
            t1 = t0 + t_inc
            if self.verbose:
                print(f'HSDetection: Analysing frames from {t0:8d} to {t1:8d} '
                      f' ({100 * t0 / self.num_frames[0]:.1f}%)')

            vm = self.get_traces(
                segment_index=0, start_frame=t0 - t_cut, end_frame=t1 + t_cut2)
            if self.num_channels >= 20:
                det.MeanVoltage(cython.address(vm[0]), t_inc, t_cut)
            det.Iterate(cython.address(vm[0]), t0,
                        t_inc, t_cut, t_cut2, t_inc_orig)

            t0 += t_inc
            if t0 < self.num_frames[0] - t_cut2:
                t_inc = min(t_inc, self.num_frames[0] - t_cut2 - t0)

        det.FinishDetection()
        del det

        if self.out_file.stat().st_size == 0:
            spikes: NDArray[np.int32] = np.empty(
                (0, 5 + self.cutout_length), dtype=np.int32)
        else:
            spikes: NDArray[np.int32] = np.memmap(
                str(self.out_file), dtype=np.int32, mode='r').reshape(-1, 5 + self.cutout_length)

        result: dict[str, RealArray] = {'channel_ind': spikes[:, 0],
                                        'sample_ind': spikes[:, 1],
                                        'amplitude': spikes[:, 2]}
        if self.localize:
            result |= {'location': spikes[:, 3:5] / 1000}
        if self.save_shape:
            result |= {'spike_shape': spikes[:, 5:]}

        return [result]
