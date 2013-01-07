/**
 * QR Code generator module for Python
 *
 * Copyright (c) 2007-2013 Ryusuke SEKIYAMA. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * @package     pyqr
 * @author      Ryusuke SEKIYAMA <rsky0711@gmail.com>
 * @copyright   2007-2013 Ryusuke SEKIYAMA
 * @license     http://www.opensource.org/licenses/mit-license.php  MIT License
 */

#include "qrmodule.h"
#include "qr_util.h"
#include <structmember.h>

/* {{{ type object declarations */

staticforward PyTypeObject QRCodeObjectType;

struct _QRCodeObject {
    PyObject_HEAD
    QRCode *qr;
    QRStructured *st;
    int pos;
    int format;
    int separator;
    int magnify;
    int order;
};

/* }}} */
/* {{{ errors */

static PyObject *QRCodeError;

/* }}} */
/* {{{ pydoc */

PyDoc_STRVAR(qr_module__doc__,
"QR Code generator module for Python\n"
"\n"
"This module provides following functions:\n"
"\n"
"qrcode(data,[output],...) -- Return a QR Code symbol as a string.\n"
"mimetype(format) -- Return a Content-Type for the given format.\n"
"extension(format[, include_dot]) -- Return a filename extension.");

PyDoc_STRVAR(qrcode__doc__,
"qrcode(data,[output],...) -- Return a QR Code symbol as a string.\n"
"\n"
"Optional output is the target file object.\n"
"If output is specified, returns size of the result as an integer.\n"
"\n"
"following options are also available:\n"
"  version:   symbol version (1-40, default: auto)\n"
"  mode:      encoding mode (MN, MA, M8, MK, default: auto)\n"
"  eclevel:   error correction level (ECL_{L,M,Q,H}, default: ECL_M)\n"
"  masktype:  mask pattern (0-7, default: auto)\n"
"  maxnum:    maximum number of symbols (1-16, default: 1)\n"
"  format:    output format (default: DIGIT)\n"
"             Available formats are followings.\n"
"               DIGIT, ASCII, JSON, PBM, BMP, SVG,\n"
"               TIFF, GIF, JPEG, PNG, WBMP\n"
"  separator: separator pattan width (0-16, default: 4)\n"
"             '4' is the lower limit of the QR Code specification.\n"
"  order:     ordering method of symbols, in case value is\n"
"             = 0 (default): order to square as possible\n"
"             >= 1: order each NUM symbols to horizontal\n"
"             <= -1: order each NUM symbols to vertical");

PyDoc_STRVAR(mimetype__doc__,
"mimetype(format) -- Return a Content-Type\n"
"which corresponds to the given output format.");

PyDoc_STRVAR(extension__doc__,
"extension(format[, include_dot]) -- Return a filename extension\n"
"which corresponds to the given output format.\n"
"\n"
"If optional arg include_dot is zero, the result will not\n"
"include leading dot.");

PyDoc_STRVAR(QRCode_class__doc__, "QR Code generator class.");

/* }}} */
/* {{{ method definitions and member definitions */

static PyMethodDef qr_methods[] = {
    { "qrcode", (PyCFunction)qr_qrcode,
        METH_VARARGS | METH_KEYWORDS, qrcode__doc__ },
    { "mimetype", qr_mimetype, METH_VARARGS, mimetype__doc__ },
    { "extension", qr_extension, METH_VARARGS, extension__doc__ },
    { NULL, NULL, 0, NULL }
};

static PyMethodDef QRCode_methods[] = {
    { "add_data", (PyCFunction)QRCode_add_data,
        METH_CLASS | METH_KEYWORDS, NULL },
    { "read_data", (PyCFunction)QRCode_read_data,
        METH_CLASS | METH_KEYWORDS, NULL },
    { "copy", (PyCFunction)QRCode_copy,
        METH_CLASS | METH_NOARGS, NULL },
    { "finalize", (PyCFunction)QRCode_finalize,
        METH_CLASS | METH_NOARGS, NULL },
    { "get_symbol", (PyCFunction)QRCode_get_symbol,
        METH_CLASS | METH_KEYWORDS, NULL },
    { "output_symbol", (PyCFunction)QRCode_output_symbol,
        METH_CLASS | METH_KEYWORDS, NULL },
    { "get_info", (PyCFunction)QRCode_get_info,
        METH_CLASS | METH_NOARGS, NULL },
    { NULL, NULL, 0, NULL }
};

