# _pgencode

## How to use it

```python
$ python3
Python 3.6.9 (default, Jul 17 2020, 12:50:27) 
[GCC 8.4.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> from _pgencode import pgencode
>>> pgencode(['hello', 2, None, 'world', 7.3])
b'hello\t2\t\\N\tworld\t7.3'
```

## Build and install

```
$ python setup.py install
```
