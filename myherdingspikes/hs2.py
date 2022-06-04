import os


class HS2Detection(object):
    """ This class provides a simple interface to the detection, localisation of
    spike data from dense multielectrode arrays according to the methods
    described in the following papers:

    Muthmann, J. O., Amin, H., Sernagor, E., Maccione, A., Panas, D.,
    Berdondini, L., ... & Hennig, M. H. (2015). Spike detection for large neural
    populations using high density multielectrode arrays. Frontiers in
    neuroinformatics, 9.

    Hilgen, G., Sorbaro, M., Pirmoradian, S., Muthmann, J. O., Kepiro, I. E.,
    Ullo, S., ... & Hennig, M. H. (2017). Unsupervised spike sorting for
    large-scale, high-density multielectrode arrays. Cell reports, 18(10),
    2521-2532.

    Usage:
        1. Create a HSDetection object by calling its constructor with a
    Probe object and all the detection parameters (see documentation there).
        2. Call DetectFromRaw.
        3. Save the result, or create a HSClustering object.
    """

    def __init__(
        self,
        probe,
        to_localize=True,
        num_com_centers=1,
        threshold=20,
        maa=0,
        ahpthr=0,
        out_file_name="ProcessedSpikes",
        file_directory_name="",
        decay_filtering=False,
        save_all=False,
        left_cutout_time=1.0,
        right_cutout_time=2.2,
        amp_evaluation_time=0.4,  # former minsl
        spk_evaluation_time=1.7,
    ):  # former maxsl
        """
        Arguments:
        probe -- probe object, with a link to raw data
        to_localize -- set to False if spikes should only be detected, not
            localised (will break sorting)
        cutout_start -- deprecated, frame-based version of left_cutout_ms
        cutout_end -- deprecated, frame-based version of right_cutout_ms
        threshold -- detection threshold
        maa -- minimum average amplitude
        maxsl -- deprecated, frame-based version of spk_evaluation_time
        minsl -- deprecated, frame-based version of amp_evaluation_time
        ahpthr --
        out_file_name -- base file name (without extension) for the output files
        file_directory_name -- directory where output is saved
        save_all --
        left_cutout_ms -- the number of milliseconds, before the spike,
        to be included in the spike shape
        right_cutout_ms -- the number of milliseconds, after the spike,
        to be included in the spike shape
        amp_evaluation_time -- the number of milliseconds during which the trace
        is integrated, for the purposed of evaluating amplitude, used for later
        comparison with 'maa'
        spk_evaluation_time -- dead time in ms after spike peak, used for
        further triaging of spike shape
        """
        self.probe = probe

        self.cutout_start = int(left_cutout_time * self.probe.fps / 1000 + 0.5)
        self.cutout_end = int(right_cutout_time * self.probe.fps / 1000 + 0.5)
        self.minsl = int(amp_evaluation_time * self.probe.fps / 1000 + 0.5)
        self.maxsl = int(spk_evaluation_time * self.probe.fps / 1000 + 0.5)

        self.to_localize = to_localize
        self.cutout_length = self.cutout_start + self.cutout_end + 1
        self.threshold = threshold
        self.maa = maa
        self.ahpthr = ahpthr
        self.decay_filtering = decay_filtering
        self.num_com_centers = num_com_centers
        self.sp_flat = None
        self.spikes = {}

        # Make directory for results if it doesn't exist
        os.makedirs(file_directory_name, exist_ok=True)

        if out_file_name[-4:] == ".bin":
            file_path = file_directory_name + out_file_name
            self.out_file_name = file_path
        else:
            file_path = os.path.join(file_directory_name, out_file_name)
            self.out_file_name = file_path + ".bin"
        self.save_all = save_all

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