static PyMemberDef QRCode_members[] = {
    { "format",    T_INT, offsetof(QRCodeObject, format),    0, "format" },
    { "separator", T_INT, offsetof(QRCodeObject, separator), 0, "separator" },
    { "magnify",   T_INT, offsetof(QRCodeObject, magnify),   0, "magnify" },
    { "order",     T_INT, offsetof(QRCodeObject, format),    0, "order" },
    { NULL, 0, 0, 0, NULL }
};

/* }}} */
/* {{{ initialize a module */

#define QR_DECLARE_CONSTANT(name) { \
    v = PyInt_FromLong(QR_##name); \
    PyDict_SetItemString(d, #name, v); \
    Py_DECREF(v); \
}

#define QR_DECLARE_CONSTANT_EX(name, value) { \
    v = PyInt_FromLong(value); \
    PyDict_SetItemString(d, #name, v); \
    Py_DECREF(v); \
}

PyMODINIT_FUNC initqr(void)
{
    PyObject *m, *d, *v;

    QRCodeObjectType.ob_type = &PyType_Type;
    if (PyType_Ready(&QRCodeObjectType) < 0) {
        return;
    }

    m = Py_InitModule4("qr", qr_methods,
                       qr_module__doc__,
                       NULL, PYTHON_API_VERSION);
    d = PyModule_GetDict(m);

    /* encoding mode (fullname) */
    QR_DECLARE_CONSTANT(EM_AUTO);
    QR_DECLARE_CONSTANT(EM_NUMERIC);
    QR_DECLARE_CONSTANT(EM_ALNUM);
    QR_DECLARE_CONSTANT(EM_8BIT);
    QR_DECLARE_CONSTANT(EM_KANJI);

    /* encoding mode (alias) */
    QR_DECLARE_CONSTANT_EX(MN, QR_EM_NUMERIC);
    QR_DECLARE_CONSTANT_EX(MA, QR_EM_ALNUM);
    QR_DECLARE_CONSTANT_EX(M8, QR_EM_8BIT);
    QR_DECLARE_CONSTANT_EX(MK, QR_EM_KANJI);

    /* error correction level (fullname) */
    QR_DECLARE_CONSTANT(ECL_L);
    QR_DECLARE_CONSTANT(ECL_M);
    QR_DECLARE_CONSTANT(ECL_Q);
    QR_DECLARE_CONSTANT(ECL_H);

    /* output format (fullname) */
    QR_DECLARE_CONSTANT(FMT_DIGIT);
    QR_DECLARE_CONSTANT(FMT_ASCII);
    QR_DECLARE_CONSTANT(FMT_JSON);
    QR_DECLARE_CONSTANT(FMT_PBM);
    QR_DECLARE_CONSTANT(FMT_BMP);
    QR_DECLARE_CONSTANT(FMT_SVG);
    QR_DECLARE_CONSTANT(FMT_TIFF);
    QR_DECLARE_CONSTANT(FMT_GIF);
    QR_DECLARE_CONSTANT(FMT_JPEG);
    QR_DECLARE_CONSTANT(FMT_PNG);
    QR_DECLARE_CONSTANT(FMT_WBMP);

    /* output format (alias) */
    QR_DECLARE_CONSTANT_EX(DIGIT, QR_FMT_DIGIT);
    QR_DECLARE_CONSTANT_EX(ASCII, QR_FMT_ASCII);
    QR_DECLARE_CONSTANT_EX(JSON,  QR_FMT_JSON);
    QR_DECLARE_CONSTANT_EX(PBM,   QR_FMT_PBM);
    QR_DECLARE_CONSTANT_EX(BMP,   QR_FMT_BMP);
    QR_DECLARE_CONSTANT_EX(SVG,   QR_FMT_SVG);
    QR_DECLARE_CONSTANT_EX(TIFF,  QR_FMT_TIFF);
    QR_DECLARE_CONSTANT_EX(GIF,   QR_FMT_GIF);
    QR_DECLARE_CONSTANT_EX(JPEG,  QR_FMT_JPEG);
    QR_DECLARE_CONSTANT_EX(PNG,   QR_FMT_PNG);
    QR_DECLARE_CONSTANT_EX(WBMP,  QR_FMT_WBMP);

    /* add objects */
    QRCodeError = PyErr_NewException("qr.Error", PyExc_RuntimeError, NULL);
    Py_INCREF(QRCodeError);
    PyModule_AddObject(m, "Error", QRCodeError);

    Py_INCREF(&QRCodeObjectType);
    PyModule_AddObject(m, "QRCode", (PyObject *)&QRCodeObjectType);
}

#undef QR_DECLARE_CONSTANT
#undef QR_DECLARE_CONSTANT_EX

/* }}} */
/* {{{ qrcode() */

static PyObject *
qr_qrcode(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {
        "data", "output",
        "version", "mode", "eclevel", "masktype", "maxnum",
        "format", "magnify", "separator", "order",
        NULL
    };

    const char *data = NULL;
    int data_len = 0;
    PyObject *output = NULL;
    PyObject *writer = NULL;
    PyObject *result = NULL;
    FILE *out = NULL;

    int version = -1;
    int mode = QR_EM_AUTO;
    int eclevel = QR_ECL_M;
    int masktype = -1;
    int maxnum = 1;
    int format = QR_FMT_DIGIT;
    int magnify = 1;
    int separator = 4;
    int order = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s#|Oiiiiiiiii:qrcode", kwlist,
                                     &data, &data_len, &output,
                                     &version, &mode, &eclevel,
                                     &masktype, &maxnum,
                                     &format, &magnify, &separator, &order))
    {
        return NULL;
    }

    if (output) {
        if (PyFile_Check(output)) {
            out = PyFile_AsFile(output);
            if (out == NULL) {
                PyErr_SetString(PyExc_ValueError, "output already closed");
                return NULL;
            }
        }
        else {
            writer = PyObject_GetAttrString(output, "write");
            if (writer == NULL) {
                PyErr_SetString(PyExc_TypeError,
                                "output must be a file object");
                return NULL;
            }
        }
    }

    if (maxnum == 1) {
        result = _qr_process((const qr_byte_t *)data, data_len, out,
                             version, mode, eclevel, masktype,
                             format, magnify, separator);
    }
    else {
        if (version == -1) {
            version = 1;
        }
        result = _qrs_process((const qr_byte_t *)data, data_len, out,
                              version, mode, eclevel, masktype, maxnum,
                              format, magnify, separator, order);
    }

    if (result != NULL && writer != NULL) {
        PyObject *write_args, *write_result;

        write_args = PyTuple_Pack(1, result);
        if (write_args == NULL) {
            Py_DECREF(result);
            result = NULL;
        }
        else {
            write_result = PyEval_CallObject(writer, write_args);
            Py_DECREF(write_args);
            Py_DECREF(result);
            result = write_result;
        }
    }

    if (writer != NULL) {
        Py_DECREF(writer);
    }

    return result;
}

