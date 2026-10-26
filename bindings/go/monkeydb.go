package monkeydb

/*
#cgo CFLAGS: -I${SRCDIR}/../..
#cgo LDFLAGS: -L${SRCDIR}/../../build -lmonkeydb
#include <stdlib.h>
#include "MonkeyDB_C.h"
*/
import "C"
import (
	"encoding/json"
	"errors"
	"fmt"
	"runtime"
	"unsafe"
)

type Collection struct{ handle *C.monkeydb_collection_handle }

func lastError() error {
	message := C.GoString(C.monkeydb_last_error())
	if message == "" { return errors.New("MonkeyDB operation failed") }
	return errors.New(message)
}
func Open(path string) (*Collection, error) {
	runtime.LockOSThread(); defer runtime.UnlockOSThread()
	p := C.CString(path); defer C.free(unsafe.Pointer(p))
	h := C.monkeydb_collection_open(p)
	if h == nil { return nil, lastError() }
	return &Collection{handle: h}, nil
}
func (c *Collection) Close() { if c != nil && c.handle != nil { runtime.LockOSThread(); defer runtime.UnlockOSThread(); C.monkeydb_collection_close(c.handle); c.handle = nil } }
func (c *Collection) Insert(document any) (string, error) {
	runtime.LockOSThread(); defer runtime.UnlockOSThread()
	data, err := json.Marshal(document); if err != nil { return "", err }
	input := C.CString(string(data)); defer C.free(unsafe.Pointer(input))
	out := C.monkeydb_insert_json(c.handle, input); if out == nil { return "", lastError() }
	defer C.monkeydb_free(unsafe.Pointer(out)); return C.GoString(out), nil
}
func (c *Collection) FindByID(id string) (map[string]any, bool, error) {
	runtime.LockOSThread(); defer runtime.UnlockOSThread()
	input := C.CString(id); defer C.free(unsafe.Pointer(input))
	out := C.monkeydb_find_by_id_json(c.handle, input)
	if out == nil { message := C.GoString(C.monkeydb_last_error()); if message != "" { return nil, false, errors.New(message) }; return nil, false, nil }
	defer C.monkeydb_free(unsafe.Pointer(out))
	var document map[string]any
	if err := json.Unmarshal([]byte(C.GoString(out)), &document); err != nil { return nil, false, err }
	return document, true, nil
}
func (c *Collection) DeleteByID(id string) (bool, error) {
	runtime.LockOSThread(); defer runtime.UnlockOSThread()
	input := C.CString(id); defer C.free(unsafe.Pointer(input))
	result := C.monkeydb_delete_by_id(c.handle, input); if result < 0 { return false, lastError() }; return result == 1, nil
}
func (c *Collection) Count() (int, error) { runtime.LockOSThread(); defer runtime.UnlockOSThread(); result := C.monkeydb_count(c.handle); if result < 0 { return 0, fmt.Errorf("%w", lastError()) }; return int(result), nil }
