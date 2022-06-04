from typing import Optional
from .detection_localisation.detect import detectData
from .hs2 import HS2Detection


def DetectFromRaw(det: HS2Detection, tInc: int = 50000, recording_duration: Optional[float] = None):
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

    if recording_duration is not None:
        nFrames = int(recording_duration * det.fps)
    else:
        nFrames = None

    detectData(
        probe=det,
        file_name=str.encode(det.out_file_name[:-4]),
        to_localize=det.to_localize,
        sf=det.fps,
        thres=det.threshold,
        cutout_start=det.cutout_start,
        cutout_end=det.cutout_end,
        maa=det.maa,
        maxsl=det.maxsl,
        minsl=det.minsl,
        ahpthr=det.ahpthr,
        num_com_centers=det.num_com_centers,
        decay_filtering=det.decay_filtering,
        verbose=det.save_all,
        nFrames=nFrames,
        tInc=tInc,
    )