/* }}} */
/* {{{ qr_mimetype() */

static PyObject *
qr_mimetype(PyObject *self, PyObject *args)
{
    const char *mimetype = NULL;
    int format = -1;

    if (!PyArg_ParseTuple(args, "i:mimetype", &format)) {
        return NULL;
    }

    mimetype = qrMimeType(format);
    if (mimetype == NULL) {
        PyErr_SetString(PyExc_ValueError, qrStrError(QR_ERR_INVALID_FMT));
        return NULL;
    }

    return Py_BuildValue("s", mimetype);
}

/* }}} */
/* {{{ qr_extension() */

static PyObject *
qr_extension(PyObject *self, PyObject *args)
{
    const char *extension = NULL;
    int format = -1;
    int include_dot = 1;

    if (!PyArg_ParseTuple(args, "i|i:extension", &format, &include_dot)) {
        return NULL;
    }

    extension = qrExtension(format);
    if (extension == NULL) {
        PyErr_SetString(PyExc_ValueError, qrStrError(QR_ERR_INVALID_FMT));
        return NULL;
    }

    if (include_dot) {
        char extension_buffer[QR_EXT_MAX_LEN + 2] = {0};
        snprintf(&(extension_buffer[0]), QR_EXT_MAX_LEN + 2, ".%s", extension);
        return Py_BuildValue("s", extension_buffer);
    }
    else {
        return Py_BuildValue("s", extension);
    }
}

