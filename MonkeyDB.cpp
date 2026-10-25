#include "MonkeyDB.hpp"
#include <filesystem>
#include <stdexcept>

MonkeyDB::MonkeyDB(const std::string& dbName) {
    dbPath = "./" + dbName;
    if (!std::filesystem::exists(dbPath)) {
        if (!std::filesystem::create_directories(dbPath)) {
            throw std::runtime_error("Failed to create database directory: " + dbPath);
        }
    }
}

MonkeyCollection MonkeyDB::getCollection(const std::string& name) {
    return MonkeyCollection(dbPath + "/" + name + ".jsonl");
}
