import filecmp
import shutil
import sys

import spikeinterface.sorters as ss
from spikeinterface.extractors import MEArecRecordingExtractor

from data_utils import download_small, str2Path
from run_hs2 import run_hs2


def test_corectness(data_fn: str = 'mearec_test_10s.h5') -> None:
    data_path = str2Path(data_fn)
    if not data_path.exists():
        download_small(data_fn)

    recording = MEArecRecordingExtractor(data_path)
    # TODO: test contents of metadata

    sihs_path = str2Path('results_HS')
    hs2det_path = str2Path('result_HS2')

    stdout, stderr = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = open('/dev/null', 'w')
    try:
        ss.run_herdingspikes(recording, output_folder=sihs_path,
                             remove_existing_folder=False, with_output=False)
        run_hs2(recording, output_folder=hs2det_path)
    except Exception as e:
        sys.stdout, sys.stderr = stdout, stderr
        raise e
    else:
        sys.stdout, sys.stderr = stdout, stderr

    assert filecmp.cmp(str(sihs_path / 'HS2_detected.bin'),
                       str(hs2det_path / 'HS2_detected.bin'))

    shutil.rmtree(str(sihs_path))
    shutil.rmtree(str(hs2det_path))


if __name__ == '__main__':
    test_corectness()
