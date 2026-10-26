package monkeydb;

import com.sun.jna.Library;
import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import java.nio.charset.StandardCharsets;
import java.util.Map;

public final class MonkeyDB implements AutoCloseable {
    private interface Api extends Library {
        Pointer monkeydb_collection_open(Pointer path);
        void monkeydb_collection_close(Pointer handle);
        Pointer monkeydb_insert_json(Pointer handle, Pointer json);
        Pointer monkeydb_find_by_id_json(Pointer handle, Pointer id);
        int monkeydb_delete_by_id(Pointer handle, Pointer id);
        int monkeydb_count(Pointer handle);
        Pointer monkeydb_last_error();
        void monkeydb_free(Pointer memory);
    }
    private static final Api API = Native.load(
        System.getenv().getOrDefault("MONKEYDB_LIBRARY", "monkeydb"), Api.class);
    private Pointer handle;
    private static Memory utf8(String value) {
        byte[] bytes = (value + "\0").getBytes(StandardCharsets.UTF_8);
        Memory memory = new Memory(bytes.length);
        memory.write(0, bytes, 0, bytes.length);
        return memory;
    }

    public MonkeyDB(String filePath) {
        handle = API.monkeydb_collection_open(utf8(filePath));
        if (handle == null) throw new IllegalStateException(error());
    }
    private static String error() {
        Pointer p = API.monkeydb_last_error();
        return p == null ? "MonkeyDB operation failed" : p.getString(0, "UTF-8");
    }
    private static String takeString(Pointer p) {
        if (p == null) return null;
        try { return p.getString(0, "UTF-8"); }
        finally { API.monkeydb_free(p); }
    }
    public String insert(Map<String, ?> document) {
        String json = new com.google.gson.Gson().toJson(document);
        String id = takeString(API.monkeydb_insert_json(handle, utf8(json)));
        if (id == null) throw new IllegalArgumentException(error());
        return id;
    }
    public String findById(String id) {
        String json = takeString(API.monkeydb_find_by_id_json(handle, utf8(id)));
        if (json == null) { String e = error(); if (!e.isEmpty()) throw new IllegalStateException(e); }
        return json;
    }
    public boolean deleteById(String id) {
        int result = API.monkeydb_delete_by_id(handle, utf8(id));
        if (result < 0) throw new IllegalStateException(error());
        return result == 1;
    }
    public int count() {
        int result = API.monkeydb_count(handle);
        if (result < 0) throw new IllegalStateException(error());
        return result;
    }
    @Override public void close() { if (handle != null) { API.monkeydb_collection_close(handle); handle = null; } }
}
