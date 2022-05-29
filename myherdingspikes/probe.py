import numpy as np
from matplotlib import pyplot as plt
import ctypes


DEFAULT_EVENT_LENGTH = 0.5
DEFAULT_PEAK_JITTER = 0.2


class RecordingExtractor(object):  # NeuralProbe
    def __init__(
        self,
        re,
        noise_amp_percent=1,
        inner_radius=60,
        neighbor_radius=60,
        masked_channels=None,
        xy=None,
        event_length=DEFAULT_EVENT_LENGTH,
        peak_jitter=DEFAULT_PEAK_JITTER,
    ):
        self.d = re
        try:
            self.nFrames = re.get_num_frames()
        except:
            self.nFrames = re.get_num_frames(0)
        num_channels = re.get_num_channels()
        fps = re.get_sampling_frequency()
        ch_positions = np.array(
            [
                np.array(re.get_channel_property(ch, "location"))
                for ch in re.get_channel_ids()
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

        self.neighbors = []
        for dist_from_ch in distances:
            self.neighbors.append(np.nonzero(
                dist_from_ch < neighbor_radius)[0])
        self.max_neighbors = max([len(n) for n in self.neighbors])

        self.fps = fps
        self.num_channels = num_channels
        self.spike_peak_duration = int(event_length * self.fps / 1000)
        self.noise_duration = int(peak_jitter * self.fps / 1000)
        self.noise_amp_percent = noise_amp_percent
        self.positions_file_path = "/tmp/tmp_probe_info/positions_spikeextractor"
        self.neighbors_file_path = "/tmp/tmp_probe_info/neighbormatrix_spikeextractor"
        self.masked_channels = masked_channels
        self.inner_radius = inner_radius
        if masked_channels is None:
            self.masked_channels = []

    # Show visualization of probe
    def show(self, show_neighbors=[10], figwidth=3):
        xmax, ymax = self.positions.max(0)
        xmin, ymin = self.positions.min(0)
        ratio = ymax / xmax
        plt.figure(figsize=(figwidth, figwidth * ratio))
        for ch in show_neighbors:
            for neighbor in self.neighbors[ch]:
                plt.plot(
                    [self.positions[ch, 0], self.positions[neighbor, 0]],
                    [self.positions[ch, 1], self.positions[neighbor, 1]],
                    "--k",
                    alpha=0.7,
                )
        plt.scatter(*self.positions.T)
        plt.scatter(*self.positions[self.masked_channels].T, c="r")
        for i, pos in enumerate(self.positions):
            plt.annotate(f'{i}', pos)

    def Read(self, t0, t1):
        return (
            self.d.get_traces(
                channel_ids=self.d.get_channel_ids(), start_frame=t0, end_frame=t1
            )
            .ravel()
            .astype(ctypes.c_short)
        )

    def getChannelsPositions(self, channels):
        channel_positions = []
        for channel in channels:
            if channel >= self.num_channels:
                raise ValueError(
                    "Channel index too large, maximum " + self.num_channels
                )
            else:
                channel_positions.append(self.positions[channel])
        return channel_positions
