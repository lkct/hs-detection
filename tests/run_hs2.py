from pathlib import Path
from typing import Union

import spikeinterface.toolkit as st
from myherdingspikes import HS2Detection, RecordingExtractor
from myherdingspikes.entry import DetectFromRaw

default_kwargs = {  # TODO:???
    # core params
    'clustering_bandwidth': 5.5,
    'clustering_alpha': 5.5,
    'clustering_n_jobs': -1,
    'clustering_bin_seeding': True,
    'clustering_min_bin_freq': 16,
    'clustering_subset': None,
    'left_cutout_time': 0.3,
    'right_cutout_time': 1.8,
    'detect_threshold': 20,

    # extra probe params
    'probe_masked_channels': [],
    'probe_inner_radius': 70,
    'probe_neighbor_radius': 90,
    'probe_event_length': 0.26,
    'probe_peak_jitter': 0.2,

    # extra detection params
    't_inc': 100000,
    'num_com_centers': 1,
    'maa': 12,
    'ahpthr': 11,
    'out_file_name': "HS2_detected",
    'decay_filtering': False,
    'save_all': False,
    'amp_evaluation_time': 0.4,
    'spk_evaluation_time': 1.0,

    # extra pca params
    'pca_ncomponents': 2,
    'pca_whiten': True,

    # bandpass filter
    'freq_min': 300.0,
    'freq_max': 6000.0,
    'filter': True,

    # rescale traces
    'pre_scale': True,
    'pre_scale_value': 20.0,

    # remove duplicates (based on spk_evaluation_time)
    'filter_duplicates': True
}


# TODO:??? anno
def run_hs2(recording, output_folder: Union[str, Path] = 'result_HS2', **kwargs) -> None:
    params = default_kwargs.copy()
    params.update(kwargs)

    if params['filter'] and params['freq_min'] is not None and params['freq_max'] is not None:
        recording = st.bandpass_filter(
            recording, freq_min=params['freq_min'], freq_max=params['freq_max'])

    if params['pre_scale']:
        recording = st.normalize_by_quantile(
            recording, scale=params['pre_scale_value'], median=0.0, q1=0.05, q2=0.95)

    probe = RecordingExtractor(recording,
                               masked_channels=params['probe_masked_channels'],
                               inner_radius=params['probe_inner_radius'],
                               neighbor_radius=params['probe_neighbor_radius'],
                               event_length=params['probe_event_length'],
                               peak_jitter=params['probe_peak_jitter'])

    H = HS2Detection(
        probe,
        file_directory_name=str(output_folder),
        left_cutout_time=params['left_cutout_time'],
        right_cutout_time=params['right_cutout_time'],
        threshold=params['detect_threshold'],
        to_localize=True,
        num_com_centers=params['num_com_centers'],
        maa=params['maa'],
        ahpthr=params['ahpthr'],
        out_file_name=params['out_file_name'],
        decay_filtering=params['decay_filtering'],
        save_all=params['save_all'],
        amp_evaluation_time=params['amp_evaluation_time'],
        spk_evaluation_time=params['spk_evaluation_time']
    )

    DetectFromRaw(H, tInc=int(params['t_inc']))
