#include "MonkeyCollection.hpp"
#include "MonkeyDB_C.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
int main(){
 const std::string path="/tmp/monkeydb_test.jsonl";std::remove(path.c_str());std::remove((path+".lock").c_str());
 MonkeyCollection c(path);auto child=std::make_shared<MonkeyObject>();(*child)["note"]=std::string("quote \" slash \\ newline\n");auto arr=std::make_shared<MonkeyArray>();arr->push_back(4);arr->push_back(child);
 MonkeyDoc d;d["name"]=std::string("Ada");d["nested"]=child;d["items"]=arr;auto id=c.insert(d);auto got=c.findById(id);assert(got);assert(std::get<std::string>(got->at("name"))=="Ada");auto n=std::get<std::shared_ptr<MonkeyObject>>(got->at("nested"));assert(std::get<std::string>(n->at("note"))=="quote \" slash \\ newline\n");auto a=std::get<std::shared_ptr<MonkeyArray>>(got->at("items"));assert(a->size()==2&&std::get<int>((*a)[0])==4);
 assert(c.updateById(id,[](MonkeyDoc& x){x["active"]=true;}));assert(c.findById(id)->size()==5);assert(c.count()==1);assert(c.deleteById(id));assert(!c.findById(id));
 auto h=monkeydb_collection_open(path.c_str());assert(h);char* cid=monkeydb_insert_json(h,"{\"_id\":\"from-c\",\"data\":[1,{\"ok\":true}]}");assert(cid&&std::string(cid)=="from-c");monkeydb_free(cid);char* json=monkeydb_find_by_id_json(h,"from-c");assert(json&&std::string(json).find("\"ok\":true")!=std::string::npos);monkeydb_free(json);assert(monkeydb_count(h)==1);assert(monkeydb_delete_by_id(h,"from-c")==1);monkeydb_collection_close(h);
 std::remove(path.c_str());std::remove((path+".lock").c_str());return 0;
}
