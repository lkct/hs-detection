import warnings
from pathlib import Path
from typing import Mapping, Sequence, Union

import numpy as np
import spikeinterface.sorters as ss
import spikeinterface.toolkit as st
from hs_detection import HSDetection
from hs_detection.recording import RealArray, Recording
from spikeinterface import BaseRecording

default_kwargs = {
    # core params
    'left_cutout_time': 0.3,
    'right_cutout_time': 1.8,
    'threshold': 20,

    # extra probe params
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

    # bandpass filter
    'freq_min': 300.0,
    'freq_max': 6000.0,
    'filter': True,

    # rescale traces
    'rescale': True,
    'rescale_value': 20.0,

    # added switches
    'localize': True,
    'save_shape': True,
    'verbose': True
}


deprecation = {
    'detect_threshold': 'threshold',
    'probe_inner_radius': 'inner_radius',
    'probe_neighbor_radius': 'neighbor_radius',
    'probe_event_length': 'event_length',
    'probe_peak_jitter': 'peak_jitter',
    't_inc': 'chunk_size',
    'out_file_name': 'out_file',
    'pre_scale': 'rescale',
    'pre_scale_value': 'rescale_value',
    # following not supported anymore
    'probe_masked_channels': 'None',
    'save_all': 'None'
}


def run_hsdet(recording: Recording,
              output_folder: Union[str, Path] = 'result_HS',
              **kwargs
              ) -> Sequence[Mapping[str, RealArray]]:
    params = default_kwargs.copy()
    for k, v in kwargs.items():
        if k in deprecation:
            warnings.warn(
                f'HSDetection params: {k} deprecated, use {deprecation[k]} instead.')
            params[deprecation[k]] = v
        else:
            params[k] = v
    params['out_file'] = Path(output_folder) / params['out_file']

    if params['filter'] and params['freq_min'] is not None and params['freq_max'] is not None:
        recording = st.bandpass_filter(
            recording, freq_min=params['freq_min'], freq_max=params['freq_max'])

    det = HSDetection(recording, params)

    return det.detect()


def run_herdingspikes(recording: BaseRecording,
                      output_folder: Union[str, Path] = 'results_HS',
                      **kwargs
                      ) -> Sequence[Mapping[str, RealArray]]:
    params = default_kwargs.copy()
    params.update(kwargs)
    if 'out_file_name' in params:
        params['out_file'] = params['out_file_name']
        del kwargs['out_file_name']  # must be in if in params

    out_file = Path(output_folder) / str(params['out_file'])
    out_file = out_file.with_suffix('.bin')

    fps = recording.get_sampling_frequency()
    cutout_start = int(params['left_cutout_time'] * fps / 1000 + 0.5)
    cutout_end = int(params['right_cutout_time'] * fps / 1000 + 0.5)
    cutout_length = cutout_start + cutout_end + 1

    segments = recording._recording_segments
    result = []

    for seg in range(len(segments)):
        recording._recording_segments = [segments[seg]]
        out_file_seg = out_file.with_stem(out_file.stem + f'-{seg}')

        ss.run_herdingspikes(recording, output_folder=output_folder,
                             out_file_name=str(
                                 out_file_seg.relative_to(output_folder)),
                             remove_existing_folder=False, with_output=False, **kwargs)

        if out_file_seg.stat().st_size == 0:
            spikes = np.empty((0, 5 + cutout_length), dtype=np.int32)
        else:
            spikes = np.memmap(str(out_file_seg), dtype=np.int32, mode='r'
                               ).reshape(-1, 5 + cutout_length)

        result.append({'channel_ind': spikes[:, 0],
                       'sample_ind': spikes[:, 1],
                       'amplitude': spikes[:, 2],
                       'location': spikes[:, 3:5] / 1000,
                       'spike_shape': spikes[:, 5:]})

    recording._recording_segments = segments

    return result
