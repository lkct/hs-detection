import numpy as np
from .neighborMatrixUtils import createNeighborMatrix
import ctypes
import os

this_file_path = os.path.dirname(os.path.abspath(__file__))

DEFAULT_EVENT_LENGTH = 0.5
DEFAULT_PEAK_JITTER = 0.2


def create_probe_files(pos_file, neighbor_file, radius, ch_positions):
    n_channels = ch_positions.shape[0]
    # NB: Notice the column, row order in write
    with open(pos_file, "w") as f:
        for pos in ch_positions:
            f.write("{},{},\n".format(pos[0], pos[1]))
    f.close()
    # using Euclidean distance, also possible to use Manhattan (ord=1)
    distances = np.linalg.norm(
        ch_positions[:, None] - ch_positions[None, :], axis=2, ord=2)
    indices = np.arange(n_channels)
    with open(neighbor_file, "w") as f:
        for dist_from_ch in distances:
            neighbors = indices[dist_from_ch <= radius]
            f.write("{},\n".format(str(list(neighbors))[1:-1]))
    f.close()

def in_probes_dir(file):
    probe_path1 = os.getenv('HS2_PROBE_PATH', this_file_path)    
    probe_path = os.path.join(probe_path1, "probes")
    if not os.path.exists(probe_path):
        os.mkdir(probe_path)
    return os.path.join(probe_path, file)

def in_probe_info_dir(file):
    probe_path1 = os.getenv('HS2_PROBE_PATH', this_file_path)    
    probe_path = os.path.join(probe_path1, "probe_info")
    if not os.path.exists(probe_path):
        os.mkdir(probe_path)
    return os.path.join(probe_path, file)

class NeuralProbe(object):
    def __init__(
        self,
        num_channels,
        noise_amp_percent,
        inner_radius,
        fps,
        positions_file_path,
        neighbors_file_path,
        neighbor_radius,
        event_length,
        peak_jitter,
        masked_channels=[],
    ):
        if neighbor_radius is not None:
            createNeighborMatrix(
                neighbors_file_path, positions_file_path, neighbor_radius
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

    def Read(self, t0, t1):
        raise NotImplementedError(
            "The Read function is not implemented for \
            this probe"
        )


class RecordingExtractor(NeuralProbe):
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
        positions_file_path = in_probe_info_dir("positions_spikeextractor")
        neighbors_file_path = in_probe_info_dir("neighbormatrix_spikeextractor")
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
            positions_file_path, neighbors_file_path, inner_radius, ch_positions
        )

        NeuralProbe.__init__(
            self,
            num_channels=num_channels,
            noise_amp_percent=noise_amp_percent,
            fps=fps,
            inner_radius=inner_radius,
            positions_file_path=positions_file_path,
            neighbors_file_path=neighbors_file_path,
            masked_channels=masked_channels,
            neighbor_radius=neighbor_radius,
            event_length=event_length,
            peak_jitter=peak_jitter,
        )

    def Read(self, t0, t1):
        return (
            self.d.get_traces(
                channel_ids=self.d.get_channel_ids(), start_frame=t0, end_frame=t1
            )
            .ravel()
            .astype(ctypes.c_short)
        )
