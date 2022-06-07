import sys

import line_profiler
from myherdingspikes import HSDetection
from myherdingspikes.detection_localisation import utils
from spikeinterface.extractors import MdaRecordingExtractor

from data_utils import download_large, str2Path
from run_hs2 import run_hs


def test_linetrace(data_fn: str = 'sub-MEAREC-250neuron-Neuropixels_ecephys.mda') -> None:
    data_path = str2Path(data_fn)
    if not (data_path / 'raw.mda').exists():
        download_large(data_fn)

    recording = MdaRecordingExtractor(data_path)
    assert recording.get_sampling_frequency() == 32000 and \
        recording.get_num_samples() == 19200000
    recording = recording.frame_slice(start_frame=0, end_frame=192000)

    hsdet_path = str2Path('result_HS')
    hsdet_path.mkdir(parents=True, exist_ok=True)
    if str((hsdet_path / 'HS2_detected.bin').resolve()) != '/dev/null':
        (hsdet_path / 'HS2_detected.bin').unlink(missing_ok=True)
        (hsdet_path / 'HS2_detected.bin').symlink_to('/dev/null')

    prof_path = str2Path('prof/hs')

    prof = line_profiler.LineProfiler()
    prof(run_hs)
    prof(HSDetection.__init__)
    prof(HSDetection.get_traces)
    prof(HSDetection.detect)
    prof(utils.get_random_data_chunks)
    prof(utils.get_scaling_param)

    stdout, stderr = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = open('/dev/null', 'w')

    try:
        prof.runcall(run_hs,
                     recording, filter=False, output_folder=hsdet_path)
    except Exception as e:
        sys.stdout, sys.stderr = stdout, stderr
        print(e)
    else:
        sys.stdout, sys.stderr = stdout, stderr
    finally:
        prof.dump_stats(str(prof_path.with_suffix('.lprof')))

    with prof_path.with_suffix('.lprof.txt').open('w') as f:
        prof.print_stats(f, output_unit=1.0)


if __name__ == '__main__':
    test_linetrace()
