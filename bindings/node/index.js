'use strict';
const koffi = require('koffi');
const path = require('node:path');
const fs = require('node:fs');

function defaultLibrary() {
  const base = path.resolve(__dirname, '../..', 'build');
  const name = process.env.MONKEYDB_LIBRARY || (process.platform === 'win32' ? 'monkeydb.dll' : process.platform === 'darwin' ? 'libmonkeydb.dylib' : 'libmonkeydb.so');
  return path.isAbsolute(name) ? name : path.join(base, name);
}
const libraryPath = process.env.MONKEYDB_LIBRARY || defaultLibrary();
if (!fs.existsSync(libraryPath)) throw new Error(`MonkeyDB shared library not found: ${libraryPath}`);
const lib = koffi.load(libraryPath);
const open = lib.func('void * monkeydb_collection_open(const char * path)');
const close = lib.func('void monkeydb_collection_close(void * handle)');
const insertJson = lib.func('void * monkeydb_insert_json(void * handle, const char * document_json)');
const findJson = lib.func('void * monkeydb_find_by_id_json(void * handle, const char * id)');
const deleteById = lib.func('int monkeydb_delete_by_id(void * handle, const char * id)');
const count = lib.func('int monkeydb_count(void * handle)');
const lastError = lib.func('const char * monkeydb_last_error(void)');
const free = lib.func('void monkeydb_free(void * memory)');

function readOwnedString(ptr) {
  if (!ptr) return null;
  try { return koffi.decode(ptr, 'char', -1); }
  finally { free(ptr); }
}
function errorText() { return lastError() || ''; }

class Collection {
  constructor(filePath) {
    this.handle = open(filePath);
    if (!this.handle) throw new Error(errorText() || 'Cannot open collection');
  }
  insert(document) {
    const result = readOwnedString(insertJson(this.handle, JSON.stringify(document)));
    if (result === null) throw new Error(errorText() || 'Insert failed');
    return result;
  }
  findById(id) {
    const result = readOwnedString(findJson(this.handle, String(id)));
    if (result === null) { const error = errorText(); if (error) throw new Error(error); return null; }
    return JSON.parse(result);
  }
  deleteById(id) {
    const result = deleteById(this.handle, String(id));
    if (result < 0) throw new Error(errorText());
    return result === 1;
  }
  count() {
    const result = count(this.handle);
    if (result < 0) throw new Error(errorText());
    return result;
  }
  close() { if (this.handle) { close(this.handle); this.handle = null; } }
}
module.exports = { Collection };
