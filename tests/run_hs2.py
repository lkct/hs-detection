from pathlib import Path
from typing import Mapping, Sequence, Union

import numpy as np
import spikeinterface.sorters as ss
import spikeinterface.toolkit as st
from hs_detection import HSDetection
from hs_detection.entry import DetectFromRaw
from hs_detection.recording import RealArray, Recording

default_kwargs = {
    # core params
    'left_cutout_time': 0.3,
    'right_cutout_time': 1.8,
    'detect_threshold': 20,

    # extra probe params
    'probe_inner_radius': 70,
    'probe_neighbor_radius': 90,
    'probe_event_length': 0.26,
    'probe_peak_jitter': 0.2,

    # extra detection params
    't_inc': 100000,
    'num_com_centers': 1,
    'maa': 12,
    'ahpthr': 11,
    'out_file_name': 'HS2_detected',
    'decay_filtering': False,
    'save_all': False,
    'amp_evaluation_time': 0.4,
    'spk_evaluation_time': 1.0,

    # bandpass filter
    'freq_min': 300.0,
    'freq_max': 6000.0,
    'filter': True,

    # rescale traces
    'pre_scale': True,
    'pre_scale_value': 20.0,
}


def run_hsdet(recording: Recording,
              output_folder: Union[str, Path] = 'result_HS',
              **kwargs
              ) -> Sequence[Mapping[str, RealArray]]:
    params = default_kwargs.copy()
    params.update(kwargs)

    if params['filter'] and params['freq_min'] is not None and params['freq_max'] is not None:
        recording = st.bandpass_filter(
            recording, freq_min=params['freq_min'], freq_max=params['freq_max'])

    if params['pre_scale']:
        recording = st.normalize_by_quantile(
            recording, scale=params['pre_scale_value'], median=0.0, q1=0.05, q2=0.95)

    det = HSDetection(
        recording,
        inner_radius=params['probe_inner_radius'],
        neighbor_radius=params['probe_neighbor_radius'],
        event_length=params['probe_event_length'],
        peak_jitter=params['probe_peak_jitter'],
        left_cutout_time=params['left_cutout_time'],
        right_cutout_time=params['right_cutout_time'],
        threshold=params['detect_threshold'],
        to_localize=True,
        num_com_centers=params['num_com_centers'],
        maa=params['maa'],
        ahpthr=params['ahpthr'],
        out_file=Path(output_folder) / params['out_file_name'],
        decay_filtering=params['decay_filtering'],
        save_all=params['save_all'],
        amp_evaluation_time=params['amp_evaluation_time'],
        spk_evaluation_time=params['spk_evaluation_time']
    )

    return DetectFromRaw(det, t_inc=int(params['t_inc']))


def run_herdingspikes(recording: Recording,
                      output_folder: Union[str, Path] = 'results_HS',
                      **kwargs
                      ) -> Sequence[Mapping[str, RealArray]]:
    params = default_kwargs.copy()
    params.update(kwargs)

    out_file = Path(output_folder) / str(params['out_file_name'])
    out_file = out_file.with_suffix('.bin')

    fps = recording.get_sampling_frequency()
    cutout_start = int(params['left_cutout_time'] * fps / 1000 + 0.5)
    cutout_end = int(params['right_cutout_time'] * fps / 1000 + 0.5)
    cutout_length = cutout_start + cutout_end + 1

    ss.run_herdingspikes(recording, output_folder=output_folder,
                         remove_existing_folder=False, with_output=False, **kwargs)

    if out_file.stat().st_size == 0:
        spikes = np.empty((0, 5 + cutout_length), dtype=np.int32)
    else:
        spikes = np.memmap(str(out_file), dtype=np.int32, mode='r'
                           ).reshape(-1, 5 + cutout_length)

    return [{'channel_ind': spikes[:, 0],
             'sample_ind': spikes[:, 1],
             'amplitude': spikes[:, 2],
             'location': spikes[:, 3:5] / 1000,
             'spike_shape': spikes[:, 5:]}]
