# _pgencode

## How to use it

```python
$ python3
Python 3.6.9 (default, Apr 18 2020, 01:56:04) 
[GCC 8.4.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> from _pgencode import pgencode
>>> pgencode([1,2,3])
b'1\\t2\\t3'
>>> pgencode(['hello',2,'world'])
b'hello\\t2\\tworld'
```

## Build and install

```
$ python setup.py install
```
