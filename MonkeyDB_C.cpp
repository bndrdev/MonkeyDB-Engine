#include "MonkeyDB_C.h"
#include "MonkeyCollection.hpp"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <stdexcept>
#include <exception>
struct monkeydb_collection_handle { MonkeyCollection collection; explicit monkeydb_collection_handle(const char* p):collection(p){} };
namespace { thread_local std::string lastError; char* copyString(const std::string& s){auto p=static_cast<char*>(std::malloc(s.size()+1));if(p)std::memcpy(p,s.c_str(),s.size()+1);return p;} }
extern "C" {
monkeydb_collection_handle* monkeydb_collection_open(const char* path){try{lastError.clear();if(!path)throw std::invalid_argument("path is null");return new monkeydb_collection_handle(path);}catch(const std::exception&e){lastError=e.what();return nullptr;}}
void monkeydb_collection_close(monkeydb_collection_handle* h){delete h;}
char* monkeydb_insert_json(monkeydb_collection_handle* h,const char* json){try{lastError.clear();if(!h||!json)throw std::invalid_argument("handle or document is null");auto id=h->collection.insertJson(json);auto p=copyString(id);if(!p)throw std::bad_alloc();return p;}catch(const std::exception&e){lastError=e.what();return nullptr;}}
char* monkeydb_find_by_id_json(monkeydb_collection_handle* h,const char* id){try{lastError.clear();if(!h||!id)throw std::invalid_argument("handle or id is null");auto json=h->collection.findByIdJson(id);if(!json)return nullptr;auto p=copyString(*json);if(!p)throw std::bad_alloc();return p;}catch(const std::exception&e){lastError=e.what();return nullptr;}}
int monkeydb_delete_by_id(monkeydb_collection_handle* h,const char* id){try{lastError.clear();if(!h||!id)throw std::invalid_argument("handle or id is null");return h->collection.deleteById(id)?1:0;}catch(const std::exception&e){lastError=e.what();return -1;}}
int monkeydb_count(monkeydb_collection_handle* h){try{lastError.clear();if(!h)throw std::invalid_argument("handle is null");return h->collection.count();}catch(const std::exception&e){lastError=e.what();return -1;}}
const char* monkeydb_last_error(void){return lastError.c_str();}
void monkeydb_free(void* p){std::free(p);}
}
