import sys

import numpy as np
from spikeinterface.extractors import MEArecRecordingExtractor

from data_utils import download_small, str2Path
from run_hs2 import run_herdingspikes, run_hsdet


def test_correctness(data_fn: str = 'mearec_test_10s.h5', expected: int = -1, **kwargs) -> None:
    data_path = str2Path(data_fn)
    if not data_path.exists():
        download_small(data_fn)

    recording = MEArecRecordingExtractor(data_path)
    recording.add_recording_segment(recording._recording_segments[0])

    assert [recording.is_writable, recording.is_filtered(),
            recording.check_if_dumpable(), recording.has_scaled_traces(),
            recording.has_3d_locations(), recording.has_time_vector(0),
            recording.get_dtype(), str(recording.get_probe()),
            recording.get_num_channels(), recording.get_sampling_frequency(),
            recording.get_num_segments(), recording.get_num_samples(0)
            ] == [False, True,
                  True, True,
                  False, False,
                  np.float32, 'Probe - 32ch - 1shanks',
                  32, 32000.0,
                  2, 320000]

    sihs_path = str2Path('results_HS')
    sihs_path.mkdir(parents=True, exist_ok=True)
    for seg in range(recording.get_num_segments()):
        if str((sihs_path / f'HS2_detected-{seg}.bin').resolve()) == '/dev/null':
            (sihs_path / f'HS2_detected-{seg}.bin').unlink(missing_ok=True)
    hsdet_path = str2Path('result_HS')
    hsdet_path.mkdir(parents=True, exist_ok=True)
    for seg in range(recording.get_num_segments()):
        if str((hsdet_path / f'HS2_detected-{seg}.bin').resolve()) == '/dev/null':
            (hsdet_path / f'HS2_detected-{seg}.bin').unlink(missing_ok=True)

    stdout, stderr = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = open('/dev/null', 'w')
    try:
        sihs = run_herdingspikes(recording, output_folder=sihs_path, **kwargs)
        hsdet = run_hsdet(recording, output_folder=hsdet_path, **kwargs)
    except Exception as e:
        sys.stdout, sys.stderr = stdout, stderr
        raise e
    else:
        sys.stdout, sys.stderr = stdout, stderr

    for seg in range(recording.get_num_segments()):
        print(hsdet[seg]['channel_ind'].shape[0], expected,
              sihs[seg]['channel_ind'].shape[0])
        # for k in hsdet[seg].keys():
        #     assert np.allclose(sihs[seg][k], hsdet[seg][k],
        #                        rtol=0, atol=0.6e-3), k


if __name__ == '__main__':
    test_correctness(filter=False, expected=619)
    test_correctness(expected=738)
    test_correctness(filter=False, decay_filtering=True, expected=620)
    test_correctness(decay_filtering=True, expected=739)
