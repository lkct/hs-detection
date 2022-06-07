# distutils: language=c++
# cython: language_level=3

import warnings
from pathlib import Path
from typing import Any, Mapping, Sequence, Union

import cython
import numpy as np
from numpy.typing import NDArray

from ..recording import Recording
from .utils import get_scaling_param


class HSDetection(object):
    """TODO:
    """

    def __init__(self, recording: Recording, params: Mapping[str, Any]) -> None:
        self.rescale: bool = params['rescale']
        if self.rescale:
            self.scale, self.offset = get_scaling_param(
                recording, scale=params['rescale_value'])
        else:
            self.scale: NDArray[np.float32] = np.array(0, dtype=np.float32)
            self.offset: NDArray[np.float32] = np.array(0, dtype=np.float32)

        self.recording = recording
        self.num_frames = recording.get_num_samples(0)  # TODO: segment proc
        self.fps = recording.get_sampling_frequency()
        self.num_channels = recording.get_num_channels()

        positions: NDArray[np.float64] = np.array([
            recording.get_channel_property(ch, 'location') for ch in recording.get_channel_ids()])
        if positions.shape[1] > 2:
            warnings.warn(f'Channel locations have {positions.shape[1]} dimensions, '
                          'using the last two.')
            positions = positions[:, -2:]
        self.positions = positions

        # using Euclidean distance, also possible to use Manhattan
        distances: NDArray[np.float64] = np.linalg.norm(
            positions[:, None] - positions[None, :], axis=2, ord=2)

        neighbors: list[NDArray[np.int64]] = [
            np.nonzero(dist_from_ch < params['neighbor_radius'])[0] for dist_from_ch in distances]
        self.max_neighbors = max([n.shape[0] for n in neighbors])
        self.neighbors: NDArray[np.int64] = np.array([
            np.pad(n, (0, self.max_neighbors - n.shape[0]),
                   mode='constant', constant_values=-1)
            for n in neighbors])

        self.spike_peak_duration = int(
            params['event_length'] * self.fps / 1000)
        self.noise_duration = int(params['peak_jitter'] * self.fps / 1000)
        self.noise_amp_percent: float = params['noise_amp_percent']
        self.inner_radius: float = params['inner_radius']

        self.masked_channels: NDArray[np.int64] = np.ones(
            self.num_channels, dtype=np.int64)

        self.cutout_start = int(
            params['left_cutout_time'] * self.fps / 1000 + 0.5)
        self.cutout_end = int(
            params['right_cutout_time'] * self.fps / 1000 + 0.5)
        self.minsl = int(params['amp_evaluation_time'] * self.fps / 1000 + 0.5)
        self.maxsl = int(params['spk_evaluation_time'] * self.fps / 1000 + 0.5)

        self.localize: bool = params['localize']
        self.cutout_length = self.cutout_start + self.cutout_end + 1
        self.threshold: int = params['threshold']
        self.maa: int = params['maa']
        self.ahpthr: int = params['ahpthr']
        self.decay_filtering: bool = params['decay_filtering']
        self.num_com_centers: int = params['num_com_centers']

        out_file: Union[str, Path] = params['out_file']
        if isinstance(out_file, str):
            out_file = Path(out_file)
        out_file.parent.mkdir(parents=True, exist_ok=True)

        if out_file.suffix != '.bin':
            out_file = out_file.with_suffix('.bin')
        self.out_file = out_file

        self.verbose: bool = params['verbose']
        self.save_shape: bool = params['save_shape']  # TODO: add functionality
        self.chunk_size: int = params['chunk_size']

    def get_traces(self, start_frame: int, end_frame: int) -> NDArray[np.short]:
        if start_frame < 0:
            lpad = -start_frame * self.num_channels
            start_frame = 0
        else:
            lpad = 0
        if end_frame > self.num_frames:
            rpad = (end_frame - self.num_frames) * self.num_channels
            end_frame = self.num_frames
        else:
            rpad = 0

        traces: NDArray[np.number] = self.recording.get_traces(
            start_frame=start_frame, end_frame=end_frame)

        if self.rescale:
            traces = traces.astype(np.float32, copy=False) * \
                self.scale + self.offset

        # astype cannot convert type annotation
        traces_int: NDArray[np.short] = traces.astype(
            np.short, copy=False).reshape(-1)  # type: ignore

        if lpad + rpad > 0:
            traces_int = np.pad(traces_int, (lpad, rpad),
                                mode='constant', constant_values=0)

        return traces_int

    def detect(self) -> Sequence[Mapping[str, Union[NDArray[np.integer], NDArray[np.floating]]]]:

        det: cython.pointer(Detection) = new Detection()

        # set tCut, tCut2 and tInc
        t_cut_l = self.cutout_start + self.maxsl
        t_cut_r = self.cutout_end + self.maxsl

        # cap at specified number of frames
        t_inc = min(self.num_frames - t_cut_l - t_cut_r, self.chunk_size)
        # ! To be consistent, X and Y have to be swappped
        ch_indices: cython.long[:] = np.arange(
            self.num_channels, dtype=np.int_)
        vm: cython.short[:] = np.zeros(
            self.num_channels * (t_inc + t_cut_l + t_cut_r), dtype=np.short)

        # initialise detection algorithm
        # ensure sampling rate is integer, assumed to be in Hertz
        det.InitDetection(self.num_frames, int(self.fps), self.num_channels, t_inc,
                          cython.address(ch_indices[0]), 0)

        position_matrix: cython.int[:, :] = np.ascontiguousarray(
            self.positions, dtype=np.intc)
        neighbor_matrix: cython.int[:, :] = np.ascontiguousarray(
            self.neighbors, dtype=np.intc)
        masked_channels: cython.int[:] = np.ascontiguousarray(
            self.masked_channels, dtype=np.intc)

        det.SetInitialParams(cython.address(position_matrix[0, 0]), cython.address(neighbor_matrix[0, 0]), self.num_channels,
                             self.spike_peak_duration, str(self.out_file.with_suffix('')).encode(
        ), self.noise_duration,
            self.noise_amp_percent, self.inner_radius, cython.address(
            masked_channels[0]),
            self.max_neighbors, self.num_com_centers, self.localize,
            self.threshold, self.cutout_start, self.cutout_end, self.maa, self.ahpthr, self.maxsl,
            self.minsl, self.decay_filtering, False)

        t0 = 0
        max_frames_processed = t_inc
        while t0 + t_inc + t_cut_r <= self.num_frames:
            t1 = t0 + t_inc
            if self.verbose:
                print(f'# Analysing frames from {t0 - t_cut_l} to {t1 + t_cut_r} '
                      f' ({100 * t0 / self.num_frames:.1f}%)')

            vm = self.get_traces(t0 - t_cut_l, t1 + t_cut_r)

            # detect spikes
            if self.num_channels >= 20:
                det.MeanVoltage(cython.address(vm[0]), t_inc, t_cut_l)
            det.Iterate(cython.address(vm[0]), t0,
                        t_inc, t_cut_l, t_cut_r, max_frames_processed)

            t0 += t_inc
            if t0 < self.num_frames - t_cut_r:
                t_inc = min(t_inc, self.num_frames - t_cut_r - t0)

        det.FinishDetection()
        del det

        if self.out_file.stat().st_size == 0:
            spikes = np.empty((0, 5 + self.cutout_length), dtype=np.intc)
        else:
            spikes = np.memmap(str(self.out_file), dtype=np.intc, mode='r'
                               ).reshape(-1, 5 + self.cutout_length)

        return [{'channel_ind': spikes[:, 0],
                 'sample_ind': spikes[:, 1],
                 'amplitude': spikes[:, 2],
                 'location': spikes[:, 3:5] / 1000,
                 'spike_shape': spikes[:, 5:]}]