/* }}} */
/* {{{ QRCode_add_data() */

static PyObject *
QRCode_add_data(QRCodeObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return NULL;
}

/* }}} */
/* {{{ QRCode_read_data() */

static PyObject *
QRCode_read_data(QRCodeObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return NULL;
}

/* }}} */
/* {{{ QRCode_copy() */

static PyObject *
QRCode_copy(QRCodeObject *self, PyObject *unused)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return NULL;
}

/* }}} */
/* {{{ QRCode_finalize() */

static PyObject *
QRCode_finalize(QRCodeObject *self, PyObject *unused)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return NULL;
}

/* }}} */
/* {{{ QRCode_get_symbol() */

static PyObject *
QRCode_get_symbol(QRCodeObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return NULL;
}

/* }}} */
/* {{{ QRCode_output_symbol() */

static PyObject *
QRCode_output_symbol(QRCodeObject *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return NULL;
}

/* }}} */
/* {{{ QRCode_get_info() */

static PyObject *
QRCode_get_info(QRCodeObject *self, PyObject *unused)
{
    PyErr_SetString(PyExc_NotImplementedError, "not yet implemented");
    return NULL;
}

/* }}} */
/* {{{ _qr_process() */

static PyObject *
_qr_process(const qr_byte_t *data, int data_len, FILE *out,
            int version, int mode, int eclevel, int masktype,
            int format, int magnify, int separator)
{
    PyObject *result = NULL;
    QRCode *qr = NULL;
    int size = 0;

    qr = _qr_create_simple(data, data_len, version, mode, eclevel, masktype);
    if (qr == NULL) {
        return NULL;
    }

    if (out) {
        size = _qr_output_symbol(qr, out, format, separator, magnify);
        if (size > 0) {
            result = Py_BuildValue("i", size);
        }
    }
    else {
        qr_byte_t *symbol = NULL;

        symbol = _qr_get_symbol(qr, format, separator, magnify, &size);
        if (symbol) {
            result = Py_BuildValue("s#", (char *)symbol, size);
            free(symbol);
        }
    }

    qrDestroy(qr);

    return result;
}

/* }}} */
/* {{{ _qr_create_simple() */

static QRCode *
_qr_create_simple(const qr_byte_t *data, int data_len,
                  int version, int mode, int eclevel, int masktype)
{
    QRCode *qr = NULL;
    int errcode = QR_ERR_NONE;

    qr = qrInit(version, mode, eclevel, masktype, &errcode);
    if (qr == NULL) {
        if (errcode == QR_ERR_MEMORY_EXHAUSTED) {
            PyErr_SetString(PyExc_MemoryError, qrStrError(errcode));
        }
        else {
            PyErr_SetString(PyExc_ValueError, qrStrError(errcode));
        }
        return NULL;
    }

    if (!qrAddData2(qr, data, data_len, mode)) {
        PyErr_SetString(PyExc_ValueError, qrGetErrorInfo(qr));
        qrDestroy(qr);
        return NULL;
    }

    if (!qrFinalize(qr)) {
        PyErr_SetString(QRCodeError, qrGetErrorInfo(qr));
        qrDestroy(qr);
        return NULL;
    }

    return qr;
}

