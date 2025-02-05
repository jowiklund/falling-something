#ifndef W_FALLING
#define W_FALLING

// Buffer
#include "arena.h"
#include <raylib.h>
#include <stdint.h>
typedef struct {
	char *cells;
} row;

typedef struct {
	row *rows;
} Buffer;

Buffer *buffer_create(arena *a, int64_t rows, int64_t cols);

typedef struct {
	Buffer *data;
	uint8_t rendered;
	uint8_t simulated;
	uint8_t size;
} Chunk;

Chunk *chunk_create(arena *a, uint8_t size);

typedef struct {
	uint8_t chunk_size;
	int16_t width;
	int16_t height;
} world_config;

typedef struct {
	arena *arena;
	Chunk *chunks;
	uint8_t chunk_size;
	int16_t width;
	int16_t height;
} World;

int get_chunk_index(World *w, int16_t chunk_x, int16_t chunk_y);
Chunk *get_chunk_at(World *w, int16_t chunk_x, int16_t chunk_y);
World *world_create(arena *a, world_config cfg);
int load_chunk(arena *a, Chunk *chunk);

// Materials
typedef struct {
	uint8_t data;
	uint8_t material;
} Pixel;

Pixel parse_pixel(uint8_t data);
uint8_t create_pixel(uint8_t type, uint8_t flags);

int physics_powder(
	Buffer *buf,
	Pixel p,
	int64_t row,
	int64_t col,
	int p_height,
	int p_width,
	int seed
);

int physics_liquid(
	Buffer *buf,
	Pixel p,
	int64_t row,
	int64_t col,
	int p_height,
	int p_width,
	uint8_t fluidity,
	int seed
);

typedef struct {
	Vector2 pos;
} Player;

#endif
