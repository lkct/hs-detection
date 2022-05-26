from setuptools import Extension, find_packages, setup
import os
import sys
import platform
import numpy


def get_version():
    version = {}
    version['__file__'] = __file__

    # Because __init__.py in myherdingspikes loads packages we may not have installed yet, we need to parse version.py directly
    # (see https://packaging.python.org/guides/single-sourcing-package-version/)
    with open('myherdingspikes/version.py', 'r') as f:
        exec(f.read(), version)

    # Store the commit hash for when we don't have access to git
    with open("myherdingspikes/.commit_version", 'w') as f:
        f.write(version['__commit__'])

    # Public versions should not contain local identifiers (inspired from mypy)
    if any(cmd in sys.argv[1:] for cmd in ('install', 'bdist_wheel', 'sdist')):
        return version['base_version']
    else:
        return version['__version__']


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
sources = ["SpkDonline.cpp",
           "SpikeHandler.cpp",
           "ProcessSpikes.cpp",
           "FilterSpikes.cpp",
           "LocalizeSpikes.cpp"]
# if Cython available, compile from pyx -> cpp
# otherwise, compile from cpp
if USE_CYTHON:
    sources.append("detect.pyx")
else:
    sources.append("detect.cpp")
sources = [ext_folder + fn for fn in sources]

extra_compile_args = ['-std=c++11', '-O3']
link_extra_args = []
# OS X support
if platform.system() == 'Darwin':
    extra_compile_args += ['-mmacosx-version-min=10.9', '-F.']
    link_extra_args = ["-stdlib=libc++", "-mmacosx-version-min=10.9"]

# compile with/without Cython
detect_ext = cythonize(
    Extension(name="myherdingspikes.detection_localisation.detect",
              sources=sources,
              language="c++",
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


os.remove("myherdingspikes/.commit_version")
