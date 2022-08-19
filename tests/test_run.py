import sys

from spikeinterface.extractors import MdaRecordingExtractor

from data_utils import str2Path
from run_detection import run_hsdet


def test_run(data_fn: str = 'sub-MEAREC-250neuron-Neuropixels_ecephys.mda', data_size: int = 1) -> None:
    recording = MdaRecordingExtractor(str2Path(data_fn))
    assert recording.get_sampling_frequency() == 32000 and \
        recording.get_num_samples() == 19200000
    # data 1 unit = 6.25s ~= 0.3GB
    data_size = 200000 * min(data_size, 19200000 // 200000)
    recording = recording.frame_slice(start_frame=0, end_frame=data_size)

    hsdet_path = str2Path('result_HS')
    hsdet_path.mkdir(parents=True, exist_ok=True)
    if str((hsdet_path / 'HS2_detected-0.bin').resolve()) != '/dev/null':
        (hsdet_path / 'HS2_detected-0.bin').unlink(missing_ok=True)
        (hsdet_path / 'HS2_detected-0.bin').symlink_to('/dev/null')

    stdout, stderr = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = open('/dev/null', 'w')
    try:
        hsdet = run_hsdet(recording, filter=False, output_folder=hsdet_path)
    except Exception as e:
        sys.stdout, sys.stderr = stdout, stderr
        raise e
    else:
        sys.stdout, sys.stderr = stdout, stderr

    print(hsdet[0]['channel_ind'].shape[0])


if __name__ == '__main__':
    if len(sys.argv) > 1:
        test_run(data_size=int(sys.argv[1]))
    else:
        test_run()
