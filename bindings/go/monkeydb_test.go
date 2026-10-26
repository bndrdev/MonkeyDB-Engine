package monkeydb

import (
	"testing"
)

func TestCollectionCRUD(t *testing.T) {
	collection, err := Open(t.TempDir() + "/users.jsonl")
	if err != nil { t.Fatal(err) }
	defer collection.Close()

	id, err := collection.Insert(map[string]any{"name": "Ada", "nested": map[string]any{"enabled": true}})
	if err != nil { t.Fatal(err) }
	found, ok, err := collection.FindByID(id)
	if err != nil { t.Fatal(err) }
	if !ok || found["name"] != "Ada" { t.Fatalf("unexpected document: %#v", found) }
	if n, err := collection.Count(); err != nil || n != 1 { t.Fatalf("count=%d err=%v", n, err) }
	if deleted, err := collection.DeleteByID(id); err != nil || !deleted { t.Fatalf("deleted=%v err=%v", deleted, err) }
	if _, ok, err := collection.FindByID(id); err != nil || ok { t.Fatalf("expected deleted record, ok=%v err=%v", ok, err) }
}
