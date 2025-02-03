#ifndef W_FALLING
#define W_FALLING

// Buffer
#include "arena.h"
#include <stdint.h>
typedef struct {
	char *cells;
} row;

typedef struct {
	row *rows;
} buffer;

buffer *buffer_create(arena *a, int64_t rows, int64_t cols);

// Materials
typedef struct {
	uint8_t data;
	uint8_t material;
} pixel;

pixel parse_pixel(uint8_t data);
uint8_t create_pixel(uint8_t type, uint8_t flags);

int physics_powder(
	buffer *buf,
	pixel p,
	int64_t row,
	int64_t col,
	int p_height,
	int p_width,
	int seed
);

int physics_liquid(
	buffer *buf,
	pixel p,
	int64_t row,
	int64_t col,
	int p_height,
	int p_width,
	uint8_t fluidity,
	int seed
);

#endif
