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

    RADIUS_EPS = 1e-3  # 1nm

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
                                  q=[0.05, 0.5, 1 - 0.05], axis=0)
            # quantile gives float64 on float32 data
            l: NDArray[np.single] = l.astype(np.single)
            m: NDArray[np.single] = m.astype(np.single)
            r: NDArray[np.single] = r.astype(np.single)

            self.scale: NDArray[np.single] = np.ascontiguousarray(
                params['rescale_value'] / (r - l), dtype=np.single)
            self.offset: NDArray[np.single] = np.ascontiguousarray(
                -m * self.scale, dtype=np.single)
        else:
            self.scale: NDArray[np.single] = np.ones(
                self.num_channels, dtype=np.single)
            self.offset: NDArray[np.single] = np.zeros(
                self.num_channels, dtype=np.single)

        positions: NDArray[np.single] = np.array(
            [recording.get_channel_property(ch, 'location')
             for ch in recording.get_channel_ids()],
            dtype=np.single)
        if positions.shape[1] > 2:
            warnings.warn(f'Channel locations have {positions.shape[1]} dimensions, '
                          'using the last two.')
            positions = positions[:, -2:]
        self.positions = np.ascontiguousarray(positions, dtype=np.single)

        self.spike_peak_duration = int(
            params['event_length'] * self.fps / 1000)
        self.noise_duration = int(params['peak_jitter'] * self.fps / 1000)
        self.noise_amp_percent: float = params['noise_amp_percent']
        self.neighbor_radius: float = params['neighbor_radius'] + \
            self.RADIUS_EPS
        self.inner_radius: float = params['inner_radius'] + self.RADIUS_EPS

        self.cutout_start = int(
            params['left_cutout_time'] * self.fps / 1000 + 0.5)
        self.cutout_end = int(
            params['right_cutout_time'] * self.fps / 1000 + 0.5)
        self.minsl = int(params['amp_evaluation_time'] * self.fps / 1000 + 0.5)
        self.maxsl = int(params['spk_evaluation_time'] * self.fps / 1000 + 0.5)
        self.cutout_length = self.cutout_start + 1 + self.cutout_end

        self.chunk_size: int = params['chunk_size']
        self.threshold: int = params['threshold']
        self.maa: int = params['maa']
        self.ahpthr: int = params['ahpthr']
        self.decay_filtering: bool = params['decay_filtering']

        self.common_reference: str = params['common_reference']
        if self.num_channels < 20 and self.common_reference in ['median', 'average']:
            warnings.warn(
                f'Number of channels too few for common {self.common_reference} reference')
        self.localize: bool = params['localize']
        self.save_shape: bool = params['save_shape']
        self.verbose: bool = params['verbose']

        if self.save_shape:
            out_file: Union[str, Path] = params['out_file']
            if isinstance(out_file, str):
                out_file = Path(out_file)
            out_file.parent.mkdir(parents=True, exist_ok=True)
            if out_file.suffix != '.bin':
                out_file = out_file.with_suffix('.bin')
            self.out_file = out_file
        else:
            self.out_file = None

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

    def get_traces(self, segment_index: int, start_frame: int, end_frame: int) -> NDArray[np.single]:
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

        traces_float: NDArray[np.single] = traces.astype(
            np.single, copy=False).reshape(-1)

        if lpad or rpad:
            traces_float = np.pad(traces_float, (lpad, rpad),
                                  mode='constant', constant_values=0)

        return traces_float

    def detect(self) -> Sequence[Mapping[str, RealArray]]:
        return [self.detect_seg(seg) for seg in range(self.num_segments)]

    def detect_seg(self, segment_index: int) -> Mapping[str, RealArray]:
        t_cut = self.cutout_start + self.maxsl
        t_cut2 = self.cutout_end + self.maxsl
        t_inc = min(self.chunk_size,
                    self.num_frames[segment_index] - t_cut - t_cut2)
        t_cut = 2048  # TODO: can be smaller? effect of band pass?

        scale: cython.float[:] = self.scale
        offset: cython.float[:] = self.offset
        position_matrix: cython.float[:, :] = self.positions
        out_file = self.out_file.with_stem(
            self.out_file.stem + f'-{segment_index}') if self.out_file else None
        det: cython.pointer(Detection) = new Detection(
            self.num_channels,
            t_inc,
            t_cut,
            self.rescale,
            cython.address(scale[0]),
            cython.address(offset[0]),
            self.common_reference == 'median',
            self.common_reference == 'average',
            self.maxsl - 1,  # TODO:??? -1
            self.minsl - 1,
            self.threshold,
            self.maa,
            self.ahpthr,
            cython.address(position_matrix[0, 0]),
            self.neighbor_radius,
            self.inner_radius,
            self.noise_duration,
            self.spike_peak_duration,
            self.decay_filtering,
            self.noise_amp_percent,
            self.localize,
            self.save_shape,
            str(out_file).encode(),
            self.cutout_start,
            self.cutout_length
        )

        vm: cython.float[:] = np.zeros(
            self.num_channels * (t_inc + t_cut + t_cut2), dtype=np.single)

        t0 = 0
        while t0 + t_inc + t_cut2 <= self.num_frames[segment_index]:
            t1 = t0 + t_inc
            if self.verbose:
                print(f'HSDetection: Analysing segment {segment_index}, '
                      f'frames from {t0:8d} to {t1:8d} '
                      f' ({100 * t0 / self.num_frames[segment_index]:.1f}%)')

            vm = self.get_traces(segment_index=segment_index,
                                 start_frame=t0 - t_cut, end_frame=t1 + t_cut2)
            det.step(cython.address(vm[0]), t0, t_inc)

            t0 += t_inc
            if t0 < self.num_frames[segment_index] - t_cut2:
                t_inc = min(
                    t_inc, self.num_frames[segment_index] - t_cut2 - t0)

        det_len = det.finish()
        det_res: cython.pointer(Spike) = det.getResult()

        channel_ind = np.empty(det_len, dtype=np.int32)
        sample_ind = np.empty(det_len, dtype=np.int32)
        amplitude = np.empty(det_len, dtype=np.int32)
        position = np.empty((det_len, 2), dtype=np.single)
        for i in range(det_len):
            sample_ind[i] = det_res[i].frame
            channel_ind[i] = det_res[i].channel
            amplitude[i] = det_res[i].amplitude
            position[i, 0] = det_res[i].position.x
            position[i, 1] = det_res[i].position.y

        del det

        if out_file and out_file.stat().st_size > 0:
            spikes: NDArray[np.int16] = np.memmap(
                str(out_file), dtype=np.int16, mode='r').reshape(-1, self.cutout_length)
        else:
            spikes: NDArray[np.int16] = np.empty(
                (0, self.cutout_length), dtype=np.int16)

        result: dict[str, RealArray] = {'channel_ind': channel_ind,
                                        'sample_ind': sample_ind,
                                        'amplitude': amplitude}
        if self.localize:
            result |= {'location': position}
        if self.save_shape:
            result |= {'spike_shape': spikes}
            if spikes.shape[0] < 10000:  # TODO:??? 64
                result['spike_shape'] = result['spike_shape'] // -64

        return result
