import warnings
from pathlib import Path
from typing import Any, Iterable, Optional, Union

import numpy as np
from numpy.typing import NDArray

from .recording import Recording


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
        self.nFrames = recording.get_num_samples(0)  # TODO: segment proc
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
        # astype cannot convert type annotation
        return self.recording.get_traces(
            start_frame=start_frame, end_frame=end_frame
        ).astype(np.short, copy=False).reshape(-1)  # type: ignore

        # self.sp_flat = np.memmap(file_name, dtype=np.int32, mode="r")
        # shapecache = self.sp_flat.reshape((-1, self.cutout_length + 5))
        # self.spikes = {
        #         "ch": shapecache[:, 0],
        #         "t": shapecache[:, 1],
        #         "Amplitude": shapecache[:, 2],
        #         "x": shapecache[:, 3] / 1000,
        #         "y": shapecache[:, 4] / 1000,
        #         "Shape": list(shapecache[:, 5:]),
        #     }
