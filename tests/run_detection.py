from pathlib import Path
from typing import Any, Mapping, Sequence, Union

import numpy as np
from hs_detection import HSDetection
from hs_detection.recording import RealArray, Recording
from spikeinterface import BaseRecording
from spikeinterface.preprocessing import bandpass_filter
from spikeinterface.sorters import run_herdingspikes
from spikeinterface.sortingcomponents.hs_detection import run_hs_detection
from spikeinterface.sortingcomponents.peak_detection import detect_peaks


def run_hsdet(recording: Recording, *,
              output_folder: Union[str, Path] = 'result_HS',
              **kwargs
              ) -> Sequence[Mapping[str, RealArray]]:
    return run_hs_detection(recording, output_folder=output_folder, **kwargs)


def run_hs2(recording: BaseRecording, *,
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
        recording = bandpass_filter(
            recording, freq_min=params['freq_min'], freq_max=params['freq_max'], margin_ms=100)
        kwargs['filter'] = False

    segments = recording._recording_segments
    result = []

    for seg in range(len(segments)):
        recording._recording_segments = [segments[seg]]
        out_file_seg = out_file.with_stem(out_file.stem + f'-{seg}')

        run_herdingspikes(recording, output_folder=output_folder,
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


def run_peakdet(recording: BaseRecording,
                **kwargs
                ) -> Any:
    # version 15b419cb of SI.sortingcomponents (12 Aug 2022)
    default = dict(method='locally_exclusive',
                   peak_sign='neg', detect_threshold=5, exclude_sweep_ms=0.1,
                   chunk_size=100000, verbose=1, progress_bar=False)

    return detect_peaks(recording, **default | kwargs)
