const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { test } = require('node:test');
const { Collection } = require('./index');

test('collection CRUD and nested JSON', () => {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'monkeydb-node-'));
  const collection = new Collection(path.join(directory, 'users.jsonl'));
  try {
    const id = collection.insert({ name: 'Ada', profile: { tags: ['js', 'ffi'] } });
    assert.deepEqual(collection.findById(id).profile.tags, ['js', 'ffi']);
    assert.equal(collection.findById('missing'), null);
    assert.equal(collection.count(), 1);
    assert.equal(collection.deleteById(id), true);
    assert.equal(collection.count(), 0);
  } finally {
    collection.close();
    fs.rmSync(directory, { recursive: true, force: true });
  }
});