/* }}} */
/* {{{ _qr_get_symbol() */

static qr_byte_t *
_qr_get_symbol(QRCode *qr,
               int format, int separator, int magnify, int *symbol_size)
{
    qr_byte_t *symbol = NULL;

    symbol = qrGetSymbol(qr, format, separator, magnify, symbol_size);
    if (symbol == NULL) {
        _qr_set_get_error(qrGetErrorCode(qr), qrGetErrorInfo(qr));
        return NULL;
    }

    return symbol;
}

/* }}} */
/* {{{ _qr_output_symbol() */

static int
_qr_output_symbol(QRCode *qr, FILE *out,
                  int format, int separator, int magnify)
{
    int out_size = 0;

    out_size = qrOutputSymbol(qr, out, format, separator, magnify);
    if (out_size <= 0) {
        _qr_set_output_error(qrGetErrorCode(qr), qrGetErrorInfo(qr));
    }

    return out_size;
}

/* }}} */
/* {{{ _qrs_process() */

static PyObject *
_qrs_process(const qr_byte_t *data, int data_len, FILE *out,
             int version, int mode, int eclevel, int masktype, int maxnum,
             int format, int magnify, int separator, int order)
{
    PyObject *result = NULL;
    QRStructured *st = NULL;
    int size = 0;

    st = _qrs_create_simple((const qr_byte_t *)data, data_len,
            version, mode, eclevel, masktype, maxnum);
    if (st == NULL) {
        return NULL;
    }

    if (out) {
        size = _qrs_output_symbols(st, out, format, separator, magnify, order);
        if (size > 0) {
            result = Py_BuildValue("i", size);
        }
    }
    else {
        qr_byte_t *symbol = NULL;

        symbol = _qrs_get_symbols(st, format, separator, magnify, order, &size);
        if (symbol) {
            result = Py_BuildValue("s#", (char *)symbol, size);
            free(symbol);
        }
    }

    qrsDestroy(st);

    return result;
}

/* }}} */
/* {{{ _qrs_create_simple() */

static QRStructured *
_qrs_create_simple(const qr_byte_t *data, int data_len,
                   int version, int mode, int eclevel, int masktype, int maxnum)
{
    QRStructured *st = NULL;
    int errcode = QR_ERR_NONE;

    st = qrsInit(version, mode, eclevel, masktype, maxnum, &errcode);
    if (st == NULL) {
        if (errcode == QR_ERR_MEMORY_EXHAUSTED) {
            PyErr_SetString(PyExc_MemoryError, qrStrError(errcode));
        }
        else {
            PyErr_SetString(PyExc_ValueError, qrStrError(errcode));
        }
        return NULL;
    }

    if (!qrsAddData2(st, data, data_len, mode)) {
        PyErr_SetString(PyExc_ValueError, qrsGetErrorInfo(st));
        qrsDestroy(st);
        return NULL;
    }

    if (!qrsFinalize(st)) {
        PyErr_SetString(QRCodeError, qrsGetErrorInfo(st));
        qrsDestroy(st);
        return NULL;
    }

    return st;
}

/* }}} */
/* {{{ _qrs_get_symbols() */

static qr_byte_t *
_qrs_get_symbols(QRStructured * st,
                 int format, int separator, int magnify,
                 int order, int *symbol_size)
{
    qr_byte_t *symbol = NULL;

    symbol = qrsGetSymbols(st, format, separator, magnify, order, symbol_size);
    if (symbol == NULL) {
        _qr_set_get_error(qrsGetErrorCode(st), qrsGetErrorInfo(st));
        return NULL;
    }

    return symbol;
}

/* }}} */
/* {{{ _qrs_output_symbols() */

