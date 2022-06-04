import ctypes

import numpy as np

from .recording import Recording

DEFAULT_EVENT_LENGTH = 0.5
DEFAULT_PEAK_JITTER = 0.2


class RecordingExtractor(object):  # NeuralProbe
    def __init__(
        self,
        recording: Recording,
        noise_amp_percent=1,
        inner_radius=60,
        neighbor_radius=60,
        masked_channels=None,
        xy=None,
        event_length=DEFAULT_EVENT_LENGTH,
        peak_jitter=DEFAULT_PEAK_JITTER,
    ):
        self.recording = recording
        try:
            self.nFrames = recording.get_num_samples()
        except:
            self.nFrames = recording.get_num_samples(0)
        num_channels = recording.get_num_channels()
        fps = recording.get_sampling_frequency()
        ch_positions = np.array(
            [
                np.array(recording.get_channel_property(ch, "location"))
                for ch in recording.get_channel_ids()
            ]
        )
        if ch_positions.shape[1] > 2:
            if xy is None:
                print(
                    "# Warning: channel locations have",
                    ch_positions.shape[1],
                    "dimensions",
                )
                print("#          using the last two.")
                xy = (ch_positions.shape[1] - 2, ch_positions.shape[1] - 1)
            ch_positions = ch_positions[:, xy]

        self.positions = ch_positions

        # using Euclidean distance, also possible to use Manhattan
        distances = np.linalg.norm(
            ch_positions[:, None] - ch_positions[None, :], axis=2, ord=2)

        neighbors = []
        for dist_from_ch in distances:
            neighbors.append(np.nonzero(dist_from_ch < neighbor_radius)[0])
        self.neighbors = neighbors
        self.max_neighbors = max([len(n) for n in neighbors])

        self.fps = fps
        self.num_channels = num_channels
        # to num frames
        self.spike_peak_duration = int(event_length * self.fps / 1000)
        self.noise_duration = int(peak_jitter * self.fps / 1000)
        self.noise_amp_percent = noise_amp_percent
        self.positions_file_path = ''
        self.neighbors_file_path = ''
        self.masked_channels = masked_channels
        self.inner_radius = inner_radius
        if masked_channels is None:
            self.masked_channels = []

    def Read(self, t0, t1):
        return self.recording.get_traces(
            channel_ids=self.recording.get_channel_ids(), start_frame=t0, end_frame=t1
        ).ravel().astype(ctypes.c_short)
