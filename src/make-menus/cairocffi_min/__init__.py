"""
    cairocffi
    ~~~~~~~~~

    CFFI-based cairo bindings for Python. See README for details.

    :copyright: Copyright 2013-2019 by Simon Sapin
    :license: BSD, see LICENSE for details.

"""

import os
import sys
from contextlib import suppress
from ctypes.util import find_library

from . import constants
from .ffi import ffi

VERSION = __version__ = '1.7.1'
# supported version of cairo, used to be pycairo version too:
version = '1.17.2'
version_info = (1, 17, 2)


# Python 3.8 no longer searches for DLLs in PATH, so we can add everything in
# CAIROCFFI_DLL_DIRECTORIES manually. Note that unlike PATH, add_dll_directory
# has no defined order, so if there are two cairo DLLs in PATH we might get a
# random one.
dll_directories = os.getenv('CAIROCFFI_DLL_DIRECTORIES')
if dll_directories and hasattr(os, 'add_dll_directory'):
    for path in dll_directories.split(';'):
        with suppress((OSError, FileNotFoundError)):
            os.add_dll_directory(path)


def dlopen(ffi, library_names, filenames):
    """Try various names for the same library, for different platforms."""
    exceptions = []

    for library_name in library_names:
        library_filename = find_library(library_name)
        if library_filename:
            filenames = (library_filename, *filenames)
        else:
            exceptions.append(
                'no library called "{}" was found'.format(library_name))

    for filename in filenames:
        try:
            return ffi.dlopen(filename)
        except OSError as exception:  # pragma: no cover
            exceptions.append(exception)

    error_message = '\n'.join(  # pragma: no cover
        str(exception) for exception in exceptions)
    raise OSError(error_message)  # pragma: no cover


cairo = dlopen(
    ffi, ('cairo',),
    ('cairo.dll',))


class _keepref(object):  # noqa: N801
    """Function wrapper that keeps a reference to another object."""
    def __init__(self, ref, func):
        self.ref = ref
        self.func = func

    def __call__(self, *args, **kwargs):
        self.func(*args, **kwargs)


class CairoError(Exception):
    """Raised when cairo returns an error status."""
    def __init__(self, message, status):
        super(CairoError, self).__init__(message)
        self.status = status


Error = CairoError  # pycairo compat

STATUS_TO_EXCEPTION = {
    constants.STATUS_NO_MEMORY: MemoryError,
    constants.STATUS_READ_ERROR: IOError,
    constants.STATUS_WRITE_ERROR: IOError,
    constants.STATUS_TEMP_FILE_ERROR: IOError,
    constants.STATUS_FILE_NOT_FOUND: FileNotFoundError,
}


def _check_status(status):
    """Take a cairo status code and raise an exception if/as appropriate."""
    if status != constants.STATUS_SUCCESS:
        exception = STATUS_TO_EXCEPTION.get(status, CairoError)
        status_name = ffi.string(ffi.cast("cairo_status_t", status))
        message = 'cairo returned %s: %s' % (
            status_name, ffi.string(cairo.cairo_status_to_string(status)))
        raise exception(message, status)


def cairo_version():
    """Return the cairo version number as a single integer,
    such as 11208 for ``1.12.8``.
    Major, minor and micro versions are "worth" 10000, 100 and 1 respectively.

    Can be useful as a guard for method not available in older cairo versions::

        if cairo_version() >= 11000:
            surface.set_mime_data('image/jpeg', jpeg_bytes)

    """
    return cairo.cairo_version()


def cairo_version_string():
    """Return the cairo version number as a string, such as ``1.12.8``."""
    return ffi.string(cairo.cairo_version_string()).decode('ascii')


def install_as_pycairo():
    """Install cairocffi so that ``import cairo`` imports it.

    cairoffi’s API is compatible with pycairo as much as possible.

    """
    sys.modules['cairo'] = sys.modules[__name__]


# Implementation is in submodules, but public API is all here.

from .surfaces import (  # noqa isort:skip
    Surface, ImageSurface, PDFSurface, PSSurface, SVGSurface, RecordingSurface,
    Win32Surface, Win32PrintingSurface)
try:
    from .xcb import XCBSurface  # noqa isort:skip
except ImportError:
    pass
from .patterns import (  # noqa isort:skip
    Pattern, SolidPattern, SurfacePattern, Gradient, LinearGradient,
    RadialGradient)
from .fonts import (  # noqa isort:skip
    FontFace, ToyFontFace, ScaledFont, FontOptions)
from .context import Context  # noqa isort:skip
from .matrix import Matrix  # noqa isort:skip

from .constants import *  # noqa isort:skip