static int
_qrs_output_symbols(QRStructured * st, FILE *out,
                    int format, int separator, int magnify, int order)
{
    int out_size = 0;

    out_size = qrsOutputSymbols(st, out, format, separator, magnify, order);
    if (out_size <= 0) {
        _qr_set_output_error(qrsGetErrorCode(st), qrsGetErrorInfo(st));
    }

    return out_size;
}

/* }}} */
/* {{{ _qr_set_get_error() */

static void
_qr_set_get_error(int errcode, const char *errmsg)
{
    if (errcode >= QR_ERR_INVALID_ARG && errcode <= QR_ERR_EMPTY_PARAM) {
        PyErr_SetString(PyExc_ValueError, errmsg);
    }
    else if (errcode == QR_ERR_SEE_ERRNO) {
        PyErr_SetFromErrno(QRCodeError);
    }
    else {
        PyErr_SetString(QRCodeError, errmsg);
    }
}

/* }}} */
/* {{{ _qr_set_output_error() */

static void
_qr_set_output_error(int errcode, const char *errmsg)
{
    if (errcode >= QR_ERR_INVALID_ARG && errcode <= QR_ERR_EMPTY_PARAM) {
        PyErr_SetString(PyExc_ValueError, errmsg);
    }
    else if (errcode == QR_ERR_SEE_ERRNO) {
        PyErr_SetFromErrno(PyExc_IOError);
    }
    else if (errcode == QR_ERR_FWRITE) {
        PyErr_SetString(PyExc_IOError, errmsg);
    }
    else {
        PyErr_SetString(QRCodeError, errmsg);
    }
}

/* }}} */
/* {{{ type object entities */

static PyTypeObject QRCodeObjectType = {
    PyObject_HEAD_INIT(NULL)
    0,                                  /* ob_size */
    "qr.QRCode",                        /* tp_name */
    sizeof(QRCodeObject),               /* tp_basicsize */
    0,                                  /* tp_itemsize */ 
    (destructor)QRCode_dealloc,         /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    QRCode_class__doc__,                /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    QRCode_methods,                     /* tp_methods */
    QRCode_members,                     /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)QRCode_init,              /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};

/* }}} */
/* {{{ QRCode_init() */

static int
QRCode_init(QRCodeObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {
        "version", "mode", "eclevel", "masktype", "maxnum",
        "format", "magnify", "separator", "order",
        NULL
    };

    int errcode = QR_ERR_NONE;
    int version = -1;
    int mode = QR_EM_AUTO;
    int eclevel = QR_ECL_M;
    int masktype = -1;
    int maxnum = 1;

    self->format = QR_FMT_DIGIT;
    self->magnify = 1;
    self->separator = 4;
    self->order = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "|iiiiiiiii:QRCode.__init__",
                                     kwlist,
                                     &version, &mode, &eclevel,
                                     &masktype, &maxnum,
                                     &self->format, &self->magnify,
                                     &self->separator, &self->order))
    {
        return -1;
    }

    if (maxnum == 1) {
        self->st = NULL;
        self->qr = qrInit(version, mode, eclevel, masktype, &errcode);
    }
    else {
        if (version == -1) {
            version = 1;
        }
        self->qr = NULL;
        self->st = qrsInit(version, mode, eclevel, masktype, maxnum, &errcode);
    }

    if (self->qr == NULL && self->st == NULL) {
        if (errcode == QR_ERR_MEMORY_EXHAUSTED) {
            PyErr_SetString(PyExc_MemoryError, qrStrError(errcode));
        }
        else {
            PyErr_SetString(PyExc_ValueError, qrStrError(errcode));
        }
        return -1;
    }

    return 0;
}

/* }}} */
/* {{{ QRCode_dealloc() */

static void
QRCode_dealloc(QRCodeObject *self)
{
    if (self->st) {
        qrsDestroy(self->st);
    }
    else if (self->qr) {
        qrDestroy(self->qr);
    }
    self->ob_type->tp_free(self);
}

/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: nil;
 * End:
 * vim600: et sw=4 ts=4 fdm=marker
 * vim<600: et sw=4 ts=4
 */
