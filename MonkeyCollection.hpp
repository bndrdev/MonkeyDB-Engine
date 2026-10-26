#ifndef MONKEY_COLLECTION_HPP
#define MONKEY_COLLECTION_HPP

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <unordered_map>
#include <variant>
#include <mutex>
#include <memory>
#include <cstdint>

struct MonkeyArray;
struct MonkeyObject;
using MonkeyValue = std::variant<std::string, int, double, bool, std::nullptr_t,
                                 std::shared_ptr<MonkeyArray>, std::shared_ptr<MonkeyObject>>;
struct MonkeyArray : std::vector<MonkeyValue> { using std::vector<MonkeyValue>::vector; };
struct MonkeyObject : std::unordered_map<std::string, MonkeyValue> {
    using std::unordered_map<std::string, MonkeyValue>::unordered_map;
};
using MonkeyDoc = MonkeyObject;

class MonkeyCollection {
private:
    std::string filePath;
    std::string lockPath;
    mutable std::mutex mtx;
    mutable std::unordered_map<std::string, std::uint64_t> idIndex;
    mutable std::uintmax_t indexedSize = static_cast<std::uintmax_t>(-1);
    mutable std::int64_t indexedMtime = -1;

    // Serialization
    std::string serializeDoc(const MonkeyDoc& doc) const;
    MonkeyDoc   parseDoc(const std::string& line) const;
    std::string serializeValue(const MonkeyValue& val) const;
    MonkeyValue parseValue(const std::string& raw) const;

    // File I/O
    std::vector<MonkeyDoc> readAll() const;
    void writeAll(const std::vector<MonkeyDoc>& docs) const;
    void refreshIndex() const;

    // File locking (cross-process)
    int  acquireLock() const;
    void releaseLock(int fd) const;

    // Auto ID generation
    std::string generateId() const;

public:
    explicit MonkeyCollection(const std::string& path);
    std::string insertJson(const std::string& json);
    std::optional<std::string> findByIdJson(const std::string& id) const;

    // Insert — auto-assigns "_id" if not provided; returns the assigned id
    std::string insert(MonkeyDoc doc);

    // Find by exact _id
    std::optional<MonkeyDoc> findById(const std::string& id) const;

    // Find first document matching filter
    std::optional<MonkeyDoc> findOne(const std::function<bool(const MonkeyDoc&)>& filter) const;

    // Find all documents matching filter
    std::vector<MonkeyDoc> find(const std::function<bool(const MonkeyDoc&)>& filter) const;

    // Find all documents
    std::vector<MonkeyDoc> findAll() const;

    // Update first matching document; returns true if found
    bool updateOne(
        const std::function<bool(const MonkeyDoc&)>& filter,
        const std::function<void(MonkeyDoc&)>& updater
    );

    // Update by _id; returns true if found
    bool updateById(const std::string& id, const std::function<void(MonkeyDoc&)>& updater);

    // Delete first matching document; returns true if found
    bool deleteOne(const std::function<bool(const MonkeyDoc&)>& filter);

    // Delete by _id; returns true if found
    bool deleteById(const std::string& id);

    // Delete all matching documents; returns count deleted
    int deleteMany(const std::function<bool(const MonkeyDoc&)>& filter);

    // Total document count
    int count() const;
};

#endif
