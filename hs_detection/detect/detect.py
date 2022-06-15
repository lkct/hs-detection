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
        self.num_segments = recording.get_num_segments()
        self.num_frames = [recording.get_num_samples(seg)
                           for seg in range(self.num_segments)]

        self.rescale: bool = params['rescale']
        if self.rescale:
            l, m, r = np.quantile(self.get_random_data_chunks(recording),
                                  q=[0.05, 0.5, 1 - 0.05], axis=0, keepdims=True)
            # quantile gives float64 on float32 data
            l: NDArray[np.float32] = l.astype(np.float32)
            m: NDArray[np.float32] = m.astype(np.float32)
            r: NDArray[np.float32] = r.astype(np.float32)

            self.scale: NDArray[np.float32] = params['rescale_value'] / (r - l)
            self.offset: NDArray[np.float32] = -m * self.scale
        else:
            self.scale: NDArray[np.float32] = np.array(1.0, dtype=np.float32)
            self.offset: NDArray[np.float32] = np.array(0.0, dtype=np.float32)

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

    @staticmethod
    def get_random_data_chunks(recording: Recording,
                               num_chunks_per_segment: int = 20,
                               chunk_size: int = 10000,
                               seed: int = 0
                               ) -> NDArray[np.float32]:
        # TODO: sample uniformly on samples instead of segments
        chunks: list[RealArray] = []
        for seg in range(recording.get_num_segments()):
            num_frames = recording.get_num_samples(seg)
            # TODO: new generator api
            random_starts = np.random.RandomState(seed).randint(
                0, num_frames - chunk_size, size=num_chunks_per_segment)
            for start_frame in random_starts:
                chunks.append(recording.get_traces(
                    segment_index=seg, start_frame=start_frame, end_frame=start_frame + chunk_size))
        return np.concatenate(chunks, axis=0, dtype=np.float32)

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

        traces: RealArray = self.recording.get_traces(
            segment_index=segment_index, start_frame=start_frame, end_frame=end_frame)

        if self.rescale:
            traces = traces.astype(np.float32, copy=False) * \
                self.scale + self.offset

        traces_int: NDArray[np.int16] = traces.astype(
            np.int16, copy=False).reshape(-1)

        if lpad or rpad:
            traces_int = np.pad(traces_int, (lpad, rpad),
                                mode='constant', constant_values=0)

        return traces_int

    def detect(self) -> Sequence[Mapping[str, RealArray]]:
        return [self.detect_seg(seg) for seg in range(self.num_segments)]

    def detect_seg(self, segment_index: int) -> Mapping[str, RealArray]:
        t_cut = self.cutout_start + self.maxsl
        t_cut2 = self.cutout_end + self.maxsl
        t_inc = min(self.chunk_size,
                    self.num_frames[segment_index] - t_cut - t_cut2)

        position_matrix: cython.int[:, :] = np.ascontiguousarray(
            self.positions, dtype=np.int32)
        neighbor_matrix: cython.int[:, :] = np.ascontiguousarray(
            self.neighbors, dtype=np.int32)
        out_file = self.out_file.with_stem(
            self.out_file.stem + f'-{segment_index}')
        det: cython.pointer(Detection) = new Detection(
            t_inc,
            cython.address(position_matrix[0, 0]),
            cython.address(neighbor_matrix[0, 0]),
            self.num_channels,
            self.spike_peak_duration,
            str(out_file).encode(),
            self.noise_duration,
            self.noise_amp_percent,
            self.inner_radius,
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
            self.decay_filtering)

        vm: cython.short[:] = np.zeros(
            self.num_channels * (t_inc + t_cut + t_cut2), dtype=np.int16)

        t0 = 0
        while t0 + t_inc + t_cut2 <= self.num_frames[segment_index]:
            t1 = t0 + t_inc
            if self.verbose:
                print(f'HSDetection: Analysing segment {segment_index}, '
                      f'frames from {t0:8d} to {t1:8d} '
                      f' ({100 * t0 / self.num_frames[segment_index]:.1f}%)')

            vm = self.get_traces(segment_index=segment_index,
                                 start_frame=t0 - t_cut, end_frame=t1 + t_cut2)
            if self.num_channels >= 20:
                det.MeanVoltage(cython.address(vm[0]), t_inc, t_cut)
            det.Iterate(cython.address(vm[0]), t0, t_inc, t_cut, t_cut2)

            t0 += t_inc
            if t0 < self.num_frames[segment_index] - t_cut2:
                t_inc = min(
                    t_inc, self.num_frames[segment_index] - t_cut2 - t0)

        det.FinishDetection()
        del det

        if out_file.stat().st_size == 0:
            spikes: NDArray[np.int32] = np.empty(
                (0, 5 + self.cutout_length), dtype=np.int32)
        else:
            spikes: NDArray[np.int32] = np.memmap(
                str(out_file), dtype=np.int32, mode='r').reshape(-1, 5 + self.cutout_length)

        result: dict[str, RealArray] = {'channel_ind': spikes[:, 0],
                                        'sample_ind': spikes[:, 1],
                                        'amplitude': spikes[:, 2]}
        if self.localize:
            result |= {'location': spikes[:, 3:5] / 1000}
        if self.save_shape:
            result |= {'spike_shape': spikes[:, 5:]}

        return result
