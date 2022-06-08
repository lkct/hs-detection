import warnings
from pathlib import Path
from typing import Union

import numpy as np
from numpy.typing import NDArray

from .recording import Recording


class HSDetection(object):
    """TODO:
    """

    def __init__(self,
                 recording: Recording,
                 noise_amp_percent: float = 1.0,
                 inner_radius: float = 70.0,
                 neighbor_radius: float = 90.0,
                 event_length: float = 0.26,
                 peak_jitter: float = 0.2,
                 to_localize: bool = True,
                 num_com_centers: int = 1,
                 threshold: int = 20,
                 maa: int = 12,
                 ahpthr: int = 11,
                 out_file: Union[str, Path] = 'HS2_detected',
                 decay_filtering: bool = False,
                 save_all: bool = False,
                 left_cutout_time: float = 0.3,
                 right_cutout_time: float = 1.8,
                 amp_evaluation_time: float = 0.4,
                 spk_evaluation_time: float = 1.0
                 ) -> None:
        self.recording = recording
        self.num_segments = recording.get_num_segments()  # TODO: seg proc
        self.num_frames = [recording.get_num_samples(seg)
                           for seg in range(self.num_segments)]
        self.fps = recording.get_sampling_frequency()
        self.num_channels = recording.get_num_channels()

        positions: NDArray[np.float64] = np.array([
            recording.get_channel_property(ch, 'location')
            for ch in recording.get_channel_ids()])
        if positions.shape[1] > 2:
            warnings.warn(f'Channel locations have {positions.shape[1]} dimensions, '
                          'using the last two.')
            positions = positions[:, -2:]

        self.positions = positions

        # using Euclidean distance, also possible to use Manhattan (ord=1)
        distances: NDArray[np.float64] = np.linalg.norm(
            positions[:, None] - positions[None, :], axis=2, ord=2)

        neighbors: list[NDArray[np.int64]] = [
            np.nonzero(dist < neighbor_radius)[0] for dist in distances]
        self.max_neighbors = max([n.shape[0] for n in neighbors])
        self.neighbors: NDArray[np.int64] = np.full(
            (self.num_channels, self.max_neighbors), -1, dtype=np.int64)
        for i, n in enumerate(neighbors):
            self.neighbors[i, :n.shape[0]] = n

        self.spike_peak_duration = int(event_length * self.fps / 1000)
        self.noise_duration = int(peak_jitter * self.fps / 1000)
        self.noise_amp_percent = noise_amp_percent
        self.inner_radius = inner_radius

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
