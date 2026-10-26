using System.Text.Json;
using MonkeyDB;

var path = Path.Combine(Path.GetTempPath(), $"monkeydb-dotnet-{Guid.NewGuid():N}.jsonl");
using (var collection = new Collection(path))
{
    var id = collection.Insert(new { name = "Ada", tags = new[] { "dotnet", "ffi" } });
    using var found = collection.FindById(id) ?? throw new Exception("record not found");
    if (found.RootElement.GetProperty("name").GetString() != "Ada") throw new Exception("unexpected record");
    if (collection.FindById("missing") is not null) throw new Exception("missing record was returned");
    if (collection.Count() != 1 || !collection.DeleteById(id) || collection.Count() != 0)
        throw new Exception("CRUD check failed");
}
File.Delete(path);
File.Delete(path + ".lock");
Console.WriteLine(".NET binding OK");
