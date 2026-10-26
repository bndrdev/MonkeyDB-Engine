using System;
using System.Runtime.InteropServices;
using System.Text.Json;

namespace MonkeyDB;

public sealed class Collection : IDisposable
{
    private const string Library = "monkeydb";
    private IntPtr _handle;

    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr monkeydb_collection_open([MarshalAs(UnmanagedType.LPUTF8Str)] string path);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    private static extern void monkeydb_collection_close(IntPtr handle);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr monkeydb_insert_json(IntPtr handle, [MarshalAs(UnmanagedType.LPUTF8Str)] string json);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr monkeydb_find_by_id_json(IntPtr handle, [MarshalAs(UnmanagedType.LPUTF8Str)] string id);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    private static extern int monkeydb_delete_by_id(IntPtr handle, [MarshalAs(UnmanagedType.LPUTF8Str)] string id);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    private static extern int monkeydb_count(IntPtr handle);
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr monkeydb_last_error();
    [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
    private static extern void monkeydb_free(IntPtr memory);

    private static string LastError() => Marshal.PtrToStringUTF8(monkeydb_last_error()) ?? "MonkeyDB operation failed";
    private static string? TakeString(IntPtr value)
    {
        if (value == IntPtr.Zero) return null;
        try { return Marshal.PtrToStringUTF8(value); }
        finally { monkeydb_free(value); }
    }
    public Collection(string path)
    {
        _handle = monkeydb_collection_open(path);
        if (_handle == IntPtr.Zero) throw new InvalidOperationException(LastError());
    }
    public string Insert<T>(T document)
    {
        string json = JsonSerializer.Serialize(document);
        string id = TakeString(monkeydb_insert_json(_handle, json))
            ?? throw new InvalidOperationException(LastError());
        return id;
    }
    public JsonDocument? FindById(string id)
    {
        string json = TakeString(monkeydb_find_by_id_json(_handle, id));
        if (json == null) { string error = LastError(); if (error.Length != 0) throw new InvalidOperationException(error); return null; }
        return JsonDocument.Parse(json);
    }
    public bool DeleteById(string id)
    {
        int result = monkeydb_delete_by_id(_handle, id);
        if (result < 0) throw new InvalidOperationException(LastError());
        return result == 1;
    }
    public int Count()
    {
        int result = monkeydb_count(_handle);
        if (result < 0) throw new InvalidOperationException(LastError());
        return result;
    }
    public void Dispose()
    {
        if (_handle != IntPtr.Zero) { monkeydb_collection_close(_handle); _handle = IntPtr.Zero; }
        GC.SuppressFinalize(this);
    }
}
