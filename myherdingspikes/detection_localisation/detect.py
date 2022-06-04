# distutils: language=c++
# cython: language_level=3

import numpy as np
from datetime import datetime
import pprint
import cython
import warnings
from pathlib import Path
from typing import Iterable, Optional, Union

import numpy as np
from numpy.typing import NDArray

from ..recording import Recording


class HS2Detection(object):
    """TODO:
    """

    def __init__(self,
                 recording: Recording,
                 noise_amp_percent: float = 1.0,
                 inner_radius: float = 70.0,
                 neighbor_radius: float = 90.0,
                 masked_channels: Optional[Iterable[int]] = None,  # indices
                 # xy: Optional[Union[tuple[int, int], slice]] = None,
                 event_length: float = 0.26,
                 peak_jitter: float = 0.2,
                 to_localize: bool = True,
                 num_com_centers: int = 1,
                 threshold: int = 20,
                 maa: int = 0,
                 ahpthr: int = 0,
                 out_file: Union[str, Path] = 'HS2_detected',
                 decay_filtering: bool = False,
                 save_all: bool = False,
                 left_cutout_time: float = 1.0,
                 right_cutout_time: float = 2.2,
                 amp_evaluation_time: float = 0.4,
                 spk_evaluation_time: float = 1.7,
                 ) -> None:
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
            np.nonzero(dist_from_ch < neighbor_radius)[0] for dist_from_ch in distances]
        self.max_neighbors = max([n.shape[0] for n in neighbors])
        self.neighbors = np.array([
            np.pad(n, (0, self.max_neighbors - n.shape[0]),
                   mode='constant', constant_values=-1)
            for n in neighbors])

        self.spike_peak_duration = int(event_length * self.fps / 1000)  # frame
        self.noise_duration = int(peak_jitter * self.fps / 1000)  # frame
        self.noise_amp_percent = noise_amp_percent
        self.inner_radius = inner_radius
        if masked_channels is not None:
            self.masked_channels = masked_channels
        else:
            self.masked_channels: Iterable[int] = []

        self.cutout_start = int(left_cutout_time * self.fps / 1000 + 0.5)
        self.cutout_end = int(right_cutout_time * self.fps / 1000 + 0.5)
        self.minsl = int(amp_evaluation_time * self.fps / 1000 + 0.5)
        self.maxsl = int(spk_evaluation_time * self.fps / 1000 + 0.5)

        self.to_localize = to_localize
        self.cutout_length = self.cutout_start + self.cutout_end + 1
        self.threshold = threshold
        self.maa = maa
        self.ahpthr = ahpthr
        self.decay_filtering = decay_filtering
        self.num_com_centers = num_com_centers

        if isinstance(out_file, str):
            out_file = Path(out_file)
        out_file.parent.mkdir(parents=True, exist_ok=True)

        if out_file.suffix != '.bin':
            out_file = out_file.with_suffix('.bin')
        self.out_file_name = str(out_file)

        self.save_all = save_all

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

        # astype cannot convert type annotation
        traces: NDArray[np.short] = self.recording.get_traces(
            start_frame=start_frame, end_frame=end_frame
        ).astype(np.short, copy=False).reshape(-1)  # type: ignore

        if lpad + rpad > 0:
            traces = np.pad(traces, (lpad, rpad),
                            mode='constant', constant_values=0)

        return traces

    def detect(self, t_inc: int = 50000, recording_duration: Optional[float] = None) -> None:
        if recording_duration is not None:
            num_frames = int(recording_duration * self.fps)
        else:
            num_frames = self.num_frames

        self._detect(
            file_name=str.encode(self.out_file_name[:-4]),
            to_localize=self.to_localize,
            sf=int(self.fps),
            thres=self.threshold,
            cutout_start=self.cutout_start,
            cutout_end=self.cutout_end,
            maa=self.maa,
            maxsl=self.maxsl,
            minsl=self.minsl,
            ahpthr=self.ahpthr,
            num_com_centers=self.num_com_centers,
            decay_filtering=self.decay_filtering,
            verbose=self.save_all,
            num_frames=num_frames,
            t_inc=t_inc,
        )

    def _detect(self,
                file_name: bytes,
                to_localize: bool,
                sf: int,
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

        # READ PROBE PARAMETERS
        sf = int(sf)  # ensure sampling rate is integer, assumed to be in Hertz
        num_channels = int(self.num_channels)
        spike_peak_duration = int(self.spike_peak_duration)
        noise_duration = int(self.noise_duration)
        noise_amp_percent = float(self.noise_amp_percent)
        max_neighbors = int(self.max_neighbors)
        inner_radius = float(self.inner_radius)

        masked_channel_list = self.masked_channels
        masked_channels: cython.int[:] = np.ones(num_channels, dtype=np.intc)

        if masked_channel_list == []:
            print('# Not Masking any Channels')
        else:
            print(f'# Masking Channels: {masked_channel_list}')

        for channel in masked_channel_list:
            masked_channels[channel] = 0

        print(f'# Sampling rate: {sf}')
        if to_localize:
            print('# Localization On')
        else:
            print('# Localization Off')
        if verbose:
            print('# Writing out extended detection info')
        print(f'# Number of recorded channels: {num_channels}')
        if num_channels < 20:
            print('# Few recording channels: not subtracing mean from activity')
        print(f'# Analysing frames: {num_frames}; Seconds: {num_frames / sf}')
        print(f'# Frames before spike in cutout: {cutout_start}')
        print(f'# Frames after spike in cutout: {cutout_end}')

        det: cython.pointer(Detection) = new Detection()

        # set tCut, tCut2 and tInc
        t_cut_l = cutout_start + maxsl
        t_cnt_r = cutout_end + maxsl
        print(f'# tcuts: {t_cut_l} {t_cnt_r}')

        # cap at specified number of frames
        t_inc = min(num_frames - t_cut_l - t_cnt_r, t_inc)
        print(f'# tInc: {t_inc}')
        # ! To be consistent, X and Y have to be swappped
        ch_indices: cython.long[:] = np.arange(num_channels, dtype=np.int_)
        vm: cython.short[:] = np.zeros(
            num_channels * (t_inc + t_cut_l + t_cnt_r), dtype=np.short)

        # initialise detection algorithm
        det.InitDetection(num_frames, sf, num_channels, t_inc,
                          cython.address(ch_indices[0]), 0)

        position_matrix: cython.int[:, :] = np.ascontiguousarray(
            self.positions, dtype=np.intc)
        neighbor_matrix: cython.int[:, :] = np.ascontiguousarray(
            self.neighbors, dtype=np.intc)

        det.SetInitialParams(cython.address(position_matrix[0, 0]), cython.address(neighbor_matrix[0, 0]), num_channels,
                             spike_peak_duration, file_name, noise_duration,
                             noise_amp_percent, inner_radius, cython.address(
            masked_channels[0]),
            max_neighbors, num_com_centers, to_localize,
            thres, cutout_start, cutout_end, maa, ahpthr, maxsl,
            minsl, decay_filtering, verbose)

        startTime = datetime.now()
        t0 = 0
        max_frames_processed = t_inc
        while t0 + t_inc + t_cnt_r <= num_frames:
            t1 = t0 + t_inc
            if verbose:
                print(f'# Analysing frames from {t0 - t_cut_l} to {t1 + t_cnt_r} '
                      f' ({100 * t0 / num_frames:.1f}%)')

            vm = self.get_traces(t0 - t_cut_l, t1 + t_cnt_r)

            # detect spikes
            if num_channels >= 20:
                det.MeanVoltage(cython.address(vm[0]), t_inc, t_cut_l)
            det.Iterate(cython.address(vm[0]), t0,
                        t_inc, t_cut_l, t_cnt_r, max_frames_processed)

            t0 += t_inc
            if t0 < num_frames - t_cnt_r:
                t_inc = min(t_inc, num_frames - t_cnt_r - t0)

        now = datetime.now()
        # Save state of detection
        detection_state_dict = {
            'Probe Name': self.__class__.__name__,
            'Probe Object': self,
            'Date and Time Detection': str(now),
            'Threshold': thres,
            'Localization': to_localize,
            'Masked Channels': masked_channel_list,
            'Associated Results File': file_name,
            'Cutout Length': cutout_start + cutout_end,
            'Advice': 'For more information about detection, load and look at the parameters of the probe object',
        }

        with open(f'{file_name.decode()}DetectionDict{now.strftime("%Y-%m-%d_%H%M%S_%f")}.txt', 'a') as f:
            f.write(pprint.pformat(detection_state_dict))

        det.FinishDetection()

        t = datetime.now() - startTime
        print(f'# Detection completed, time taken: {t}')
        print(f'# Time per frame: {1000 * t / num_frames}')
        print(f'# Time per sample: {1000 * t / (num_channels * num_frames)}')
