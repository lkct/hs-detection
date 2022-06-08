import cProfile
import os
import pstats
import sys

from spikeinterface.extractors import MdaRecordingExtractor

from data_utils import download_large, str2Path
from run_hs2 import run_hsdet


def test_profiling(data_fn: str = 'sub-MEAREC-250neuron-Neuropixels_ecephys.mda') -> None:
    data_path = str2Path(data_fn)
    if not (data_path / 'raw.mda').exists():
        download_large(data_fn)

    recording = MdaRecordingExtractor(data_path)
    assert recording.get_sampling_frequency() == 32000 and \
        recording.get_num_samples() == 19200000
    recording = recording.frame_slice(start_frame=0, end_frame=192000 * 1)

    hsdet_path = str2Path('result_HS')
    hsdet_path.mkdir(parents=True, exist_ok=True)
    if str((hsdet_path / 'HS2_detected.bin').resolve()) != '/dev/null':
        (hsdet_path / 'HS2_detected.bin').unlink(missing_ok=True)
        (hsdet_path / 'HS2_detected.bin').symlink_to('/dev/null')

    prof_path = str2Path('prof/hs')
    prof_path.parent.mkdir(parents=True, exist_ok=True)

    prof = cProfile.Profile()

    stdout, stderr = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = open('/dev/null', 'w')
    try:
        prof.runcall(run_hsdet, recording, filter=False,
                     output_folder=hsdet_path)
    except Exception as e:
        sys.stdout, sys.stderr = stdout, stderr
        print(e)
    else:
        sys.stdout, sys.stderr = stdout, stderr
    finally:
        prof.dump_stats(str(prof_path.with_suffix('.prof')))

    with prof_path.with_suffix('.prof.txt').open('w') as f:
        sys.stdout = f
        pstats.Stats(str(prof_path.with_suffix('.prof'))
                     ).strip_dirs().sort_stats('cumtime').print_callers()
        sys.stdout = stdout

    os.system('gprof2dot --node-label self-time --node-label self-time-percentage '
              '--node-label total-time --node-label total-time-percentage '
              f'-n 0.1 -e 0.01 -f pstats {str(prof_path.with_suffix(".prof"))} | '
              f'dot -T png -o {str(prof_path.with_suffix(".png"))}')


if __name__ == '__main__':
    test_profiling()
