from distutils.core import setup, Extension


pgencode = Extension('_pgencode',
                     sources=['pgencode.c'])

setup(name='_pgencode',
      version='1.0',
      description='PostgreSQL bulk copy encode data',
      ext_modules=[pgencode])
