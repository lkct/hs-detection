import sys
import timeit

import spikeinterface.sorters as ss
from spikeinterface.extractors import MdaRecordingExtractor
from spikeinterface.sortingcomponents.peak_detection import detect_peaks

from data_utils import download_large, str2Path
from run_hs2 import run_hs2


def test_performance(data_fn: str = 'sub-MEAREC-250neuron-Neuropixels_ecephys.mda') -> None:
    data_path = str2Path(data_fn)
    if not (data_path / 'raw.mda').exists():
        download_large(data_fn)

    recording = MdaRecordingExtractor(data_path)
    assert recording.get_sampling_frequency() == 32000 and \
        recording.get_num_samples() == 19200000
    recording = recording.frame_slice(start_frame=0, end_frame=192000)

    def timeit1(stmt): return timeit.timeit(stmt, number=1)

    sihs_path = str2Path('results_HS')
    hs2det_path = str2Path('result_HS2')

    sihs_path.mkdir(parents=True, exist_ok=True)
    if str((sihs_path / 'HS2_detected.bin').resolve()) != '/dev/null':
        (sihs_path / 'HS2_detected.bin').unlink(missing_ok=True)
        (sihs_path / 'HS2_detected.bin').symlink_to('/dev/null')
    hs2det_path.mkdir(parents=True, exist_ok=True)
    if str((hs2det_path / 'HS2_detected.bin').resolve()) != '/dev/null':
        (hs2det_path / 'HS2_detected.bin').unlink(missing_ok=True)
        (hs2det_path / 'HS2_detected.bin').symlink_to('/dev/null')

    stdout, stderr = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = open('/dev/null', 'w')
    try:
        t_sihs = timeit1(lambda: ss.run_herdingspikes(
            recording, filter=False, output_folder=sihs_path, remove_existing_folder=False, with_output=False))
        t_hs2det = timeit1(lambda: run_hs2(
            recording, filter=False, output_folder=hs2det_path))

        t_peak = timeit1(lambda: detect_peaks(
            recording, method='by_channel',
            peak_sign='neg', detect_threshold=5, n_shifts=2,
            chunk_size=10000, verbose=1, progress_bar=False))
        t_peakloc = timeit1(lambda: detect_peaks(
            recording, method='locally_exclusive',
            peak_sign='neg', detect_threshold=5, n_shifts=2,
            chunk_size=10000, verbose=1, progress_bar=False,
            localization_dict=dict(method='center_of_mass', local_radius_um=150,
                                   ms_before=0.1, ms_after=0.3)))
    except Exception as e:
        sys.stdout, sys.stderr = stdout, stderr
        raise e
    else:
        sys.stdout, sys.stderr = stdout, stderr

    print(t_sihs)
    print(t_hs2det)
    print(t_peak)
    print(t_peakloc)


if __name__ == '__main__':
    test_performance()
