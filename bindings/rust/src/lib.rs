use serde::Serialize;
use serde_json::Value;
use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::ptr::NonNull;

#[repr(C)] struct RawCollection { _private: [u8; 0] }
#[link(name = "monkeydb")]
extern "C" {
    fn monkeydb_collection_open(path: *const c_char) -> *mut RawCollection;
    fn monkeydb_collection_close(handle: *mut RawCollection);
    fn monkeydb_insert_json(handle: *mut RawCollection, json: *const c_char) -> *mut c_char;
    fn monkeydb_find_by_id_json(handle: *mut RawCollection, id: *const c_char) -> *mut c_char;
    fn monkeydb_delete_by_id(handle: *mut RawCollection, id: *const c_char) -> c_int;
    fn monkeydb_count(handle: *mut RawCollection) -> c_int;
    fn monkeydb_last_error() -> *const c_char;
    fn monkeydb_free(memory: *mut c_void);
}
fn last_error() -> String {
    unsafe { let p = monkeydb_last_error(); if p.is_null() { return "MonkeyDB operation failed".into(); } CStr::from_ptr(p).to_string_lossy().into_owned() }
}
fn cstring(s: &str) -> Result<CString, String> { CString::new(s).map_err(|_| "input contains a NUL byte".to_string()) }

pub struct Collection { handle: NonNull<RawCollection> }
impl Collection {
    pub fn open(path: &str) -> Result<Self, String> {
        let path = cstring(path)?;
        let handle = unsafe { monkeydb_collection_open(path.as_ptr()) };
        NonNull::new(handle).map(|handle| Self { handle }).ok_or_else(last_error)
    }
    pub fn insert<T: Serialize>(&self, document: &T) -> Result<String, String> {
        let json = serde_json::to_string(document).map_err(|e| e.to_string())?;
        let json = cstring(&json)?;
        let p = unsafe { monkeydb_insert_json(self.handle.as_ptr(), json.as_ptr()) };
        if p.is_null() { return Err(last_error()); }
        let result = unsafe { CStr::from_ptr(p).to_string_lossy().into_owned() };
        unsafe { monkeydb_free(p.cast()); }
        Ok(result)
    }
    pub fn find_by_id(&self, id: &str) -> Result<Option<Value>, String> {
        let id = cstring(id)?;
        let p = unsafe { monkeydb_find_by_id_json(self.handle.as_ptr(), id.as_ptr()) };
        if p.is_null() { let e = last_error(); return if e.is_empty() { Ok(None) } else { Err(e) }; }
        let text = unsafe { CStr::from_ptr(p).to_string_lossy().into_owned() };
        unsafe { monkeydb_free(p.cast()); }
        serde_json::from_str(&text).map(Some).map_err(|e| e.to_string())
    }
    pub fn delete_by_id(&self, id: &str) -> Result<bool, String> {
        let id = cstring(id)?;
        let result = unsafe { monkeydb_delete_by_id(self.handle.as_ptr(), id.as_ptr()) };
        if result < 0 { Err(last_error()) } else { Ok(result == 1) }
    }
    pub fn count(&self) -> Result<usize, String> {
        let result = unsafe { monkeydb_count(self.handle.as_ptr()) };
        if result < 0 { Err(last_error()) } else { Ok(result as usize) }
    }
}
impl Drop for Collection { fn drop(&mut self) { unsafe { monkeydb_collection_close(self.handle.as_ptr()); } } }
