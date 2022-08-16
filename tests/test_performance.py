import sys
import timeit

from spikeinterface.extractors import MdaRecordingExtractor

from data_utils import download_large, str2Path
from run_detection import run_hs2, run_hsdet, run_peakdet


def test_performance(data_fn: str = 'sub-MEAREC-250neuron-Neuropixels_ecephys.mda') -> None:
    data_path = str2Path(data_fn)
    if not (data_path / 'raw.mda').exists():
        download_large(data_fn)

    recording = MdaRecordingExtractor(data_path)
    assert recording.get_sampling_frequency() == 32000 and \
        recording.get_num_samples() == 19200000
    # data 1% = 6s ~= 0.3GB
    recording = recording.frame_slice(start_frame=0, end_frame=192000 * 1)

    sihs_path = str2Path('results_HS')
    sihs_path.mkdir(parents=True, exist_ok=True)
    if str((sihs_path / 'HS2_detected-0.bin').resolve()) != '/dev/null':
        (sihs_path / 'HS2_detected-0.bin').unlink(missing_ok=True)
        (sihs_path / 'HS2_detected-0.bin').symlink_to('/dev/null')
    hsdet_path = str2Path('result_HS')
    hsdet_path.mkdir(parents=True, exist_ok=True)
    if str((hsdet_path / 'HS2_detected-0.bin').resolve()) != '/dev/null':
        (hsdet_path / 'HS2_detected-0.bin').unlink(missing_ok=True)
        (hsdet_path / 'HS2_detected-0.bin').symlink_to('/dev/null')

    def timeit1(stmt): return timeit.timeit(stmt, number=1)

    stdout, stderr = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = open('/dev/null', 'w')
    try:
        t_sihs = timeit1(lambda: run_hs2(
            recording, filter=False, output_folder=sihs_path))
        t_hsdet = timeit1(lambda: run_hsdet(
            recording, filter=False, output_folder=hsdet_path))
        t_peak = timeit1(lambda: run_peakdet(
            recording, ))
    except Exception as e:
        sys.stdout, sys.stderr = stdout, stderr
        raise e
    else:
        sys.stdout, sys.stderr = stdout, stderr

    print(t_sihs)
    print(t_hsdet)
    print(t_peak)


if __name__ == '__main__':
    test_performance()
