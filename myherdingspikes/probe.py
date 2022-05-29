import numpy as np
from matplotlib import pyplot as plt
import ctypes


DEFAULT_EVENT_LENGTH = 0.5
DEFAULT_PEAK_JITTER = 0.2


def create_probe_files(pos_file, neighbor_file, radius, ch_positions):
    n_channels = ch_positions.shape[0]
    # NB: Notice the column, row order in write
    np.savetxt(pos_file, ch_positions, fmt='%.1f', delimiter=',', newline=',\n')
    # using Euclidean distance, also possible to use Manhattan
    distances = np.linalg.norm(
        ch_positions[:, None] - ch_positions[None, :], axis=2, ord=2)
    with open(neighbor_file, "w") as f:
        for dist_from_ch in distances:
            neighbors = np.nonzero(dist_from_ch < radius)[0]
            f.write(','.join(map(str, list(neighbors))) + ',\n')
    f.close()


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
        positions_file_path = "/tmp/tmp_probe_info/positions_spikeextractor"
        neighbors_file_path = "/tmp/tmp_probe_info/neighbormatrix_spikeextractor"
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
        print("# Generating new position and neighbor files from data file")
        create_probe_files(
            positions_file_path, neighbors_file_path, neighbor_radius, ch_positions
        )

        self.fps = fps
        self.num_channels = num_channels
        self.spike_peak_duration = int(event_length * self.fps / 1000)
        self.noise_duration = int(peak_jitter * self.fps / 1000)
        self.noise_amp_percent = noise_amp_percent
        self.positions_file_path = positions_file_path
        self.neighbors_file_path = neighbors_file_path
        self.masked_channels = masked_channels
        self.inner_radius = inner_radius
        if masked_channels is None:
            self.masked_channels = []

        self.loadPositions(positions_file_path)
        self.loadNeighbors(neighbors_file_path)

    # Load in neighbor and positions files
    def loadNeighbors(self, neighbors_file_path):
        neighbor_file = open(neighbors_file_path, "r")
        neighbors = []
        for neighbor in neighbor_file.readlines():
            neighbors.append(np.array(neighbor[:-2].split(",")).astype(int))
        neighbor_file.close()
        # assert len(neighbors) == len(pos)
        self.neighbors = neighbors
        self.max_neighbors = max([len(n) for n in neighbors])

    def loadPositions(self, positions_file_path):
        position_file = open(positions_file_path, "r")
        positions = []
        for position in position_file.readlines():
            positions.append(np.array(position[:-2].split(",")).astype(float))
        self.positions = np.asarray(positions)
        position_file.close()

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
