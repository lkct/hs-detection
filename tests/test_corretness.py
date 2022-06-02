import filecmp
import sys

import spikeinterface.sorters as ss
from spikeinterface.extractors import MEArecRecordingExtractor

from data_utils import download_small, str2Path  # TODO:??? import
from run_hs2 import run_hs2


def test_corectness(data_fn: str = 'mearec_test_10s.h5'):
    data_path = str2Path(data_fn)
    if not data_path.exists():
        download_small(data_fn)

    recording = MEArecRecordingExtractor(data_path)
    # TODO: test contents of metadata
    stdout, stderr = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = open('/dev/null', 'w')
    try:
        ss.run_herdingspikes(recording, output_folder=str2Path('results_HS'))
        run_hs2(recording, output_folder=str2Path('result_HS2'))
    except Exception as e:
        sys.stdout, sys.stderr = stdout, stderr
        raise e
    else:
        sys.stdout, sys.stderr = stdout, stderr
    assert filecmp.cmp(str(str2Path('results_HS/HS2_detected.bin')),
                       str(str2Path('result_HS2/HS2_detected.bin')))


if __name__ == '__main__':
    test_corectness()
