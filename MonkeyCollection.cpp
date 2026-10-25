#include "MonkeyCollection.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <random>
#include <iomanip>

// POSIX file locking
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>

// ─── Constructor ─────────────────────────────────────────────────────────────

MonkeyCollection::MonkeyCollection(const std::string& path)
    : filePath(path), lockPath(path + ".lock") {}

// ─── Auto ID ─────────────────────────────────────────────────────────────────

std::string MonkeyCollection::generateId() const {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    uint64_t ts = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t rnd = dist(gen);

    std::ostringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(12) << ts
       << std::setw(10) << (rnd & 0xFFFFFFFFFF);
    return ss.str();
}

// ─── File Locking ─────────────────────────────────────────────────────────────

int MonkeyCollection::acquireLock() const {
    int fd = open(lockPath.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd == -1) return -1;
    flock(fd, LOCK_EX);
    return fd;
}

void MonkeyCollection::releaseLock(int fd) const {
    if (fd != -1) {
        flock(fd, LOCK_UN);
        close(fd);
    }
}

// ─── Serialization ───────────────────────────────────────────────────────────

std::string MonkeyCollection::serializeValue(const MonkeyValue& val) const {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>)
            return "\"" + v + "\"";
        else if constexpr (std::is_same_v<T, int>)
            return std::to_string(v);
        else if constexpr (std::is_same_v<T, double>)
            return std::to_string(v);
        else if constexpr (std::is_same_v<T, bool>)
            return v ? "true" : "false";
        else
            return "null";
    }, val);
}

std::string MonkeyCollection::serializeDoc(const MonkeyDoc& doc) const {
    std::string out = "{";
    bool first = true;
    for (const auto& [key, val] : doc) {
        if (!first) out += ",";
        out += "\"" + key + "\":" + serializeValue(val);
        first = false;
    }
    out += "}";
    return out;
}

MonkeyValue MonkeyCollection::parseValue(const std::string& raw) const {
    if (raw.empty()) return nullptr;
    if (raw == "null")  return nullptr;
    if (raw == "true")  return true;
    if (raw == "false") return false;
    if (raw.front() == '"' && raw.back() == '"')
        return raw.substr(1, raw.size() - 2);
    if (raw.find('.') != std::string::npos) {
        try { return std::stod(raw); } catch (...) {}
    }
    try { return std::stoi(raw); } catch (...) {}
    return raw;
}

MonkeyDoc MonkeyCollection::parseDoc(const std::string& line) const {
    MonkeyDoc doc;
    if (line.size() < 2) return doc;

    std::string content = line.substr(1, line.size() - 2);
    std::size_t pos = 0;

    while (pos < content.size()) {
        while (pos < content.size() && std::isspace(content[pos])) ++pos;
        if (pos >= content.size()) break;

        if (content[pos] != '"') break;
        ++pos;
        std::string key;
        while (pos < content.size() && content[pos] != '"') key += content[pos++];
        ++pos;

        while (pos < content.size() && content[pos] != ':') ++pos;
        ++pos;
        while (pos < content.size() && std::isspace(content[pos])) ++pos;

        std::string rawVal;
        if (content[pos] == '"') {
            ++pos;
            while (pos < content.size() && content[pos] != '"') rawVal += content[pos++];
            ++pos;
            doc[key] = rawVal;
        } else {
            while (pos < content.size() && content[pos] != ',' && content[pos] != '}')
                rawVal += content[pos++];
            doc[key] = parseValue(rawVal);
        }

        while (pos < content.size() && (content[pos] == ',' || std::isspace(content[pos]))) ++pos;
    }

    return doc;
}

// ─── File I/O ────────────────────────────────────────────────────────────────

std::vector<MonkeyDoc> MonkeyCollection::readAll() const {
    std::vector<MonkeyDoc> docs;
    std::ifstream file(filePath);
    if (!file.is_open()) return docs;

    std::string line;
    while (std::getline(file, line))
        if (!line.empty())
            docs.push_back(parseDoc(line));
    return docs;
}

void MonkeyCollection::writeAll(const std::vector<MonkeyDoc>& docs) const {
    std::ofstream file(filePath, std::ios::trunc);
    if (!file.is_open())
        throw std::runtime_error("Cannot write to collection: " + filePath);
    for (const auto& doc : docs)
        file << serializeDoc(doc) << "\n";
}

