# MonkeyDB

A lightweight, embedded NoSQL document database written in **C++17**.  
No external dependencies. No server. Just pure C++.

---

## Features

- **Embedded** — runs directly inside your app, no separate server needed
- **Persistent storage** — data saved to `.jsonl` files on disk
- **Lambda-based filtering** — flexible queries using C++ lambdas
- **Auto-generated IDs** — every document gets a unique `_id` automatically
- **Thread-safe** — mutex + file locking for concurrent access
- **Zero dependencies** — pure C++17 standard library only

---

## Project Structure

```
MonkeyDB/
├── MonkeyDB.hpp          # Database class — manages directory & collections
├── MonkeyDB.cpp
├── MonkeyCollection.hpp  # Collection class — all CRUD operations
├── MonkeyCollection.cpp
└── main.cpp              # Usage example
```

---

## Build

```bash
g++ -std=c++17 -o myapp main.cpp MonkeyDB.cpp MonkeyCollection.cpp
./myapp
```

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
| `getCollection(name)` | Returns a collection (creates file if needed) |

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

- No nested objects or arrays
- Full file scan on every query (no indexing)
- Not recommended for large datasets (10k+ documents)
- Designed for single-machine use only

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
