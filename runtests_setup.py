from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

ext_modules = [
    Extension("redis_locker_tests",
              ["redis_locker_tests.pyx", "redis-locker.c"],
              libraries=['hiredis']),
    ]

setup(name="hiredis-locker testsuite",
      cmdclass={'build_ext': build_ext},
      ext_modules=ext_modules)
