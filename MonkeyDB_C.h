#ifndef MONKEYDB_C_H
#define MONKEYDB_C_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct monkeydb_collection_handle monkeydb_collection_handle;
/* Open a JSONL collection file. Returns NULL on error; inspect monkeydb_last_error(). */
monkeydb_collection_handle* monkeydb_collection_open(const char* path);
void monkeydb_collection_close(monkeydb_collection_handle* handle);
/* Input must be one JSON object. Returns a malloc-owned ID string, or NULL on error. */
char* monkeydb_insert_json(monkeydb_collection_handle* handle, const char* document_json);
/* Returns a malloc-owned JSON object string, or NULL if not found/error. */
char* monkeydb_find_by_id_json(monkeydb_collection_handle* handle, const char* id);
int monkeydb_delete_by_id(monkeydb_collection_handle* handle, const char* id);
int monkeydb_count(monkeydb_collection_handle* handle);
const char* monkeydb_last_error(void);
void monkeydb_free(void* memory);
#ifdef __cplusplus
}
#endif
#endif
