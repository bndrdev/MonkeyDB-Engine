# MonkeyDB

A lightweight, embedded JSONL document database written in **C++17**.

The C++ core and C ABI require no third-party libraries. The implementation uses POSIX file APIs, so it currently targets Linux and other POSIX systems. Optional language bindings have separate requirements; see [the bindings guide](bindings/README.md).

---

## Features

- **Embedded** — runs directly inside your app, no separate server needed
- **Persistent storage** — data saved to `.jsonl` files on disk
- **Lambda-based filtering** — flexible queries using C++ lambdas
- **Auto-generated IDs** — every document gets a unique `_id` automatically
- **Indexed ID lookup** — in-memory `_id` to byte-offset index
- **Nested JSON values** — documents and arrays can contain nested values
- **Crash-safer mutations** — synced temporary file and atomic replacement
- **C ABI** — JSON interface for foreign-function bindings
- **No third-party dependencies in the core** — uses C++17 and POSIX APIs

---

## Project Structure

```
MonkeyDB/
├── MonkeyDB.hpp          # Database class — manages directory & collections
├── MonkeyDB.cpp
├── MonkeyCollection.hpp  # Collection class — all CRUD operations
├── MonkeyCollection.cpp
├── MonkeyDB_C.h          # C ABI for language bindings
├── MonkeyDB_C.cpp
├── bindings/             # Python, Node.js, Go, Rust, Java and C# bindings
└── main.cpp              # Usage example
```

---

## Build

```bash
g++ -std=c++17 -pthread -o myapp main.cpp MonkeyDB.cpp MonkeyCollection.cpp
./myapp
```

Build the C ABI shared library on Linux:
```bash
mkdir -p build
g++ -std=c++17 -pthread -fPIC -shared -o build/libmonkeydb.so MonkeyCollection.cpp MonkeyDB_C.cpp
```

The C++ API uses no third-party libraries. Binding requirements vary: Python uses `ctypes`; Node.js uses Koffi; Go uses cgo and a C toolchain; Rust uses `serde` and `serde_json`; Java uses JNA and Gson; C# uses .NET P/Invoke. Setup instructions are in [`bindings/README.md`](bindings/README.md).

---

## Quick Start

```cpp
#include "MonkeyDB.hpp"

int main() {
    MonkeyDB db("myapp_data");               // creates ./myapp_data/ directory
    auto users = db.getCollection("users");  // creates ./myapp_data/users.jsonl

    // Insert
    std::string id = users.insert({
        {"name",   std::string("Ahmed")},
        {"age",    22},
        {"active", true}
    });

    // Find by ID
    auto user = users.findById(id);

    // Find with filter
    auto adults = users.find([](const MonkeyDoc& doc) {
        return std::get<int>(doc.at("age")) >= 18;
    });

    // Update by ID
    users.updateById(id, [](MonkeyDoc& doc) {
        doc["age"] = 23;
    });

    // Delete by ID
    users.deleteById(id);
}
```

---

## API Reference

### MonkeyDB

| Method | Description |
|--------|-------------|
| `MonkeyDB(name)` | Creates or opens a database directory |
| `getCollection(name)` | Returns a collection; the data file is created on first insert |

---

### MonkeyCollection

#### Insert

```cpp
std::string id = col.insert(MonkeyDoc doc);
```
Inserts a document. Auto-assigns `_id` if not provided. Returns the `_id`.

---

#### Find

```cpp
// By ID
std::optional<MonkeyDoc> doc = col.findById(std::string id);

// First match
std::optional<MonkeyDoc> doc = col.findOne([](const MonkeyDoc& d) {
    return std::get<std::string>(d.at("name")) == "Ahmed";
});

// All matches
std::vector<MonkeyDoc> docs = col.find([](const MonkeyDoc& d) {
    return std::get<int>(d.at("age")) > 20;
});

// All documents
std::vector<MonkeyDoc> all = col.findAll();
```

---

#### Update

```cpp
// By ID
col.updateById(id, [](MonkeyDoc& doc) {
    doc["age"] = 25;
});

// By filter
col.updateOne(
    [](const MonkeyDoc& doc) { return std::get<std::string>(doc.at("name")) == "Ahmed"; },
    [](MonkeyDoc& doc)       { doc["active"] = false; }
);
```

---

#### Delete

```cpp
col.deleteById(id);
col.deleteOne([](const MonkeyDoc& d) { return ...; });
int n = col.deleteMany([](const MonkeyDoc& d) { return ...; });
```

---

#### Count

```cpp
int total = col.count();
```

---

## Supported Value Types

| C++ Type | Example |
|----------|---------|
| `std::string` | `{"name", std::string("Ahmed")}` |
| `int` | `{"age", 22}` |
| `double` | `{"score", 9.5}` |
| `bool` | `{"active", true}` |
| `nullptr_t` | `{"deleted", nullptr}` |
| `std::shared_ptr<MonkeyObject>` | nested object |
| `std::shared_ptr<MonkeyArray>` | array of values, including nested values |

---

## Storage Format

Each collection is a `.jsonl` file — one JSON document per line:

```
myapp_data/
└── users.jsonl
```

```jsonl
{"_id":"01a172e037bab0b55adc7d","name":"Ahmed","age":22,"active":true}
{"_id":"01a172e037bb018b4b1e3a","name":"Sara","age":25,"active":true}
```

---

## Limitations

- ID lookups use an in-memory index; filter-based queries still scan the collection.
- Mutations rewrite the collection through a temporary file, which favors safer replacement over write throughput on large files.
- The ID index is rebuilt in memory when the collection file changes; it is not stored on disk.
- Uses POSIX file APIs; Windows is not currently supported.
- Intended for single-machine, small-to-medium workloads rather than distributed or high-write-throughput systems.

---

## When to Use MonkeyDB

| Good for | Not for |
|----------|---------|
| Desktop apps | Web apps with many concurrent users |
| CLI tools needing persistence | Large datasets |
| Personal / portfolio projects | Production at scale |
| Learning embedded databases | Distributed systems |

---

## License

MIT License — free to use, modify, and distribute.
