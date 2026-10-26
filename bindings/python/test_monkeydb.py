import tempfile
import unittest
from pathlib import Path
from monkeydb import Collection


class CollectionTest(unittest.TestCase):
    def test_crud_and_nested_json(self):
        with tempfile.TemporaryDirectory() as directory:
            with Collection(Path(directory) / "users.jsonl") as collection:
                key = collection.insert({"name": "Ada", "profile": {"tags": ["cpp", "ffi"]}})
                self.assertEqual(collection.find_by_id(key)["profile"]["tags"], ["cpp", "ffi"])
                self.assertIsNone(collection.find_by_id("missing"))
                self.assertEqual(collection.count(), 1)
                self.assertTrue(collection.delete_by_id(key))
                self.assertEqual(collection.count(), 0)


if __name__ == "__main__":
    unittest.main()
