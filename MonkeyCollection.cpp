#include "MonkeyCollection.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <random>
#include <iomanip>
#include <filesystem>
#include <cmath>
#include <cctype>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
namespace {
std::string quote(const std::string& s) {
 std::string o="\""; for(unsigned char c:s){switch(c){case '"':o+="\\\"";break;case '\\':o+="\\\\";break;case '\n':o+="\\n";break;case '\r':o+="\\r";break;case '\t':o+="\\t";break;default:if(c<32){char b[7];std::snprintf(b,sizeof b,"\\u%04x",c);o+=b;}else o+=char(c);}} return o+'"';
}
class Parser {
 const std::string& s; size_t p=0;
 void ws(){while(p<s.size()&&std::isspace((unsigned char)s[p]))++p;}
 char take(){if(p>=s.size())throw std::runtime_error("Unexpected end of JSON");return s[p++];}
 std::string str(){if(take()!='"')throw std::runtime_error("Expected JSON string");std::string o;while(p<s.size()){char c=take();if(c=='"')return o;if(c=='\\'){c=take();switch(c){case '"':case '\\':case '/':o+=c;break;case 'b':o+='\b';break;case 'f':o+='\f';break;case 'n':o+='\n';break;case 'r':o+='\r';break;case 't':o+='\t';break;case 'u':{if(p+4>s.size())throw std::runtime_error("Bad unicode escape");unsigned v=0;for(int i=0;i<4;i++){char h=s[p++];v=v*16+(h>='0'&&h<='9'?h-'0':(h|32)-'a'+10);}if(v<128)o+=char(v);else if(v<2048){o+=char(0xc0|(v>>6));o+=char(0x80|(v&63));}else{o+=char(0xe0|(v>>12));o+=char(0x80|((v>>6)&63));o+=char(0x80|(v&63));}break;}default:throw std::runtime_error("Bad JSON escape");}}else o+=c;}throw std::runtime_error("Unterminated JSON string");}
 MonkeyValue value(){ws();if(p>=s.size())throw std::runtime_error("Missing JSON value");char c=s[p];if(c=='"')return str();if(c=='{'){++p;auto o=std::make_shared<MonkeyObject>();ws();if(p<s.size()&&s[p]=='}'){++p;return o;}for(;;){ws();auto k=str();ws();if(take()!=':')throw std::runtime_error("Expected colon");(*o)[k]=value();ws();char d=take();if(d=='}')break;if(d!=',')throw std::runtime_error("Expected comma");}return o;}if(c=='['){++p;auto a=std::make_shared<MonkeyArray>();ws();if(p<s.size()&&s[p]==']'){++p;return a;}for(;;){a->push_back(value());ws();char d=take();if(d==']')break;if(d!=',')throw std::runtime_error("Expected comma");}return a;}if(s.compare(p,4,"true")==0){p+=4;return true;}if(s.compare(p,5,"false")==0){p+=5;return false;}if(s.compare(p,4,"null")==0){p+=4;return nullptr;}size_t start=p;while(p<s.size()&&(std::isdigit((unsigned char)s[p])||s[p]=='-'||s[p]=='+'||s[p]=='.'||s[p]=='e'||s[p]=='E'))++p;if(start==p)throw std::runtime_error("Invalid JSON value");auto n=s.substr(start,p-start);if(n.find_first_of(".eE")!=std::string::npos)return std::stod(n);long long i=std::stoll(n);if(i>=INT32_MIN&&i<=INT32_MAX)return int(i);return double(i);}
public:explicit Parser(const std::string& x):s(x){}MonkeyDoc doc(){auto v=value();ws();if(p!=s.size())throw std::runtime_error("Trailing JSON data");auto o=std::get_if<std::shared_ptr<MonkeyObject>>(&v);if(!o||!*o)throw std::runtime_error("JSON document must be an object");return **o;}
};
}
MonkeyCollection::MonkeyCollection(const std::string& p):filePath(p),lockPath(p+".lock"){refreshIndex();}
std::string MonkeyCollection::insertJson(const std::string& json){return insert(parseDoc(json));}
std::optional<std::string> MonkeyCollection::findByIdJson(const std::string& id)const{auto d=findById(id);if(!d)return {};return serializeDoc(*d);}
std::string MonkeyCollection::generateId()const{auto t=std::chrono::system_clock::now().time_since_epoch();uint64_t ts=std::chrono::duration_cast<std::chrono::milliseconds>(t).count();std::random_device rd;std::mt19937_64 g(rd());std::ostringstream s;s<<std::hex<<std::setfill('0')<<std::setw(12)<<ts<<std::setw(10)<<(g()&0xffffffffff);return s.str();}
int MonkeyCollection::acquireLock()const{int fd=open(lockPath.c_str(),O_CREAT|O_RDWR,0666);if(fd<0)return -1;if(flock(fd,LOCK_EX)<0){close(fd);return -1;}return fd;}
void MonkeyCollection::releaseLock(int fd)const{if(fd>=0){flock(fd,LOCK_UN);close(fd);}}
std::string MonkeyCollection::serializeValue(const MonkeyValue& v)const{return std::visit([this](const auto& x)->std::string{using T=std::decay_t<decltype(x)>;if constexpr(std::is_same_v<T,std::string>)return quote(x);else if constexpr(std::is_same_v<T,int>)return std::to_string(x);else if constexpr(std::is_same_v<T,double>){if(!std::isfinite(x))throw std::runtime_error("Non-finite JSON number");std::ostringstream o;o<<std::setprecision(17)<<x;return o.str();}else if constexpr(std::is_same_v<T,bool>)return x?"true":"false";else if constexpr(std::is_same_v<T,std::nullptr_t>)return "null";else if constexpr(std::is_same_v<T,std::shared_ptr<MonkeyArray>>){if(!x)return "null";std::string o="[";for(size_t i=0;i<x->size();++i){if(i)o+=",";o+=serializeValue((*x)[i]);}return o+"]";}else{if(!x)return "null";std::string o="{";bool first=true;for(auto&[k,val]:*x){if(!first)o+=",";first=false;o+=quote(k)+":"+serializeValue(val);}return o+"}";}},v);}
std::string MonkeyCollection::serializeDoc(const MonkeyDoc& d)const{return serializeValue(std::make_shared<MonkeyObject>(d));}
MonkeyValue MonkeyCollection::parseValue(const std::string& s)const{return std::make_shared<MonkeyObject>(Parser(s).doc());}
MonkeyDoc MonkeyCollection::parseDoc(const std::string& s)const{return Parser(s).doc();}
std::vector<MonkeyDoc> MonkeyCollection::readAll()const{std::ifstream f(filePath);std::vector<MonkeyDoc> v;std::string l;while(std::getline(f,l))if(!l.empty())v.push_back(parseDoc(l));return v;}
void MonkeyCollection::refreshIndex()const{std::lock_guard<std::mutex> g(mtx);std::error_code ec;auto size=std::filesystem::file_size(filePath,ec);if(ec)size=0;auto tm=std::filesystem::last_write_time(filePath,ec);auto mt=ec?0:tm.time_since_epoch().count();if(size==indexedSize&&mt==indexedMtime)return;idIndex.clear();std::ifstream f(filePath,std::ios::binary);std::string l;uint64_t off=0;while(std::getline(f,l)){if(!l.empty()){auto d=parseDoc(l);auto i=d.find("_id");if(i!=d.end())if(auto id=std::get_if<std::string>(&i->second))idIndex[*id]=off;}off+=l.size()+1;}indexedSize=size;indexedMtime=mt;}
void MonkeyCollection::writeAll(const std::vector<MonkeyDoc>& docs)const{auto tmp=filePath+".tmp."+generateId();int fd=open(tmp.c_str(),O_CREAT|O_EXCL|O_WRONLY,0666);if(fd<0)throw std::runtime_error("Cannot create temporary collection file");try{for(const auto&d:docs){auto line=serializeDoc(d)+"\n";const char*p=line.data();size_t n=line.size();while(n){ssize_t w=::write(fd,p,n);if(w<0)throw std::runtime_error("Cannot write temporary collection file");p+=w;n-=w;}}if(fsync(fd)<0)throw std::runtime_error("Cannot flush temporary collection file");if(close(fd)<0){fd=-1;throw std::runtime_error("Cannot close temporary collection file");}fd=-1;if(rename(tmp.c_str(),filePath.c_str())<0)throw std::runtime_error("Cannot atomically replace collection file");auto parent=std::filesystem::path(filePath).parent_path();if(parent.empty())parent=".";int dfd=open(parent.c_str(),O_RDONLY|O_DIRECTORY);if(dfd>=0){fsync(dfd);close(dfd);}indexedSize=static_cast<uintmax_t>(-1);indexedMtime=-1;}catch(...){if(fd>=0)close(fd);unlink(tmp.c_str());throw;}}
std::string MonkeyCollection::insert(MonkeyDoc d){std::lock_guard<std::mutex>g(mtx);int fd=acquireLock();if(fd<0)throw std::runtime_error("Cannot acquire collection lock");try{if(d.find("_id")==d.end())d["_id"]=generateId();auto p=std::get_if<std::string>(&d.at("_id"));if(!p)throw std::runtime_error("_id must be a string");auto docs=readAll();for(const auto& old:docs){auto i=old.find("_id");if(i!=old.end())if(auto existing=std::get_if<std::string>(&i->second);existing&&*existing==*p)throw std::runtime_error("Duplicate _id: "+*p);}std::string id=*p;docs.push_back(std::move(d));writeAll(docs);releaseLock(fd);return id;}catch(...){releaseLock(fd);throw;}}
std::optional<MonkeyDoc> MonkeyCollection::findById(const std::string&id)const{refreshIndex();uint64_t off;{std::lock_guard<std::mutex>g(mtx);auto i=idIndex.find(id);if(i==idIndex.end())return {};off=i->second;}std::ifstream f(filePath,std::ios::binary);if(!f)return {};f.seekg(off);std::string l;if(!std::getline(f,l))return {};auto d=parseDoc(l);return d;}
std::optional<MonkeyDoc> MonkeyCollection::findOne(const std::function<bool(const MonkeyDoc&)>&fn)const{for(auto&d:readAll())if(fn(d))return d;return {};}
std::vector<MonkeyDoc> MonkeyCollection::find(const std::function<bool(const MonkeyDoc&)>&fn)const{std::vector<MonkeyDoc>v;for(auto&d:readAll())if(fn(d))v.push_back(d);return v;}
std::vector<MonkeyDoc> MonkeyCollection::findAll()const{return readAll();}
bool MonkeyCollection::updateOne(const std::function<bool(const MonkeyDoc&)>&fn,const std::function<void(MonkeyDoc&)>&up){std::lock_guard<std::mutex>g(mtx);int fd=acquireLock();if(fd<0)throw std::runtime_error("Cannot acquire collection lock");try{auto v=readAll();for(auto&d:v)if(fn(d)){up(d);writeAll(v);releaseLock(fd);return true;}releaseLock(fd);return false;}catch(...){releaseLock(fd);throw;}}
bool MonkeyCollection::updateById(const std::string&id,const std::function<void(MonkeyDoc&)>&up){return updateOne([&](const MonkeyDoc&d){auto i=d.find("_id");auto s=i==d.end()?nullptr:std::get_if<std::string>(&i->second);return s&&*s==id;},up);}
bool MonkeyCollection::deleteOne(const std::function<bool(const MonkeyDoc&)>&fn){return deleteMany([&](const MonkeyDoc&d){return fn(d);})>0;}
bool MonkeyCollection::deleteById(const std::string&id){return deleteOne([&](const MonkeyDoc&d){auto i=d.find("_id");auto s=i==d.end()?nullptr:std::get_if<std::string>(&i->second);return s&&*s==id;});}
int MonkeyCollection::deleteMany(const std::function<bool(const MonkeyDoc&)>&fn){std::lock_guard<std::mutex>g(mtx);int fd=acquireLock();if(fd<0)throw std::runtime_error("Cannot acquire collection lock");try{auto v=readAll();auto old=v.size();v.erase(std::remove_if(v.begin(),v.end(),fn),v.end());auto n=int(old-v.size());if(n)writeAll(v);releaseLock(fd);return n;}catch(...){releaseLock(fd);throw;}}
int MonkeyCollection::count()const{return int(readAll().size());}
