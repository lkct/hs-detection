import numpy as np
import os
from .detection_localisation.detect import detectData
import warnings


class HSDetection(object):
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
        cutout_start=None,
        cutout_end=None,
        threshold=20,
        maa=0,
        maxsl=None,
        minsl=None,
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

        self.cutout_start = self._deprecate_or_convert(
            cutout_start, left_cutout_time, "cutout_start", "left_cutout_time"
        )
        self.cutout_end = self._deprecate_or_convert(
            cutout_end, right_cutout_time, "cutout_end", "right_cutout_time"
        )
        self.minsl = self._deprecate_or_convert(
            minsl, amp_evaluation_time, "minsl", "amp_evaluation_time"
        )
        self.maxsl = self._deprecate_or_convert(
            maxsl, spk_evaluation_time, "maxsl", "spk_evaluation_time"
        )

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

    def _deprecate_or_convert(self, old_var, new_var, old_name, new_name):
        if old_var is not None:
            msg = (
                "{} is deprecated and will be removed. Set {} instead "
                "(in milliseconds). {} takes priority over {}!"
            )
            warnings.warn(msg.format(old_name, new_name, old_name, new_name))
            return int(old_var)
        else:
            return int(new_var * self.probe.fps / 1000 + 0.5)

    def SetAddParameters(self, dict_of_new_parameters):
        """
         Adds and merges dict_of_new_parameters with the current fields of the
         object. Uses the PEP448 convention to group two dics together.

         Arguments:
         dict_of_new_parameters -- the dictionary of parameters to be updated.
        """
        self.__dict__.update(dict_of_new_parameters)

    def LoadDetected(self, file_name=None):
        """
        Reads a binary file with spikes detected with the DetectFromRaw()
        method.

        Arguments:
        file_name -- The name of the .bin file. Defaults to self.out_file_name.
        """
        if file_name is None:
            file_name = self.out_file_name

        if os.stat(file_name).st_size == 0:
            shapecache = np.asarray([]).reshape(0, 5)
            warnings.warn(
                "Loading an empty file {} . This usually happens when no spikes were"
                "detected due to the detection parameters being set too "
                "strictly".format(file_name)
            )
        else:
            self.sp_flat = np.memmap(file_name, dtype=np.int32, mode="r")
            assert self.sp_flat.shape[0] // self.cutout_length + 5 is \
                not self.sp_flat.shape[0] / self.cutout_length + 5, \
                "spike data has wrong dimensions"  # ???
            shapecache = self.sp_flat.reshape((-1, self.cutout_length + 5))

        self.spikes = {
                "ch": shapecache[:, 0],
                "t": shapecache[:, 1],
                "Amplitude": shapecache[:, 2],
                "x": shapecache[:, 3] / 1000,
                "y": shapecache[:, 4] / 1000,
                "Shape": list(shapecache[:, 5:]),
            }
        self.IsClustered = False
        print("Loaded " + str(self.spikes['ch'].shape[0]) + " spikes.")

    def DetectFromRaw(
        self, load=False, nFrames=None, tInc=50000, recording_duration=None
    ):
        """
        This function is a wrapper of the C function `detectData`. It takes
        the raw data file, performs detection and localisation, saves the result
        to HSDetection.out_file_name and loads the latter into memory by calling
        LoadDetected if load=True.

        Arguments:
        load -- bool: load the detected spikes when finished?
        nFrames -- deprecated, frame-based version of recording_duration
        tInc -- size of chunks to be read
        recording_duration -- maximum time limit of recording (the rest will be
        ignored)
        """

        if nFrames is not None:
            warnings.warn(
                "nFrames is deprecated and will be removed. Leave "
                "this out if you want to read the whole recording, "
                "or set max_duration to set the limit (in seconds)."
            )
        elif recording_duration is not None:
            nFrames = int(recording_duration * self.probe.fps)

        detectData(
            probe=self.probe,
            file_name=str.encode(self.out_file_name[:-4]),
            to_localize=self.to_localize,
            sf=self.probe.fps,
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
            nFrames=nFrames,
            tInc=tInc,
        )

        if load:
            # reload data into memory (detect saves it on disk)
            self.LoadDetected()
