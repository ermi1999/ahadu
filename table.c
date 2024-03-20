#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

/**
  * initTable - initializes a table.
  * @table: the table to initialize.
  * Return: nothing.
  */
void initTable(Table *table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

/**
  * freeTable - frees a table.
  * @table: the table to free.
  * Return: nothing.
  */
void freeTable(Table *table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

/**
  * findEntry - finds an entry in a table.
  * @entries: the entries to search.
  * @capacity: the capacity of the table.
  * @key: the key to search for.
  * Return: the entry.
  */
static Entry *findEntry(Entry *entries, int capacity, ObjString *key) {
  uint32_t index = key->hash % capacity;
  Entry *tombstone = NULL;

  for (;;) {
    Entry *entry = &entries[index];

    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // Empty entry.
        return tombstone != NULL ? tombstone : entry;
      } else {
        // Found tombstone.
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      // found the key.
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

/**
  * tableGet - gets a value from a table.
  * @table: the table to get the value from.
  * @key: the key to search for.
  * @value: the value to get.
  * Return: true if the value is found, false otherwise.
  */
bool tableGet(Table *table, ObjString *key, Value *value) {
  if (table->entries == NULL) return false;

  Entry *entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  *value = entry->value;
  return true;
}

/**
  * adjustCapacity - adjusts the capacity of a table.
  * @table: the table to adjust.
  * @capacity: the new capacity.
  * Return: nothing.
  */
static void adjustCapacity(Table *table, int capacity) {
  Entry *entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry *entry = &table->entries[i];
    if (entry->key == NULL) continue;

    Entry *dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    table->count++;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);

  table->entries = entries;
  table->capacity = capacity;
}

/**
  * tableSet - sets a value in a table.
  * @table: the table to set the value in.
  * @key: the key to set.
  * @value: the value to set.
  * Return: true if the key is new, false otherwise.
  */
bool tableSet(Table *table, ObjString *key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }

  Entry *entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = entry->key == NULL;
  if (isNewKey && IS_NIL(entry->value)) table->count++;

  entry->key = key;
  entry->value = value;
  return isNewKey;
}

/**
  * tableDelete - deletes a value from a table.
  * @table: the table to delete the value from.
  * @key: the key to delete.
  * Return: true if the value is deleted, false otherwise.
  */
bool tableDelete(Table *table, ObjString *key) {
  if (table->count == 0) return false;

  // Find the entry.
  Entry *entry = findEntry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  // Place a tombstone in the entry.
  entry->key = NULL;
  entry->value = BOOL_VAL(true);

  return true;
}

/**
  * tableAddAll - adds all entries from one table to another.
  * @from: the table to add from.
  * @to: the table to add to.
  * Return: nothing.
  */
void tableAddAll(Table *from, Table *to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry *entry = &from->entries[i];
    if (entry->key != NULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}

/**
  * tableFindString - finds a string in a table.
  * @table: the table to search.
  * @chars: the characters to search for.
  * @length: the length of the characters.
  * @hash: the hash of the characters.
  * Return: the string if found, NULL otherwise.
  */
ObjString *tableFindString(Table *table, const wchar_t *chars, int length, uint32_t hash) {
  if (table->count == 0) return NULL;

  uint32_t index = hash % table->capacity;

  for (;;) {
    Entry *entry = &table->entries[index];

    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        // Empty entry.
        return NULL;
      }
    } else if (entry->key->length == length &&
               entry->key->hash == hash &&
               memcmp(entry->key->chars, chars, length) == 0) {
      // Found it.
      return entry->key;
    }

    index = (index + 1) % table->capacity;
  }
}
