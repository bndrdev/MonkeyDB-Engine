# Language bindings

MonkeyDB exposes a C ABI in `MonkeyDB_C.h`. These bindings call the native C++ shared library and exchange JSON documents.

Build the native library from the repository root:

```sh
mkdir -p build
g++ -std=c++17 -O2 -fPIC -shared -o build/libmonkeydb.so MonkeyCollection.cpp MonkeyDB_C.cpp
```

Set `MONKEYDB_LIBRARY` to the full library path for Python and Node.js. On Linux, set `LD_LIBRARY_PATH` to the library directory for Go, Rust, Java and .NET.

The current storage implementation uses POSIX file locking and filesystem APIs. These bindings are intended for platforms where the native library builds; Windows support requires replacing that platform-specific storage layer.

| Language | Binding | Additional setup |
|---|---|---|
| Python | `python/monkeydb.py` using `ctypes` | Python standard library |
| Node.js | `node/index.js` using Koffi | `cd bindings/node && npm install` |
| Go | `go/monkeydb.go` using cgo | cgo and a C toolchain |
| Rust | `rust/` using FFI and `serde_json` | `cargo`; set `MONKEYDB_LIB_DIR` if needed |
| Java | `java/` using JNA and Gson | Java 17 and Maven |
| C# | `csharp/` using P/Invoke | .NET 8 |
| C/C++ | `MonkeyDB_C.h` / C++ headers | C/C++ compiler |

Each binding currently exposes open/close, insert, find by ID, delete by ID and count. Filter callbacks and update operations remain available only in the C++ API. Returned native strings are copied by the wrappers and released with `monkeydb_free`.

Python usage:

```python
from bindings.python.monkeydb import Collection

with Collection("data/users.jsonl") as users:
    user_id = users.insert({"name": "Ada", "roles": ["admin"]})
    user = users.find_by_id(user_id)
```

Node.js usage:

```js
const { Collection } = require('./bindings/node');
const users = new Collection('data/users.jsonl');
const id = users.insert({ name: 'Ada', roles: ['admin'] });
const user = users.findById(id);
users.close();
```

Build and run the binding checks from the repository root:

```sh
PYTHONPATH=bindings/python MONKEYDB_LIBRARY="$PWD/build/libmonkeydb.so" python3 -m unittest discover -s bindings/python
MONKEYDB_LIBRARY="$PWD/build/libmonkeydb.so" npm test --prefix bindings/node
LD_LIBRARY_PATH="$PWD/build" go test ./bindings/go
MONKEYDB_LIB_DIR="$PWD/build" LD_LIBRARY_PATH="$PWD/build" cargo test --manifest-path bindings/rust/Cargo.toml
MONKEYDB_LIBRARY="$PWD/build/libmonkeydb.so" LD_LIBRARY_PATH="$PWD/build" mvn -f bindings/java/pom.xml package
LD_LIBRARY_PATH="$PWD/build" dotnet run --project bindings/csharp-smoke/Smoke.csproj
```
