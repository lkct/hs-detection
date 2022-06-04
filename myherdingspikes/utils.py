from typing import Any, Iterable, Literal, Optional

import numpy as np
from numpy.typing import NDArray

from .recording import Recording


def get_random_data_chunks(recording: Recording,
                           num_chunks_per_segment: int = 20,
                           chunk_size: int = 10000,
                           seed: int = 0
                           ) -> NDArray[np.float32]:
    """
    Exctract random chunks across segments

    Parameters
    ----------
    recording: Recording
        The recording to get random chunks from
    num_chunks_per_segment: int
        Number of chunks per segment
    chunk_size: int
        Size of a chunk in number of frames
    seed: int
        Random seed

    Returns
    -------
    chunk_list: np.ndarray
        Array of concatenate chunks per segment
    """
    # TODO: if segment have differents length make another sampling that dependant on the lneght of the segment
    # Should be done by chnaging kwargs with total_num_chunks=XXX and total_duration=YYYY
    # And randomize the number of chunk per segment wieighted by segment duration

    chunk_list: list[NDArray[np.number]] = []
    for segment_index in range(recording.get_num_segments()):
        length = recording.get_num_samples(segment_index)
        random_starts = np.random.RandomState(seed).randint(  # TODO:??? new generator
            0, length - chunk_size, size=num_chunks_per_segment)
        for start_frame in random_starts:
            chunk = recording.get_traces(segment_index=segment_index,
                                         start_frame=start_frame,
                                         end_frame=start_frame + chunk_size)
            chunk_list.append(chunk)
    return np.concatenate(chunk_list, axis=0, dtype=np.float32)


def get_scaling_param(recording: Recording,
                      scale: float = 20.0,
                      offset: float = 0.0,
                      quantile: float = 0.05
                      ) -> tuple[NDArray[np.float32], NDArray[np.float32]]:
    random_data = get_random_data_chunks(recording)

    l, m, r = np.quantile(
        random_data, q=[quantile, 0.5, 1 - quantile], axis=0, keepdims=True)
    l: NDArray[np.float32] = l.astype(np.float32)
    m: NDArray[np.float32] = m.astype(np.float32)
    r: NDArray[np.float32] = r.astype(np.float32)

    scale_param: NDArray[np.float32] = scale / (r - l)
    offset_param: NDArray[np.float32] = offset - m * scale_param

    return scale_param, offset_param
