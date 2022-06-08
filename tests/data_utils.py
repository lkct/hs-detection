from math import ceil
from pathlib import Path
from typing import Union

import requests
import tqdm
from spikeinterface.extractors import (MdaRecordingExtractor,
                                       NwbRecordingExtractor)

data_cache = Path(__file__).parent / 'data'


def str2Path(path_like: Union[str, Path]) -> Path:
    if isinstance(path_like, str):
        return data_cache / path_like
    else:
        return path_like


def _convert(nwb_file: Union[str, Path], mda_folder: Union[str, Path]) -> None:
    nwb_file = str2Path(nwb_file)
    assert nwb_file.is_file()
    mda_folder = str2Path(mda_folder)
    mda_folder.mkdir(parents=True, exist_ok=True)
    assert mda_folder.is_dir()

    recording = NwbRecordingExtractor(nwb_file)
    MdaRecordingExtractor.write_recording(
        recording, mda_folder, chunk_size=4096, n_jobs=4, progress_bar=True)


def _download(url: str, save_path: Path, chunk_size: int = 2**20) -> None:
    """By default streaming in 1MB chunk. Handle redirects automatically.
    """
    r = requests.get(url, stream=True)
    chunks = ceil(int(r.headers['Content-Length']) / chunk_size)
    chunk_iter = tqdm.tqdm(r.iter_content(chunk_size=chunk_size),
                           desc=save_path.name, total=chunks)
    with save_path.open('wb') as f:
        for chunk in chunk_iter:
            f.write(chunk)


def download_small(save_fn: str = 'mearec_test_10s.h5') -> None:
    save_path = str2Path(save_fn)
    url = 'https://gin.g-node.org/NeuralEnsemble/ephy_testing_data/raw/master/mearec/mearec_test_10s.h5'

    if not save_path.exists():
        print('Downloading data')
        _download(url, save_path)


def download_large(orig_fn: str = 'sub-MEAREC-250neuron-Neuropixels_ecephys.nwb',
                   cvrt_fn: str = 'sub-MEAREC-250neuron-Neuropixels_ecephys.mda'
                   ) -> None:
    orig_path = str2Path(orig_fn)
    cvrt_path = str2Path(cvrt_fn)
    url = 'https://api.dandiarchive.org/api/assets/6d94dcf4-0b38-4323-8250-04fdc7039a66/download/'

    if not (cvrt_path / 'raw.mda').exists():
        if not orig_path.exists():
            print('Downloading data')
            _download(url, orig_path)

        print('Converting data')
        _convert(orig_path, cvrt_path)


if __name__ == '__main__':
    download_small()
    download_large()
