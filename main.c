#include "lib/falling.h"
#include "lib/buffer.c"
#include "lib/materials.c"
#include <stdint.h>
#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>

#define min(a, b) a < b ? a : b
#define max(a, b) a > b ? a : b

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
	queue->startIndex++;
	if (index > queue->cap) {
		return NULL;
	}
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

int main() {
	const int pixelSize = 6;

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

	for (int64_t row = 0; row < p_height; row++) {
		for (int64_t col = 0; col < p_width; col++) {
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
			for (int64_t row = 0; row < p_height; row++) {
				for (int64_t col = 0; col < p_width; col++) {
					if ((col - mX) * (col - mX) + (row - mY) * (row - mY) <= radius * radius) {
						buf->rows[row].cells[col] = 
							currentMaterial | (rand() % 2 ? P_NEW : P_NEW|P_SPECIAL);
					}
				}
			}
		}

		for (int64_t row = 0; row < p_height; row++) {
			int seed = rand();
			int even = row % 2;
			for (
				int64_t col = even ? 0 : p_width;
				even ? (col < p_width) : (col > 0);
				(even ? col++ : col--)
			) {
				pixel p = parse_pixel(buf->rows[row].cells[col]);
				if (p.data & P_NEW) {
					buf->rows[row].cells[col] = p.data & ~P_NEW;
					continue;
				}
				if (p.material == M_AIR) {
					if (row > 0) {
						pixel p = parse_pixel(buf->rows[row-1].cells[col]);
						if (p.data & P_FROZEN) {
							buf->rows[row-1].cells[col] = p.data;
						}
					}
					if (row > 0 && (col > 0 || col < p_width-1)) {
						pixel a_left = parse_pixel(buf->rows[row-1].cells[col-1]);
						if (a_left.data & P_FROZEN) {
							buf->rows[row-1].cells[col-1] = a_left.material;
						}
						pixel a_right = parse_pixel(buf->rows[row-1].cells[col+1]);
						if (a_right.data & P_FROZEN) {
							buf->rows[row-1].cells[col+1] = a_right.material;
						}
					}
				}

				if (p.material == M_SAND) {
					physics_powder(buf, p, row, col, p_height, p_width, seed);
					continue;
				}
				if (p.material == M_WATER) {
					physics_liquid(buf, p, row, col, p_height, p_width, 5, seed);
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

