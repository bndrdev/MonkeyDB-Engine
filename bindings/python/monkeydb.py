"""Small Python wrapper for the MonkeyDB C ABI. Set MONKEYDB_LIBRARY to the shared-library path."""
import ctypes
import json
import os
from pathlib import Path


def _default_library():
    names = ("libmonkeydb.so", "libmonkeydb.dylib", "monkeydb.dll")
    for name in names:
        candidate = Path(__file__).resolve().parents[2] / "build" / name
        if candidate.exists():
            return str(candidate)
    return names[0]


_lib = ctypes.CDLL(os.environ.get("MONKEYDB_LIBRARY", _default_library()))
_Handle = ctypes.c_void_p
_lib.monkeydb_collection_open.argtypes = [ctypes.c_char_p]
_lib.monkeydb_collection_open.restype = _Handle
_lib.monkeydb_collection_close.argtypes = [_Handle]
_lib.monkeydb_insert_json.argtypes = [_Handle, ctypes.c_char_p]
_lib.monkeydb_insert_json.restype = ctypes.c_void_p
_lib.monkeydb_find_by_id_json.argtypes = [_Handle, ctypes.c_char_p]
_lib.monkeydb_find_by_id_json.restype = ctypes.c_void_p
_lib.monkeydb_delete_by_id.argtypes = [_Handle, ctypes.c_char_p]
_lib.monkeydb_delete_by_id.restype = ctypes.c_int
_lib.monkeydb_count.argtypes = [_Handle]
_lib.monkeydb_count.restype = ctypes.c_int
_lib.monkeydb_last_error.argtypes = []
_lib.monkeydb_last_error.restype = ctypes.c_char_p
_lib.monkeydb_free.argtypes = [ctypes.c_void_p]


def _last_error():
    raw = _lib.monkeydb_last_error()
    return raw.decode("utf-8", errors="replace") if raw else ""


class Collection:
    def __init__(self, path):
        self._handle = _lib.monkeydb_collection_open(os.fsencode(path))
        if not self._handle:
            raise RuntimeError(_last_error())

    def close(self):
        if self._handle:
            _lib.monkeydb_collection_close(self._handle)
            self._handle = None

    def insert(self, document):
        raw = json.dumps(document, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        ptr = _lib.monkeydb_insert_json(self._handle, raw)
        if not ptr:
            raise RuntimeError(_last_error())
        try:
            return ctypes.string_at(ptr).decode("utf-8")
        finally:
            _lib.monkeydb_free(ptr)

    def find_by_id(self, document_id):
        ptr = _lib.monkeydb_find_by_id_json(self._handle, str(document_id).encode("utf-8"))
        if not ptr:
            message = _last_error()
            if message:
                raise RuntimeError(message)
            return None
        try:
            return json.loads(ctypes.string_at(ptr).decode("utf-8"))
        finally:
            _lib.monkeydb_free(ptr)

    def delete_by_id(self, document_id):
        result = _lib.monkeydb_delete_by_id(self._handle, str(document_id).encode("utf-8"))
        if result < 0:
            raise RuntimeError(_last_error())
        return result == 1

    def count(self):
        result = _lib.monkeydb_count(self._handle)
        if result < 0:
            raise RuntimeError(_last_error())
        return result

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.close()
