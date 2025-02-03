#include "lib/arena.h"
#include <stdint.h>
#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#define min(a, b) a < b ? a : b
#define max(a, b) a > b ? a : b

typedef struct {
	Color color;
	char *name;
	uint8_t id;
	uint8_t density;
} _Material;

typedef struct {
	_Material *data;
	int32_t length;
	int32_t cap;
} array_material;

typedef struct {
	int x;
	int y;
} point;

typedef struct {
	point *data;
	int32_t cap;
	int32_t endIndex;
	int32_t startIndex;
} points_queue;

point *dequeue(points_queue *queue) {
	int32_t index = queue->startIndex % queue->cap;
	printf("INDEX: %d\n", index);
	queue->startIndex++;
	if (index > queue->cap) {
		return NULL;
	}
	printf("I: %d\n", index);
	printf("END: %d\n", queue->endIndex);
	return &queue->data[index];
}

void enqueue(points_queue *queue, point val) {
	int32_t index = queue->endIndex % queue->cap;
	queue->endIndex++;
	if (index > queue->cap) {
		printf("Queue overflow");
		abort();
	}
	queue->data[index] = val;
}

points_queue *points_queue_new(arena *a, int32_t cap) {
	points_queue *queue = new(a, points_queue);
	if (!queue) return NULL;

	queue->data = new(a, point, cap);
	if (!queue->data) {
		return NULL;
	}
	queue->cap = cap;
	queue->startIndex = 0;
	queue->endIndex = 0;
	return queue;
}

int32_t queue_len(points_queue *q) {
	return  q->endIndex - q->startIndex;
}

typedef struct {
	char *cells;
} row;

typedef struct {
	row *rows;
} buffer;

buffer *buffer_create(arena *a, int64_t rows, int64_t cols) {
	buffer *b = new(a, buffer);
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
		if (!b->rows[i].cells) {
			printf("Failed to allocate cells at %ld\n", i);
			abort();
		}
	}
	return b;
}

typedef struct {
	uint8_t data;
	uint8_t material;
} pixel;

const uint8_t P_FROZEN = 1U<<5;
const uint8_t P_NEW = 1U<<6;
const uint8_t P_SPECIAL = 1U<<7;

const uint8_t MATERIAL_MASK = 0b00011111;
const uint8_t FLAGS_MASK = 0b11100000;

const uint8_t M_AIR = 0;
const uint8_t M_SAND = 1;
const uint8_t M_WATER = 2;

const uint8_t M_densities[3] = {
	0,
	10,
	1
};
const Color M_colors[3] = {
	(Color){0,0,0,255},
	(Color){222,178,58,255},
	(Color){56,64,201,255},
};

const Color M_colors_special[3] = {
	(Color){0,0,0,255},
	(Color){207,169,64,255},
	(Color){66,75,214,255},
};


pixel parse_pixel(uint8_t data) {
	return (pixel){
		.data = data,
		.material = data & MATERIAL_MASK
	};
}

uint8_t create_pixel(uint8_t type, uint8_t flags) {
	assert(type <= 31);
	return type | flags;
}

void physics_powder(
	buffer *buf,
	pixel p,
	int64_t row,
	int64_t col,
	int p_height,
	int p_width,
	int seed
) {
	if (row < p_height - 1) {
		pixel below = parse_pixel(buf->rows[row + 1].cells[col]);
		uint8_t density = M_densities[p.material];

		if (
			M_densities[below.material] < density
		) {
			buf->rows[row].cells[col] = below.data | P_NEW;
			buf->rows[row + 1].cells[col] = p.data | P_NEW;
			return;
		}

		int move_left = (col > 0 && (seed % 2));
		int move_right = (col < p_width - 1 && !move_left);

		if (move_left) {
			pixel b_left = parse_pixel(buf->rows[row + 1].cells[col - 1]);
			if (M_densities[b_left.material] < density) {
				buf->rows[row].cells[col] = b_left.data | P_NEW;
				buf->rows[row + 1].cells[col - 1] = p.data | P_NEW;
				return;
			}
		}

		if (move_right) {
			pixel b_right = parse_pixel(buf->rows[row + 1].cells[col + 1]);
			if (M_densities[b_right.material] < density) {
				buf->rows[row].cells[col] = b_right.data | P_NEW;
				buf->rows[row + 1].cells[col + 1] = p.data | P_NEW;
				return;
			}
		}
	}

	if (!(p.data & P_NEW) && row > 0 && col > 0 && col < p_width - 1) {
		pixel above = parse_pixel(buf->rows[row - 1].cells[col]);
		pixel left = parse_pixel(buf->rows[row].cells[col - 1]);
		pixel right = parse_pixel(buf->rows[row].cells[col + 1]);

		if (
			above.material &&
			left.material &&
			right.material
		) {
			buf->rows[row].cells[col] = p.data | P_FROZEN;
		}
	}
}

