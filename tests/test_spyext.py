import os
import sys

from spikeinterface.extractors import MdaRecordingExtractor

from data_utils import download_large, str2Path
from run_hs2 import run_hsdet


def test_spyext(data_fn: str = 'sub-MEAREC-250neuron-Neuropixels_ecephys.mda') -> None:
    data_path = str2Path(data_fn)
    if not (data_path / 'raw.mda').exists():
        download_large(data_fn)

    recording = MdaRecordingExtractor(data_path)
    assert recording.get_sampling_frequency() == 32000 and \
        recording.get_num_samples() == 19200000
    recording = recording.frame_slice(start_frame=0, end_frame=192000 * 1)

    hsdet_path = str2Path('result_HS')
    hsdet_path.mkdir(parents=True, exist_ok=True)
    if str((hsdet_path / 'HS2_detected-0.bin').resolve()) != '/dev/null':
        (hsdet_path / 'HS2_detected-0.bin').unlink(missing_ok=True)
        (hsdet_path / 'HS2_detected-0.bin').symlink_to('/dev/null')

    stdout, stderr = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = open('/dev/null', 'w')
    try:
        run_hsdet(recording, filter=False, output_folder=hsdet_path)
    except Exception as e:
        sys.stdout, sys.stderr = stdout, stderr
        raise e
    else:
        sys.stdout, sys.stderr = stdout, stderr


if __name__ == '__main__':
    if len(sys.argv) == 1:
        os.system(f'py-spy record -o {str2Path("prof/hs.svg")} -r 10 -n '
                  f'-- python {__file__} run')
    else:
        test_spyext()
