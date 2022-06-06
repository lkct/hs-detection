from pathlib import Path
from typing import Mapping, Union

import numpy as np
import spikeinterface.sorters as ss
import spikeinterface.toolkit as st
from myherdingspikes import HS2Detection
from myherdingspikes.recording import Recording
from numpy.typing import NDArray

default_kwargs = {
    # core params
    'left_cutout_time': 0.3,
    'right_cutout_time': 1.8,
    'threshold': 20,

    # extra probe params
    'masked_channels': None,
    'inner_radius': 70.0,
    'neighbor_radius': 90.0,
    'event_length': 0.26,
    'peak_jitter': 0.2,
    'noise_amp_percent': 1.0,

    # extra detection params
    'chunk_size': 100000,
    'num_com_centers': 1,
    'maa': 12,
    'ahpthr': 11,
    'out_file': 'HS2_detected',
    'decay_filtering': False,
    'amp_evaluation_time': 0.4,
    'spk_evaluation_time': 1.0,
    'localize': True,
    'save_shape': True,
    'save_all': False,

    # bandpass filter
    'freq_min': 300.0,
    'freq_max': 6000.0,
    'filter': True,

    # rescale traces
    'rescale': True,
    'rescale_value': 20.0,
}


def run_hs2(recording: Recording, output_folder: Union[str, Path] = 'result_HS2', **kwargs
            ) -> Mapping[str, Union[NDArray[np.integer], NDArray[np.floating]]]:
    params = default_kwargs.copy()
    params.update(kwargs)
    params['out_file'] = Path(output_folder) / params['out_file']

    if params['filter'] and params['freq_min'] is not None and params['freq_max'] is not None:
        recording = st.bandpass_filter(
            recording, freq_min=params['freq_min'], freq_max=params['freq_max'])

    H = HS2Detection(recording, params)

    return H.detect()


def run_herdingspikes(recording: Recording, output_folder: Union[str, Path] = 'results_HS', filter: bool = True
                      ) -> Mapping[str, Union[NDArray[np.integer], NDArray[np.floating]]]:
    ss.run_herdingspikes(recording, output_folder=output_folder, filter=filter,
                         remove_existing_folder=False, with_output=False)

    fps = recording.get_sampling_frequency()
    cutout_start = int(default_kwargs['left_cutout_time'] * fps / 1000 + 0.5)
    cutout_end = int(default_kwargs['right_cutout_time'] * fps / 1000 + 0.5)
    cutout_length = cutout_start + cutout_end + 1

    out_file = Path(output_folder) / str(default_kwargs['out_file'])
    out_file = out_file.with_suffix('.bin')
    if out_file.stat().st_size == 0:
        spikes = np.empty((0, 5 + cutout_length), dtype=np.intc)
    else:
        spikes = np.memmap(str(out_file), dtype=np.intc, mode='r'
                           ).reshape(-1, 5 + cutout_length)

    return {'ch': spikes[:, 0],
            't': spikes[:, 1],
            'Amplitude': spikes[:, 2],
            'x': spikes[:, 3] / 1000,
            'y': spikes[:, 4] / 1000,
            'Shape': spikes[:, 5:]}
