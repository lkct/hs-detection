from setuptools import Extension, find_packages, setup
import os
import sys
import platform
import subprocess
import numpy


def get_version():
    # ref https://packaging.python.org/guides/single-sourcing-package-version/ 3
    version = {}
    with open('myherdingspikes/version.py', 'r') as f:
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
            with open('myherdingspikes/.commit_version', 'w') as f:
                f.write(commit)
        return version['version']
    else:
        # in install, include commit hash in version if possible
        commit = '+git.' + commit[:8] if commit else ''
        return version['version'] + commit


with open('README.md', 'r', encoding='utf-8') as f:
    long_description = f.read()


try:
    from Cython.Build import cythonize
    USE_CYTHON = True
    print('Using Cython')
except ImportError:
    def cythonize(x): return [x]
    USE_CYTHON = False
    print('Not using Cython')

ext_folder = 'myherdingspikes/detection_localisation/'
sources = ['SpkDonline.cpp',
           'SpikeHandler.cpp',
           'ProcessSpikes.cpp',
           'FilterSpikes.cpp',
           'LocalizeSpikes.cpp']
# if Cython available, compile from pyx -> cpp
# otherwise, compile from cpp
if USE_CYTHON:
    sources.append('detect.pyx')
else:
    sources.append('detect.cpp')
sources = [ext_folder + fn for fn in sources]

extra_compile_args = ['-std=c++11', '-O3']
link_extra_args = []
# OS X support
if platform.system() == 'Darwin':
    extra_compile_args += ['-mmacosx-version-min=10.9', '-F.']
    link_extra_args = ['-stdlib=libc++', '-mmacosx-version-min=10.9']

# compile with/without Cython
detect_ext = cythonize(
    Extension(name='myherdingspikes.detection_localisation.detect',
              sources=sources,
              language='c++',
              extra_compile_args=extra_compile_args,
              extra_link_args=link_extra_args,
              include_dirs=[numpy.get_include(), ext_folder]))


setup(
    name='myherdingspikes',  # TODO:???
    version=get_version(),
    description='Spike detection from HS2, used for integration in SpikeInterface',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://github.com/lkct/hs2-detection',
    author='Rickey K. Liang @ Matthias Hennig Lab, University of Edinburgh',
    author_email='liangkct@yahoo.com,m.hennig@ed.ac.uk',  # TODO:???
    license='GPLv3',
    classifiers=[
        'Development Status :: 4 - Beta',
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Operating System :: OS Independent',
        'Programming Language :: Cython',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3 :: Only',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
    ],
    keywords='spikes sorting electrophysiology detection',
    packages=find_packages(exclude=['tests']),  # TODO:???
    python_requires='>=3.6',  # TODO:???
    install_requires=[
        'numpy >= 1.14',
        'scipy',
        'matplotlib >= 2.0',
        'pandas'
    ],
    package_data={
        'myherdingspikes': [
            '.commit_version',
            '../README.md',
            'detection_localisation/*'
        ]
    },
    ext_modules=detect_ext,
    zip_safe=False,
    project_urls={
        'Source': 'https://github.com/lkct/hs2-detection'
    },
)

try:
    subprocess.check_output(['git', 'rev-parse', 'HEAD'],
                            cwd=os.path.dirname(__file__)
                            )
    # if git success: in git repo, remove file
    os.remove('myherdingspikes/.commit_version')
    # if not exist: still captured by try...except
except:
    # else: keep file
    pass
