from pathlib import Path
from typing import TypedDict, Union


class Params(TypedDict):
    chunk_size: int
    rescale: bool
    rescale_value: float
    common_reference: str
    spk_evaluation_time: float
    amp_evaluation_time: float
    threshold: float
    maa: float
    ahpthr: float
    neighbor_radius: float
    inner_radius: float
    peak_jitter: float
    event_length: float
    decay_filtering: bool
    noise_amp_percent: float
    localize: bool
    save_shape: bool
    out_file: Union[str, Path]
    left_cutout_time: float
    right_cutout_time: float
    verbose: bool
