from distutils.core import setup, Extension


pgencode = Extension('_pgencode',
                     sources=['pgencode.c'])

setup(name='pgencode',
      version='1.0.2',
      description='PostgreSQL bulk copy encode data',
      ext_modules=[pgencode])
