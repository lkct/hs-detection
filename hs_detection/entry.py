import os
from typing import Mapping, Sequence

import numpy as np

from .detect.detect import detectData
from .hs2 import HSDetection
from .recording import RealArray


def DetectFromRaw(det: HSDetection, t_inc: int = 50000) -> Sequence[Mapping[str, RealArray]]:
    detectData(
        probe=det,
        file_name=det.out_file_name[:-4],
        to_localize=det.to_localize,
        fps=int(det.fps),
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
        num_frames=det.num_frames[0],
        t_inc=t_inc,
    )

    if os.stat(det.out_file_name).st_size == 0:
        shapecache = np.empty((0, 5 + det.cutout_length), dtype=np.int32)
    else:
        sp_flat = np.memmap(det.out_file_name, dtype=np.int32, mode='r')
        shapecache = sp_flat.reshape((-1, 5 + det.cutout_length))

    return [{'channel_ind': shapecache[:, 0],
             'sample_ind': shapecache[:, 1],
             'amplitude': shapecache[:, 2],
             'location': shapecache[:, 3:5] / 1000,
             'spike_shape': shapecache[:, 5:]}]
