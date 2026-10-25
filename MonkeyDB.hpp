#ifndef MONKEY_DB_HPP
#define MONKEY_DB_HPP

#include <string>
#include <filesystem>
#include "MonkeyCollection.hpp"

class MonkeyDB {
private:
    std::string dbPath;

public:
    explicit MonkeyDB(const std::string& dbName);
    MonkeyCollection getCollection(const std::string& name);
};

#endif
