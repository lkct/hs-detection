import filecmp
import sys

import numpy as np
from spikeinterface.extractors import MEArecRecordingExtractor

from data_utils import download_small, str2Path
from run_hs2 import run_herdingspikes, run_hs2


def test_corectness(data_fn: str = 'mearec_test_10s.h5') -> None:
    data_path = str2Path(data_fn)
    if not data_path.exists():
        download_small(data_fn)

    recording = MEArecRecordingExtractor(data_path)

    assert [recording.is_writable, recording.is_filtered(),
            recording.check_if_dumpable(), recording.has_scaled_traces(),
            recording.has_3d_locations(), recording.has_time_vector(),
            recording.get_dtype(), str(recording.get_probe()),
            recording.get_num_channels(), recording.get_sampling_frequency(),
            recording.get_num_segments(), recording.get_num_samples()
            ] == [False, True,
                  True, True,
                  False, False,
                  np.float32, 'Probe - 32ch - 1shanks',
                  32, 32000.0,
                  1, 320000]

    sihs_path = str2Path('results_HS')
    hs2det_path = str2Path('result_HS2')

    if str((sihs_path / 'HS2_detected.bin').resolve()) == '/dev/null':
        (sihs_path / 'HS2_detected.bin').unlink(missing_ok=True)
    if str((hs2det_path / 'HS2_detected.bin').resolve()) == '/dev/null':
        (hs2det_path / 'HS2_detected.bin').unlink(missing_ok=True)

    stdout, stderr = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = open('/dev/null', 'w')
    try:
        run_herdingspikes(recording, output_folder=sihs_path)
        run_hs2(recording, output_folder=hs2det_path)
    except Exception as e:
        sys.stdout, sys.stderr = stdout, stderr
        raise e
    else:
        sys.stdout, sys.stderr = stdout, stderr

    assert (hs2det_path / 'HS2_detected.bin').stat().st_size > 0
    assert filecmp.cmp(str(sihs_path / 'HS2_detected.bin'),
                       str(hs2det_path / 'HS2_detected.bin'))


if __name__ == '__main__':
    test_corectness()
