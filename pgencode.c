#include "Python.h"

PyObject *module;

static char *buff;                 /* temporary buffer */
static Py_ssize_t buff_size;       /* size of allocated buffer */

static int
ensure_buff_capacity(Py_ssize_t size) {
    if (size > buff_size) {
        /* find next size to allocate */
        Py_ssize_t new_buff_size = buff_size;
        if (!new_buff_size) {
            new_buff_size = 256;
        }
        while (new_buff_size < size) {
            new_buff_size *= 2;
        }
        /* try to allocate new memory buffer */
        char *new_buff;
        if (buff == NULL) {
            new_buff = malloc(new_buff_size);
        } else {
            new_buff = realloc(buff, new_buff_size);
        }
        if (new_buff == NULL) {
            PyErr_SetString(PyExc_MemoryError, "could not allocate encoding buffer");
            return -1;
        }
        /* save new memory buffer */
        buff = new_buff;
        buff_size = new_buff_size;
    }
    return 0;
}

/**
 * # Map non printable characters.
 * \b    Backspace (ASCII 8)
 * \f    Form feed (ASCII 12)
 * \n    Newline (ASCII 10)
 * \r    Carriage return (ASCII 13)
 * \t    Tab (ASCII 9)
 * \v    Vertical tab (ASCII 11)
 * \xdigits Backslash x followed by one or two hex digits specifies the
 *       character with that numeric code
 * _pg_charmap = {chr(8): '\\b', chr(12): '\\f', chr(10): '\\n', chr(13): '\\r',
 *                chr(9): '\\t', chr(11): '\\v', '\\': '\\\\'}
 * for i in range(256):
 *   if ord(' ') <= i <= ord('~'):
 *       continue
 *   if chr(i) not in _pg_charmap:
 *       _pg_charmap[chr(i)] = '\\x%02x' % i
 */
static int
pgencode_bytes_len_measure(const char *bytes, Py_ssize_t len) {
    int size = 0;
    while (len--) {
        char c = *bytes++;
        if (' ' <= c && c <= '~') {
            size++;
        } else if (8 <= c && c <= 13) {
            size += 2;
        } else {
            size += 4;
        }
    }
    return size;
}

static void
pgencode_bytes_len(const char *bytes, int len, char *dest) {
    static char hexdigit[] = "0123456789abcdef";

    while (len--) {
        char c = *bytes++;
        if (' ' <= c && c <= '~') {
            *dest++ = c;
        } else if (8 <= c && c <= 13) {
            *dest++ = '\\';
            switch (c) {
                case 8:
                    *dest++ = 'b';
                    break;
                case 9:
                    *dest++ = 't';
                    break;
                case 10:
                    *dest++ = 'n';
                    break;
                case 11:
                    *dest++ = 'v';
                    break;
                case 12:
                    *dest++ = 'f';
                    break;
                case 13:
                    *dest++ = 'r';
                    break;
            }
        } else {
            *dest++ = '\\';
            *dest++ = 'x';
            *dest++ = hexdigit[(c & 0xf0) >> 4];
            *dest++ = hexdigit[c & 0x0f];
        }
    }
}

static int
pgencode_unicode(PyObject *obj, int offset) {
    Py_ssize_t bytes_len;
    const char *bytes = PyUnicode_AsUTF8AndSize(obj, &bytes_len);

    /* measure how much space is required for output */
    int measure = pgencode_bytes_len_measure(bytes, bytes_len);
    if (ensure_buff_capacity(offset + measure) < 0) {
        return -1;
    }
    pgencode_bytes_len(bytes, bytes_len, buff + offset);

    return offset + measure;
}

static int
pgencode_bytes(PyObject *obj, int offset) {
    char *bytes;
    Py_ssize_t bytes_len;
    
    if (PyBytes_AsStringAndSize(obj, &bytes, &bytes_len) < 0) {
        return -1;
    }

    /* measure how much space is required for output */
    int measure = pgencode_bytes_len_measure(bytes, bytes_len);
    if (ensure_buff_capacity(offset + measure) < 0) {
        return -1;
    }
    pgencode_bytes_len(bytes, bytes_len, buff + offset);

    return offset + measure;
}

static int pgencode_object(PyObject *obj, int offset, int suppress_delimiter) {
    if (offset > 0 && !suppress_delimiter) {
        // \t delimiter
        if (ensure_buff_capacity(offset + 1) < 0) {
            return -1;
        }
        buff[offset++] = '\t';
    }
    if (PyUnicode_Check(obj)) {
        offset = pgencode_unicode(obj, offset);
    } else if (PyBytes_Check(obj)) {
        offset = pgencode_bytes(obj, offset);
    } else if (PySequence_Check(obj)) {
        for (int i = 0; i < PySequence_Length(obj); i++) {
            PyObject *item = PySequence_GetItem(obj, i);
            if (item == NULL) {
                return -1;
            }
            offset = pgencode_object(item, offset, i == 0);
            if (offset < 0) {
                return -1;
            }
        }
    } else if (obj == Py_None) {
        // \\N
        if (ensure_buff_capacity(offset + 2) < 0) {
            return -1;
        }
        pgencode_bytes_len("\\N", 2, buff + offset);
        offset += 2;
    } else {
        PyObject *encoded = PyObject_Str(obj);
        if (encoded == NULL) {
            return -1;
        }
        offset = pgencode_unicode(encoded, offset);
        Py_DECREF(encoded);
    }
    return offset;
}

static PyObject *
pgencode(PyObject *self, PyObject *args) {
    PyObject *obj;

    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }

    int size = pgencode_object(obj, 0, 0);
    if (size < 0) {
        return NULL;
    }

    return PyBytes_FromStringAndSize(buff, size);
}


static PyMethodDef methods[] = {
    {"pgencode",  pgencode, METH_VARARGS, "Return PostgreSQL bulk import encoding of object."},
    {0, 0, 0, 0}   // sentinel
};

static PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "_pgencode",
    0,
    -1,
    methods
};

PyMODINIT_FUNC
PyInit__pgencode(void) {
    module = PyModule_Create(&module_def);
    if (!module) {
        return NULL;
    }

    return module;
}
