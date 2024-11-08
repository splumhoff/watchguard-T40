#!/usr/bin/env python
from distutils.core import setup, Extension
import sys

_libs = []
if sys.platform != 'darwin':
  _libs.append('rt')
_mod = Extension('_monotime', sources=['_monotime.c'], libraries=_libs)

setup(
    name='Monotime',
    version='1.0',
    description='time.monotonic() for older python versions',
    author='Avery Pennarun',
    author_email='apenwarr@google.com',
    url='http://code.google.com/p/py-monotime/',
    packages=['monotime'],
    ext_modules=[_mod],
    package_dir={'monotime': ''},
)
