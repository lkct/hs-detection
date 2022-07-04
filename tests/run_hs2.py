import warnings
from pathlib import Path
from typing import Any, Mapping, Sequence, Union

import numpy as np
import spikeinterface.sorters as ss
import spikeinterface.toolkit as st
from hs_detection import HSDetection
from hs_detection.recording import RealArray, Recording
from spikeinterface import BaseRecording

deprecation = {
    'filter': 'bandpass',
    't_inc': 'chunk_size',
    'pre_scale': 'rescale',
    'pre_scale_value': 'rescale_value',
    'spk_evaluation_time': 'spike_duration',
    'amp_evaluation_time': 'amp_avg_duration',
    'detect_threshold': 'threshold',
    'maa': 'min_avg_amp',
    'ahpthr': 'AHP_thr',
    'probe_neighbor_radius': 'neighbor_radius',
    'probe_inner_radius': 'inner_radius',
    'probe_peak_jitter': 'peak_jitter',
    'probe_event_length': 'rise_duration',
    'out_file_name': 'out_file',
    # following not supported anymore
    'probe_masked_channels': 'None',
    'num_com_centers': 'None',
    'save_all': 'None'
}


def run_hsdet(recording: Recording,
              output_folder: Union[str, Path] = 'result_HS',
              **kwargs
              ) -> Sequence[Mapping[str, RealArray]]:
    params = HSDetection.DEFAULT_PARAMS.copy()
    for k, v in kwargs.items():
        if k in deprecation:
            warnings.warn(
                f'HSDetection params: "{k}" deprecated, use "{deprecation[k]}" instead.')
            params[deprecation[k]] = v
        else:
            params[k] = v
    params['out_file'] = Path(output_folder) / params['out_file']

    if params['bandpass']:
        recording = st.bandpass_filter(
            recording, freq_min=params['freq_min'], freq_max=params['freq_max'], margin_ms=100)

    det = HSDetection(recording, params)

    return det.detect()


def run_herdingspikes(recording: BaseRecording,
                      output_folder: Union[str, Path] = 'results_HS',
                      **kwargs
                      ) -> Sequence[Mapping[str, RealArray]]:
    params: dict[str, Any] = HSDetection.DEFAULT_PARAMS | kwargs
    if 'out_file_name' in params:
        params['out_file'] = params['out_file_name']
        del kwargs['out_file_name']  # must be in if in params

    out_file = Path(output_folder) / str(params['out_file'])
    out_file = out_file.with_suffix('.bin')

    fps = recording.get_sampling_frequency()
    cutout_start = int(params['left_cutout_time'] * fps / 1000 + 0.5)
    cutout_end = int(params['right_cutout_time'] * fps / 1000 + 0.5)
    cutout_length = cutout_start + cutout_end + 1

    if kwargs['filter'] if 'filter' in kwargs else params['bandpass']:
        recording = st.bandpass_filter(
            recording, freq_min=params['freq_min'], freq_max=params['freq_max'], margin_ms=100)
        kwargs['filter'] = False

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
