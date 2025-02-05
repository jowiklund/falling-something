#include "falling.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

World *world_create(arena *a, world_config cfg) {
	World *wld = new(a, World);
	if (!wld) return NULL;

	wld->chunks = new(a, Chunk, cfg.width * cfg.height);
	if (!wld->chunks) return NULL;

	for (int i = 0; i < cfg.width * cfg.height; i++) {
		wld->chunks[i] = *chunk_create(a, cfg.chunk_size);
	}

	wld->chunk_size = cfg.chunk_size;
	wld->width = cfg.width;
	wld->height = cfg.height;
	wld->arena = a;
	return wld;
}

int load_chunk(arena *a, Chunk *chunk) {
	chunk->data = buffer_create(a, chunk->size, chunk->size);
	if (!chunk->data) return 0;

	chunk->simulated = 1;
	chunk->rendered = 1;

	return 1;
}

int get_chunk_index(World *w, int16_t chunk_x, int16_t chunk_y) {
	assert(chunk_x < w->width);
	assert(chunk_y < w->height);
	return chunk_y * w->width + chunk_x;
}

Chunk *get_chunk_at(World *w, int16_t chunk_x, int16_t chunk_y) {
	int chunk_index = get_chunk_index(w, chunk_x, chunk_y);
	return &w->chunks[chunk_index];
}

Chunk *chunk_create(arena *a, uint8_t size) {
	Chunk *c = new(a, Chunk);
	if (!c) return NULL;
	c->rendered = 0;
	c->data = NULL;
	c->size = size;

	return c;
}

Buffer *buffer_create(arena *a, int64_t rows, int64_t cols) {
	Buffer *b = new(a, Buffer);
	if (!b) {
		printf("Failed to allocate buffer");
		abort();
	}

	b->rows = new(a, row, rows);
	if (!b->rows) {
		printf("Failed to allocate rows");
		abort();
	}

	for (int64_t i = 0; i < rows; i++) {
		b->rows[i].cells = new(a, char, cols);
		memset(b->rows[i].cells, 0, cols);
		if (!b->rows[i].cells) {
			printf("Failed to allocate cells at %ld\n", i);
			abort();
		}
	}
	return b;
}
