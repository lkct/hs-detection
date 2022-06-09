from typing import Any, Iterable, Literal, Optional

import numpy as np
from numpy.typing import NDArray

from .recording import RealArray, Recording


def get_random_data_chunks(recording: Recording,
                           num_chunks_per_segment: int = 20,
                           chunk_size: int = 10000,
                           seed: int = 0
                           ) -> NDArray[np.float32]:
    # TODO: sample uniformly on samples instead of segments
    chunks: list[RealArray] = []
    for seg in range(recording.get_num_segments()):
        num_frames = recording.get_num_samples(seg)
        # TODO: new generator api
        random_starts = np.random.RandomState(seed).randint(
            0, num_frames - chunk_size, size=num_chunks_per_segment)
        for start_frame in random_starts:
            chunks.append(recording.get_traces(
                segment_index=seg, start_frame=start_frame, end_frame=start_frame + chunk_size))
    return np.concatenate(chunks, axis=0, dtype=np.float32)


class ScaleRecording(Recording):
    def __init__(self,
                 recording: Recording,
                 scale: float = 20.0,
                 offset: float = 0.0,
                 quantile: float = 0.05
                 ) -> None:
        super().__init__()
        self.recording = recording

        random_data = get_random_data_chunks(recording)

        l, m, r = np.quantile(
            random_data, q=[quantile, 0.5, 1 - quantile], axis=0, keepdims=True)
        # quantile gives float64 on float32 data
        l: NDArray[np.float32] = l.astype(np.float32)
        m: NDArray[np.float32] = m.astype(np.float32)
        r: NDArray[np.float32] = r.astype(np.float32)

        self.scale: NDArray[np.float32] = scale / (r - l)
        self.offset: NDArray[np.float32] = offset - m * self.scale

    def get_num_channels(self) -> int:
        return self.recording.get_num_channels()

    def get_channel_ids(self) -> Iterable[Any]:
        return self.recording.get_channel_ids()

    def get_channel_property(self, channel_id: Any, key: Any) -> Any:
        return self.recording.get_channel_property(channel_id, key)

    def get_sampling_frequency(self) -> float:
        return self.recording.get_sampling_frequency()

    def get_num_segments(self) -> int:
        return self.recording.get_num_segments()

    def get_num_samples(self, segment_index: Optional[int] = None) -> int:
        return self.recording.get_num_samples(segment_index)

    def get_traces(self,
                   segment_index: Optional[int] = None,
                   start_frame: Optional[int] = None,
                   end_frame: Optional[int] = None,
                   channel_ids: Optional[Iterable[Any]] = None,
                   order: Optional[Literal['C', 'F']] = None,
                   return_scaled: bool = False
                   ) -> NDArray[np.float32]:
        # TODO: currently no selection on channel_ids
        assert channel_ids is None
        trace = self.recording.get_traces(segment_index, start_frame, end_frame,
                                          channel_ids, order, return_scaled)
        return trace.astype(np.float32, copy=False) * self.scale + self.offset
