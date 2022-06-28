# distutils: language=c++
# cython: language_level=3
# cython: annotation_typing=False

import warnings
from pathlib import Path
from typing import Mapping, Optional, Sequence

import cython
import numpy as np
from numpy.typing import NDArray

from ..recording import RealArray, Recording
from . import Params

bool_t = cython.typedef(_bool)  # type: ignore
int32_t = cython.typedef(_int32_t)  # type: ignore
single = cython.typedef(cython.float)  # type: ignore
p_single = cython.typedef(cython.p_float)  # type: ignore
vector_i32 = cython.typedef(vector[int32_t])  # type: ignore

RADIUS_EPS: float = cython.declare(single, 1e-3)  # type: ignore  # 1nm


@cython.cclass
class HSDetection(object):
    """TODO:
    """

    recording: Recording = cython.declare(object)  # type: ignore
    num_segments: int = cython.declare(int32_t)  # type: ignore
    num_frames: list[int] = cython.declare(vector_i32)  # type: ignore

    num_channels: int = cython.declare(int32_t)  # type: ignore
    chunk_size: int = cython.declare(int32_t)  # type: ignore
    t_cut: int = cython.declare(int32_t)  # type: ignore

    rescale: bool = cython.declare(bool_t)  # type: ignore
    scale: NDArray[np.single] = cython.declare(np.ndarray)  # type: ignore
    offset: NDArray[np.single] = cython.declare(np.ndarray)  # type: ignore

    median_reference: bool = cython.declare(bool_t)  # type: ignore
    average_reference: bool = cython.declare(bool_t)  # type: ignore

    maxsl: int = cython.declare(int32_t)  # type: ignore
    minsl: int = cython.declare(int32_t)  # type: ignore
    threshold: float = cython.declare(single)  # type: ignore
    maa: float = cython.declare(single)  # type: ignore
    ahpthr: float = cython.declare(single)  # type: ignore

    positions: NDArray[np.single] = cython.declare(np.ndarray)  # type: ignore
    neighbor_radius: float = cython.declare(single)  # type: ignore
    inner_radius: float = cython.declare(single)  # type: ignore

    noise_duration: int = cython.declare(int32_t)  # type: ignore
    spike_peak_duration: int = cython.declare(int32_t)  # type: ignore

    decay_filtering: bool = cython.declare(bool_t)  # type: ignore
    noise_amp_percent: float = cython.declare(single)  # type: ignore

    localize: bool = cython.declare(bool_t)  # type: ignore

    save_shape: bool = cython.declare(bool_t)  # type: ignore
    out_file: Optional[Path] = cython.declare(object)  # type: ignore
    cutout_start: int = cython.declare(int32_t)  # type: ignore
    cutout_end: int = cython.declare(int32_t)  # type: ignore
    cutout_length: int = cython.declare(int32_t)  # type: ignore

    verbose: bool = cython.declare(bool_t)  # type: ignore

    @cython.locals(recording=object, params=object,
                   fps=single, float_param=single,
                   num_channels=object, common_reference=object, out_file=object,
                   l=np.ndarray, m=np.ndarray, r=np.ndarray,
                   positions=np.ndarray)
    def __init__(self, recording: Recording, params: Params) -> None:
        self.recording = recording
        self.num_segments = recording.get_num_segments()
        self.num_frames = [recording.get_num_samples(seg)
                           for seg in range(self.num_segments)]
        fps = recording.get_sampling_frequency()

        self.num_channels = num_channels = recording.get_num_channels()
        self.chunk_size = params['chunk_size']

        self.rescale = params['rescale']
        if self.rescale:
            l, m, r = np.quantile(self.get_random_data_chunks(),
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
                num_channels, dtype=np.single)
            self.offset: NDArray[np.single] = np.zeros(
                num_channels, dtype=np.single)

        common_reference = params['common_reference']
        self.median_reference = common_reference == 'median'
        self.average_reference = common_reference == 'average'
        if self.num_channels < 20 and (self.median_reference or self.average_reference):
            warnings.warn(
                f'Number of channels too few for common {common_reference} reference')

        float_param = params['spk_evaluation_time']
        self.maxsl = int(float_param * fps / 1000 + 0.5)
        float_param = params['amp_evaluation_time']
        self.minsl = int(float_param * fps / 1000 + 0.5)
        self.threshold = params['threshold']
        self.maa = params['maa']
        self.ahpthr = params['ahpthr']  # TODO:??? should be negative
        self.maxsl -= 1  # TODO:??? -1
        self.minsl -= 1
        self.threshold /= 2  # TODO:??? /2
        self.maa /= 2

        positions: NDArray[np.single] = np.array(
            [recording.get_channel_property(ch, 'location')
             for ch in recording.get_channel_ids()],
            dtype=np.single)
        if positions.shape[1] > 2:
            warnings.warn(f'Channel locations have {positions.shape[1]} dimensions, '
                          'using the last two.')
            positions = positions[:, -2:]
        self.positions: NDArray[np.single] = np.ascontiguousarray(
            positions, dtype=np.single)
        self.neighbor_radius = params['neighbor_radius']
        self.neighbor_radius += RADIUS_EPS
        self.inner_radius = params['inner_radius']
        self.inner_radius += RADIUS_EPS

        float_param = params['peak_jitter']
        self.noise_duration = int(float_param * fps / 1000 + 0.5)
        float_param = params['event_length']
        self.spike_peak_duration = int(float_param * fps / 1000 + 0.5)

        self.decay_filtering = params['decay_filtering']
        self.noise_amp_percent = params['noise_amp_percent']

        self.localize = params['localize']

        self.save_shape = params['save_shape']
        if self.save_shape:
            out_file = params['out_file']
            if isinstance(out_file, str):
                out_file = Path(out_file)
            out_file.parent.mkdir(parents=True, exist_ok=True)
            if out_file.suffix != '.bin':
                out_file = out_file.with_suffix('.bin')
            self.out_file = out_file
        else:
            self.out_file = None
        float_param = params['left_cutout_time']
        self.cutout_start = int(float_param * fps / 1000 + 0.5)
        float_param = params['right_cutout_time']
        self.cutout_end = int(float_param * fps / 1000 + 0.5)
        self.cutout_length = self.cutout_start + 1 + self.cutout_end

        self.t_cut = self.noise_duration + max(self.cutout_length,
                                               self.spike_peak_duration + 1 + self.maxsl)

        self.verbose = params['verbose']

    @cython.cfunc
    @cython.locals(num_chunks_per_segment=object, chunk_size=object, seed=object,
                   chunks=list, seg=int32_t, random_starts=np.ndarray, start_frame=object)
    @cython.returns(np.ndarray)
    def get_random_data_chunks(self,
                               num_chunks_per_segment: int = 20,
                               chunk_size: int = 10000,
                               seed: int = 0
                               ) -> NDArray[np.float32]:
        # TODO: sample uniformly on samples instead of segments
        chunks: list[RealArray] = []
        for seg in range(self.num_segments):
            # TODO: new generator api, or use c++ random
            random_starts = np.random.RandomState(seed).randint(
                0, self.num_frames[seg] - chunk_size, size=num_chunks_per_segment)
            for start_frame in random_starts:
                chunks.append(self.recording.get_traces(
                    segment_index=seg, start_frame=start_frame, end_frame=start_frame + chunk_size))
        return np.concatenate(chunks, axis=0, dtype=np.float32)

    @cython.cfunc
    @cython.locals(segment_index=int32_t, start_frame=int32_t, end_frame=int32_t,
                   lpad=int32_t, rpad=int32_t,
                   traces=np.ndarray, traces_float=np.ndarray)
    @cython.returns(np.ndarray)
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

    @cython.ccall
    @cython.returns(object)
    def detect(self) -> Sequence[Mapping[str, RealArray]]:
        return [self.detect_seg(seg) for seg in range(self.num_segments)]

    @cython.cfunc
    @cython.locals(segment_index=int32_t,
                   t_cut=int32_t, t_cut2=int32_t, t_inc=int32_t,
                   out_file=object,
                   t0=int32_t, t1=int32_t, t_end=int32_t,
                   vm=np.ndarray,
                   channel_ind=np.ndarray, sample_ind=np.ndarray,
                   amplitude=np.ndarray, location=np.ndarray,
                   spikes=np.ndarray,
                   result=dict)
    @cython.returns(object)
    def detect_seg(self, segment_index: int) -> Mapping[str, RealArray]:
        t_cut = self.cutout_start + self.maxsl
        t_cut2 = self.cutout_end + self.maxsl
        t_end = self.num_frames[segment_index] - t_cut2
        t_inc = min(self.chunk_size, t_end - t_cut)

        out_file = None if self.out_file is None \
            else self.out_file.with_stem(f'{self.out_file.stem}-{segment_index}')

        det = newDet(  # type: ignore
            self.num_channels,
            t_inc,
            self.t_cut,
            self.rescale,
            cython.cast(p_single, self.scale.data),
            cython.cast(p_single, self.offset.data),
            self.median_reference,
            self.average_reference,
            self.maxsl,
            self.minsl,
            self.threshold,
            self.maa,
            self.ahpthr,
            cython.cast(p_single, self.positions.data),
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
            self.cutout_end
        )

        t0 = 0
        while t0 + t_inc <= t_end:
            t1 = t0 + t_inc
            if self.verbose:
                print(f'HSDetection: Analysing segment {segment_index}, '
                      f'frames from {t0:8d} to {t1:8d} '
                      f' ({100 * t0 / t_end:.1f}%)')

            vm = self.get_traces(segment_index=segment_index,
                                 start_frame=t0 - self.t_cut, end_frame=t1 + t_cut2)
            det.step(cython.cast(p_single, vm.data), t0, t_inc)

            t0 += t_inc
            if t0 < t_end:
                t_inc = min(t_inc, t_end - t0)

        det_len = det.finish()
        det_res = det.getResult()

        channel_ind = np.empty(det_len, dtype=np.int32)
        sample_ind = np.empty(det_len, dtype=np.int32)
        amplitude = np.empty(det_len, dtype=np.int32)
        location = np.empty((det_len, 2), dtype=np.single)
        for i in range(det_len):
            sample_ind[i] = det_res[i].frame
            channel_ind[i] = det_res[i].channel
            amplitude[i] = det_res[i].amplitude
            location[i, 0] = det_res[i].position.x
            location[i, 1] = det_res[i].position.y

        delDet(det)  # type: ignore

        if out_file is not None and out_file.stat().st_size > 0:
            spikes: NDArray[np.int16] = np.memmap(
                str(out_file), dtype=np.int16, mode='r').reshape(-1, self.cutout_length)
        else:
            spikes: NDArray[np.int16] = np.empty(
                (0, self.cutout_length), dtype=np.int16)

        result: dict[str, RealArray] = {'channel_ind': channel_ind,
                                        'sample_ind': sample_ind,
                                        'amplitude': amplitude}
        if self.localize:
            result |= {'location': location}
        if self.save_shape:
            result |= {'spike_shape': spikes}
            if spikes.shape[0] < 10000:  # TODO:??? 64
                result['spike_shape'] = result['spike_shape'] // -64

        return result