// ─── Public API ──────────────────────────────────────────────────────────────

std::string MonkeyCollection::insert(MonkeyDoc doc) {
    std::lock_guard<std::mutex> guard(mtx);
    int fd = acquireLock();

    if (doc.find("_id") == doc.end())
        doc["_id"] = generateId();

    std::string id = std::get<std::string>(doc.at("_id"));

    std::ofstream file(filePath, std::ios::app);
    if (!file.is_open()) { releaseLock(fd); throw std::runtime_error("Cannot open collection: " + filePath); }
    file << serializeDoc(doc) << "\n";

    releaseLock(fd);
    return id;
}

std::optional<MonkeyDoc> MonkeyCollection::findById(const std::string& id) const {
    return findOne([&id](const MonkeyDoc& doc) {
        auto it = doc.find("_id");
        if (it == doc.end()) return false;
        const auto* s = std::get_if<std::string>(&it->second);
        return s && *s == id;
    });
}

std::optional<MonkeyDoc> MonkeyCollection::findOne(
    const std::function<bool(const MonkeyDoc&)>& filter) const
{
    std::ifstream file(filePath);
    if (!file.is_open()) return std::nullopt;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        MonkeyDoc doc = parseDoc(line);
        if (filter(doc)) return doc;
    }
    return std::nullopt;
}

std::vector<MonkeyDoc> MonkeyCollection::find(
    const std::function<bool(const MonkeyDoc&)>& filter) const
{
    std::vector<MonkeyDoc> results;
    std::ifstream file(filePath);
    if (!file.is_open()) return results;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        MonkeyDoc doc = parseDoc(line);
        if (filter(doc)) results.push_back(std::move(doc));
    }
    return results;
}

std::vector<MonkeyDoc> MonkeyCollection::findAll() const {
    return readAll();
}

bool MonkeyCollection::updateOne(
    const std::function<bool(const MonkeyDoc&)>& filter,
    const std::function<void(MonkeyDoc&)>& updater)
{
    std::lock_guard<std::mutex> guard(mtx);
    int fd = acquireLock();

    auto docs = readAll();
    for (auto& doc : docs) {
        if (filter(doc)) {
            updater(doc);
            writeAll(docs);
            releaseLock(fd);
            return true;
        }
    }
    releaseLock(fd);
    return false;
}

bool MonkeyCollection::updateById(
    const std::string& id,
    const std::function<void(MonkeyDoc&)>& updater)
{
    return updateOne([&id](const MonkeyDoc& doc) {
        auto it = doc.find("_id");
        if (it == doc.end()) return false;
        const auto* s = std::get_if<std::string>(&it->second);
        return s && *s == id;
    }, updater);
}

bool MonkeyCollection::deleteOne(const std::function<bool(const MonkeyDoc&)>& filter) {
    std::lock_guard<std::mutex> guard(mtx);
    int fd = acquireLock();

    auto docs = readAll();
    auto it = std::find_if(docs.begin(), docs.end(), filter);
    if (it == docs.end()) { releaseLock(fd); return false; }
    docs.erase(it);
    writeAll(docs);
    releaseLock(fd);
    return true;
}

bool MonkeyCollection::deleteById(const std::string& id) {
    return deleteOne([&id](const MonkeyDoc& doc) {
        auto it = doc.find("_id");
        if (it == doc.end()) return false;
        const auto* s = std::get_if<std::string>(&it->second);
        return s && *s == id;
    });
}

int MonkeyCollection::deleteMany(const std::function<bool(const MonkeyDoc&)>& filter) {
    std::lock_guard<std::mutex> guard(mtx);
    int fd = acquireLock();

    auto docs = readAll();
    int before = static_cast<int>(docs.size());
    docs.erase(std::remove_if(docs.begin(), docs.end(), filter), docs.end());
    int deleted = before - static_cast<int>(docs.size());
    if (deleted > 0) writeAll(docs);
    releaseLock(fd);
    return deleted;
}

int MonkeyCollection::count() const {
    int n = 0;
    std::ifstream file(filePath);
    if (!file.is_open()) return 0;
    std::string line;
    while (std::getline(file, line))
        if (!line.empty()) ++n;
    return n;
}
