# distutils: language=c++
# cython: language_level=3

import numpy as np
from datetime import datetime
import pprint
import cython
from ..hs2 import HS2Detection


def detectData(probe: HS2Detection,
               file_name: bytes,
               to_localize: bool,
               sf: int,
               thres: int,
               cutout_start: int,
               cutout_end: int,
               maa: int,
               maxsl: int,
               minsl: int,
               ahpthr: int,
               num_com_centers: int,
               decay_filtering: bool,
               verbose: bool,
               num_frames: int,
               t_inc: int
               ) -> None:

    # READ PROBE PARAMETERS
    sf = int(sf)  # ensure sampling rate is integer, assumed to be in Hertz
    num_channels = int(probe.num_channels)
    spike_peak_duration = int(probe.spike_peak_duration)
    noise_duration = int(probe.noise_duration)
    noise_amp_percent = float(probe.noise_amp_percent)
    max_neighbors = int(probe.max_neighbors)
    inner_radius = float(probe.inner_radius)

    masked_channel_list = probe.masked_channels
    masked_channels: cython.int[:] = np.ones(num_channels, dtype=np.intc)

    if masked_channel_list == []:
        print('# Not Masking any Channels')
    else:
        print(f'# Masking Channels: {masked_channel_list}')

    for channel in masked_channel_list:
        masked_channels[channel] = 0

    print(f'# Sampling rate: {sf}')
    if to_localize:
        print('# Localization On')
    else:
        print('# Localization Off')
    if verbose:
        print('# Writing out extended detection info')
    print(f'# Number of recorded channels: {num_channels}')
    if num_channels < 20:
        print('# Few recording channels: not subtracing mean from activity')
    print(f'# Analysing frames: {num_frames}; Seconds: {num_frames / sf}')
    print(f'# Frames before spike in cutout: {cutout_start}')
    print(f'# Frames after spike in cutout: {cutout_end}')

    det: cython.pointer(Detection) = new Detection()

    # set tCut, tCut2 and tInc
    t_cut_l = cutout_start + maxsl 
    t_cnt_r = cutout_end + maxsl 
    print(f'# tcuts: {t_cut_l} {t_cnt_r}')

    # cap at specified number of frames
    t_inc = min(num_frames - t_cut_l - t_cnt_r, t_inc)
    print(f'# tInc: {t_inc}')
    # ! To be consistent, X and Y have to be swappped
    ch_indices: cython.long[:] = np.arange(num_channels, dtype=np.int_)
    vm: cython.short[:] = np.zeros(
        num_channels * (t_inc + t_cut_l + t_cnt_r), dtype=np.short)

    # initialise detection algorithm
    det.InitDetection(num_frames, sf, num_channels, t_inc, cython.address(ch_indices[0]), 0)

    position_matrix: cython.int[:, :] = np.ascontiguousarray(probe.positions, dtype=np.intc)
    neighbor_matrix: cython.int[:, :] = np.ascontiguousarray(probe.neighbors, dtype=np.intc)

    det.SetInitialParams(cython.address(position_matrix[0, 0]), cython.address(neighbor_matrix[0, 0]), num_channels,
                         spike_peak_duration, file_name, noise_duration,
                         noise_amp_percent, inner_radius, cython.address(
                             masked_channels[0]),
                         max_neighbors, num_com_centers, to_localize,
                         thres, cutout_start, cutout_end, maa, ahpthr, maxsl,
                         minsl, decay_filtering, verbose)

    startTime = datetime.now()
    t0 = 0
    max_frames_processed = t_inc
    while t0 + t_inc + t_cnt_r <= num_frames:
        t1 = t0 + t_inc
        if verbose:
            print(f'# Analysing frames from {t0 - t_cut_l} to {t1 + t_cnt_r} '
                  f' ({100 * t0 / num_frames:.1f}%)')

        vm = probe.get_traces(t0 - t_cut_l, t1 + t_cnt_r)

        # detect spikes
        if num_channels >= 20:
            det.MeanVoltage(cython.address(vm[0]), t_inc, t_cut_l)
        det.Iterate(cython.address(vm[0]), t0,
                    t_inc, t_cut_l, t_cnt_r, max_frames_processed)

        t0 += t_inc
        if t0 < num_frames - t_cnt_r:
            t_inc = min(t_inc, num_frames - t_cnt_r - t0)

    now = datetime.now()
    # Save state of detection
    detection_state_dict = {
        'Probe Name': probe.__class__.__name__,
        'Probe Object': probe,
        'Date and Time Detection': str(now),
        'Threshold': thres,
        'Localization': to_localize,
        'Masked Channels': masked_channel_list,
        'Associated Results File': file_name,
        'Cutout Length': cutout_start + cutout_end,
        'Advice': 'For more information about detection, load and look at the parameters of the probe object',
    }

    with open(f'{file_name.decode()}DetectionDict{now.strftime("%Y-%m-%d_%H%M%S_%f")}.txt', 'a') as f:
        f.write(pprint.pformat(detection_state_dict))

    det.FinishDetection()

    t = datetime.now() - startTime
    print(f'# Detection completed, time taken: {t}')
    print(f'# Time per frame: {1000 * t / num_frames}')
    print(f'# Time per sample: {1000 * t / (num_channels * num_frames)}')
