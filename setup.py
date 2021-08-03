from setuptools import setup, Extension
import setuptools
import os
import sys

def get_pybind_include():
    import pybind11
    return pybind11.get_include()

src_path = os.path.abspath('./src')

if 'ZEBRA_SCANNER_SRC_PATH' not in os.environ:
    os.environ['ZEBRA_SCANNER_SRC_PATH'] = src_path
else:
    src_path = os.environ['ZEBRA_SCANNER_SRC_PATH']

with open("README.rst", "r") as fh:
    long_description = fh.read()

source_files = [
    os.path.join(src_path, 'BoostPythonCoreScanner.cpp')
]

zebra_scanner_module = Extension("zebra_scanner",
    include_dirs=[
        '/usr/include/zebra-scanner',
        get_pybind_include(),
        src_path
    ],
    library_dirs=['/usr/lib/zebra-scanner/corescanner'],
    libraries=['cs-client', 'cs-common', 'pugixml'],
    sources=source_files,
    extra_compile_args=['-Wno-deprecated', '-std=c++11', '-fvisibility=hidden']
)

setup(
    name="zebra-scanner",
    version="v0.2.5",
    author="David Jablonski",
    author_email="dayjaby@gmail.com",
    description="Scan barcodes with a zebra barcode scanner",
    long_description=long_description,
    long_description_content_type="text/x-rst",
    url="https://github.com/dayjaby/zebra-scanner",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    ext_modules=[zebra_scanner_module]
)
