import glob
import os
import platform
import subprocess
import sys
from typing import Iterable

from setuptools import Extension, find_packages, setup

try:
    # at the second run for install we already have numpy installed as denpendency
    from numpy import get_include
    numpy_include = get_include()
    del get_include
except:
    # the first run for egg_info does not use numpy
    numpy_include = ''
    print('WARNING no NumPy found, not for build')

try:
    # if have cython, use augmented py
    from Cython.Build import cythonize
    ext_src = ['detect.py']
    print('Using Cython')
except ImportError:
    # no cython, use compiled cpp
    def cythonize(module_list, **kwargs):
        return list(module_list) if isinstance(module_list, Iterable) else [module_list]
    ext_src = ['detect.cpp']
    print('Not using Cython')


PROFILE = 0
NATIVE_OPTIM = True


def get_version() -> str:
    # ref https://packaging.python.org/guides/single-sourcing-package-version/
    # solution 3
    version = {}
    with open('hs_detection/version.py', 'r') as f:
        exec(f.read(), version)

    try:
        commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'],
                                         cwd=os.path.dirname(__file__)
                                         ).decode('utf8').strip()
    except:
        commit = ''

    if any(cmd in sys.argv for cmd in ('sdist', 'bdist', 'bdist_wheel')):
        # in dist, include commit hash as file but not in version
        if commit:
            with open('hs_detection/.commit_version', 'w') as f:
                f.write(commit)
        return version['version']
    else:
        # in install, include commit hash in version if possible
        commit = '+git.' + commit[:8] if commit else ''
        return version['version'] + commit


with open('README.md', 'r', encoding='utf-8') as f:
    long_description = f.read()


# all cpp should start with capital, except for cython generated
sources = glob.glob('hs_detection/detect/**/[A-Z]*.cpp', recursive=True)
sources += [os.path.join('hs_detection/detect', fn) for fn in ext_src]

extra_compile_args = ['-std=c++17', '-O3', '-fopenmp'] + \
    ['-march=native', '-mtune=native'] * NATIVE_OPTIM
link_extra_args = ['-fopenmp']
# OS X support
if platform.system() == 'Darwin':
    extra_compile_args += ['-mmacosx-version-min=10.14', '-F.']
    link_extra_args += ['-stdlib=libc++', '-mmacosx-version-min=10.14']

# compile with/without Cython
detect_ext = cythonize(
    Extension(name='hs_detection.detect.detect',
              sources=sources,
              include_dirs=[numpy_include],
              define_macros=[
                  ('CYTHON_TRACE_NOGIL', '1' if PROFILE >= 2 else '0')],
              extra_compile_args=extra_compile_args,
              extra_link_args=link_extra_args,
              language='c++'),
    compiler_directives={'language_level': '3', 'profile': PROFILE >= 1, 'linetrace': PROFILE >= 2})


setup(
    name='hs-detection',
    version=get_version(),
    description='Spike detection from HS2, used for integration in SpikeInterface',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://github.com/lkct/hs2-detection',
    author='Rickey K. Liang @ Matthias Hennig Lab, University of Edinburgh',
    author_email='liangkct@yahoo.com',
    license='GPLv3',
    classifiers=[
        'Development Status :: 4 - Beta',
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Operating System :: OS Independent',
        'Programming Language :: Cython',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3 :: Only'
    ],
    keywords='spikes sorting electrophysiology detection',
    python_requires='>=3.9',
    install_requires=[
        'numpy>=1.21,<1.22'
    ],
    extras_require={
        'tests': [
            'spikeinterface>=0.94',
            'requests',
            'tqdm',
            'gprof2dot',
            'line_profiler',
            'py-spy'
        ]
    },
    packages=find_packages(),
    package_data={
        'hs_detection': [
            '.commit_version',
            '../README.md',
            'detect/**'
        ]
    },
    exclude_package_data={
        'hs_detection': [
            'detect/*.so',
            '**/__pycache__'
        ]
    },
    ext_modules=detect_ext,
    zip_safe=False,
    project_urls={
        'Source': 'https://github.com/lkct/hs2-detection'
    }
)


try:
    subprocess.check_output(['git', 'rev-parse', 'HEAD'],
                            cwd=os.path.dirname(__file__))
    # if git success: in git repo, remove file
    os.remove('hs_detection/.commit_version')
    # if file to remove not exist: still captured by try...except
except:
    # else: keep file, or file not exist
    pass
