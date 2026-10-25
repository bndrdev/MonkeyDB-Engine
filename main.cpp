#include <iostream>
#include "MonkeyDB.hpp"

int main() {
    MonkeyDB db("monkeydb");
    auto users = db.getCollection("users");

    // Insert — _id assigned automatically
    std::string id1 = users.insert({{"name", std::string("Alice")}, {"age", 25}, {"active", true}});
    std::string id2 = users.insert({{"name", std::string("Bob")},   {"age", 30}, {"active", false}});
    std::string id3 = users.insert({{"name", std::string("Carol")}, {"age", 25}, {"active", true}});

    std::cout << "Inserted IDs:\n  " << id1 << "\n  " << id2 << "\n  " << id3 << "\n\n";

    // findById
    auto found = users.findById(id2);
    if (found)
        std::cout << "findById -> " << std::get<std::string>(found->at("name")) << "\n";

    // find by age
    auto age25 = users.find([](const MonkeyDoc& doc) {
        return std::get<int>(doc.at("age")) == 25;
    });
    std::cout << "Age 25 count: " << age25.size() << "\n";

    // updateById
    users.updateById(id2, [](MonkeyDoc& doc) {
        doc["active"] = true;
    });
    std::cout << "Updated Bob's active to true\n";

    // deleteById
    users.deleteById(id3);
    std::cout << "Deleted Carol\n";

    std::cout << "Total docs: " << users.count() << "\n";

    return 0;
}
