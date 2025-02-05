#include "arena.h"
#include <stdint.h>
#include <string.h>

// MAPS

typedef struct map_entry {
	const char *key;
	void *value;
	struct map_entry *next;
} map_entry;

typedef struct {
	map_entry *entries;
	size_t cap;
	size_t len;
	arena *arena;
} map;

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

static uint64_t hash_key(const char* key) {
	uint64_t hash = FNV_OFFSET;
	for (const char* p = key; *p; p++) {
		hash ^= (uint64_t)(unsigned char)(*p);
		hash *= FNV_PRIME;
	}
	return hash;
}

void map_insert_next(map *m, map_entry *e, char *key, void *val) {
	map_entry *current = e;
	while(current != NULL) {
		if (!strcmp(current->key, key)) {
			current->value = &val;
			return;
		}
		if (!current->next) {
			map_entry *ne = new(m->arena, map_entry);
			ne->key = key;
			ne->value = val;
			current->next = ne;
			return;
		}
		current = current->next;
	}
}

map_entry *map_get_next(map *m, map_entry *e, char *key) {
	map_entry *current = e;
	while(current != NULL) {
		if (!strcmp(current->key, key)) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

map *map_new(arena *a, int size) {
	map *m = new(a, map);
	if (!m) return NULL;

	m->entries = new(a, map_entry);
	if (!m->entries) return NULL;

	for (int i = 0; i < size; i++) {
		m->entries[i] = (map_entry){0};
	}

	m->arena = a;

	m->cap = size;
	return m;
}

void map_set(map *m, char *key, void *val) {
	uint64_t index = hash_key(key) % m->cap;
	if (m->entries[index].key != NULL) {
		map_insert_next(m, &m->entries[index], key, val);
	}
	map_entry *e = new(m->arena, map_entry);
	e->key = key;
	e->value = val;
	m->entries[index] = *e;
	m->len++;
}

map_entry *map_get(map *m, char *key) {
	uint64_t index = hash_key(key) % m->cap;
	map_entry *e = &m->entries[index];
	if (e == NULL) {
		return NULL;
	}
	return map_get_next(m, e, key);
}