void physics_liquid(
	buffer *buf,
	pixel p,
	int64_t row,
	int64_t col,
	int p_height,
	int p_width,
	int seed
) {
	if (row < p_height - 1) {
		pixel below = parse_pixel(buf->rows[row + 1].cells[col]);
		uint8_t density = M_densities[p.material];

		if (M_densities[below.material] < density) {
			buf->rows[row].cells[col] = below.data;
			buf->rows[row + 1].cells[col] = p.data | P_NEW;
			return;
		}

		int move_left = (col > 0 && (seed % 2));
		int move_right = (col < p_width - 1 && !move_left);

		if (move_left) {
			pixel b_left = parse_pixel(buf->rows[row + 1].cells[col - 1]);
			if (M_densities[b_left.material] < density) {
				buf->rows[row].cells[col] = b_left.data;
				buf->rows[row + 1].cells[col - 1] = p.data | P_NEW;
				return;
			}
			pixel left = parse_pixel(buf->rows[row].cells[col - 1]);
			if (M_densities[left.material] < density) {
				buf->rows[row].cells[col] = left.data;
				buf->rows[row].cells[col - 1] = p.data | P_NEW;
				return;
			}
		}

		if (move_right) {
			pixel b_right = parse_pixel(buf->rows[row + 1].cells[col + 1]);
			if (M_densities[b_right.material] < density) {
				buf->rows[row].cells[col] = b_right.data;
				buf->rows[row + 1].cells[col + 1] = p.data | P_NEW;
				return;
			}
			pixel right = parse_pixel(buf->rows[row].cells[col + 1]);
			if (M_densities[right.material] < density) {
				buf->rows[row].cells[col] = right.data;
				buf->rows[row].cells[col + 1] = p.data | P_NEW;
				return;
			}
		}
	}

	if (!(p.data & P_NEW) && row > 0 && col > 0 && col < p_width - 1) {
		pixel above = parse_pixel(buf->rows[row - 1].cells[col]);
		pixel left = parse_pixel(buf->rows[row].cells[col - 1]);
		pixel right = parse_pixel(buf->rows[row].cells[col + 1]);

		if (
			above.material && 
			left.material && 
			right.material
		) {
			buf->rows[row].cells[col] = p.data | P_FROZEN;
		}
	}
}

int main() {
	const int pixelSize = 6;
	SetTargetFPS(60);

	int width = 2560;
	int height = (int)width * 0.625;

	int p_width = width / pixelSize;
	int p_height = height / pixelSize;

	int64_t pixelGridSize = (int)(width * height) / pixelSize;

	arena a_main = arena_new(pixelGridSize + (1<<16));
	arena a_buffer = arena_slice(&a_main, pixelGridSize);

	buffer *buf = buffer_create(&a_buffer, p_height, p_width);

	uint8_t currentMaterial = 1;

	InitWindow(width, height, "Fallingsomething");

	for (int64_t row = p_height-1; row > 0; row--) {
		for (int64_t col = p_width-1; col > 0; col--) {
			buf->rows[row].cells[col] = P_FROZEN;
		}
	}

	float frameTime = 0;
	while (!WindowShouldClose()) {
		BeginDrawing();
		int mY = max(min(GetMouseY() / pixelSize, p_height), 0);
		int mX = max(min(GetMouseX() / pixelSize, p_width), 0);
		int radius = 10;

		if (IsKeyPressed(KEY_ZERO)) currentMaterial = 0;
		if (IsKeyPressed(KEY_ONE)) currentMaterial = 1;
		if (IsKeyPressed(KEY_TWO)) currentMaterial = 2;

		if (IsMouseButtonDown(0)) {
			for (int64_t row = 0; row < p_height-1; row++) {
				for (int64_t col = 0; col < p_width-1; col++) {
					if (rand() % 2) {
						continue;
					}
					if ((col - mX) * (col - mX) + (row - mY) * (row - mY) <= radius * radius) {
						buf->rows[row].cells[col] = 
							currentMaterial | (rand() % 2 ? P_NEW : P_NEW|P_SPECIAL);
					}
				}
			}
		}

		for (int64_t row = p_height-1; row > 0; row--) {
			for (int64_t col = p_width-1; col > 0; col--) {
				int seed = rand();
				pixel p = parse_pixel(buf->rows[row].cells[col]);
				if (p.material == M_AIR) {
					if (row > 0) {
						pixel p = parse_pixel(buf->rows[row-1].cells[col]);
						if (p.data & P_FROZEN) {
							buf->rows[row-1].cells[col] = 
								create_pixel(p.material, 0);
						}
					}
					if (row > 0 && (col > 0 || col < p_width-1)) {
						pixel a_left = parse_pixel(buf->rows[row-1].cells[col-1]);
						if (a_left.data & P_FROZEN) {
							buf->rows[row-1].cells[col-1] = 
								create_pixel(a_left.material, 0);
						}
						pixel a_right = parse_pixel(buf->rows[row-1].cells[col+1]);
						if (a_left.data & P_FROZEN) {
							buf->rows[row-1].cells[col+1] = 
								create_pixel(a_right.material, 0);
						}
					}
				}

				if (p.material == M_SAND) {
					physics_powder(buf, p, row, col, p_height, p_width, seed);
					continue;
				}
				if (p.material == M_WATER) {
					physics_liquid(buf, p, row, col, p_height, p_width, seed);
					continue;
				}
			}
		}

		if (frameTime >= 0.032) {
			for (int64_t row = 0; row < p_height-1; row++) {
				for (int64_t col = 0; col < p_width-1; col++) {
					pixel p = parse_pixel(buf->rows[row].cells[col]);
					if (p.data & P_FROZEN) continue;
					DrawRectangle(
						col*pixelSize,
						row*pixelSize,
						pixelSize,
						pixelSize,
						p.data & P_SPECIAL ? M_colors_special[p.material] : M_colors[p.material]
					);
				}
			}
		}

		char fps[50];
		sprintf(fps, "%d", GetFPS());
		DrawText(fps, 20, 20, 30, RAYWHITE);

		frameTime += GetFrameTime();
		EndDrawing();
	}

	CloseWindow();

	arena_destroy(&a_buffer);
	arena_destroy(&a_main);

	return 0;
}

