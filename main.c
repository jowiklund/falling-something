#include "lib/falling.h"
#include "lib/world.c"
#include "lib/materials.c"
#include <math.h>
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
	const int pixel_size = 6;
	int screenWidth = 2560;
	int screenHeight = (int)screenWidth * 0.625;

	// int p_width = screenWidth / pixel_size;
	// int p_height = screenHeight / pixel_size;

	world_config wc = (world_config){
		.chunk_size = 32,
		.width = 512,
		.height = 128
	};

	int chunk_buffer = (wc.chunk_size * pixel_size) * 3;

	int32_t world_size = 
		(wc.width * wc.height) * wc.chunk_size +
		(sizeof(Chunk) * (wc.width * wc.height)) + 
		sizeof(World);
	printf("WORLD SIZE: %d\n", world_size);
	arena a_main = arena_new(1000 * 1000 * 500);

	World *w = world_create(&a_main, wc);
	printf("CHUNK SIZE: %d\n", w->chunk_size);

	Player player = {
		.pos = (Vector2){
			((float)(wc.width * wc.chunk_size) / 2) * pixel_size,
			((float)(wc.height * wc.chunk_size) / 2) * pixel_size
		}
	};

	Camera2D camera = { 0 };
    camera.target = (Vector2){ player.pos.x + 20.0f, player.pos.y + 20.0f };
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

	InitWindow(screenWidth, screenHeight, "Fallingsomething");

	SetTargetFPS(60);
	while (!WindowShouldClose()) {
		BeginDrawing();

		if (IsKeyDown(KEY_D)) player.pos.x += 4 * pixel_size;
		else if (IsKeyDown(KEY_A)) player.pos.x -= 4 * pixel_size;

		if (IsKeyDown(KEY_W)) player.pos.y -= 4 * pixel_size;
		else if (IsKeyDown(KEY_S)) player.pos.y += 4 * pixel_size;

		camera.target = (Vector2){ player.pos.x, player.pos.y };

		if (IsKeyDown(KEY_LEFT)) camera.rotation--;
		else if (IsKeyDown(KEY_RIGHT)) camera.rotation++;

		if (camera.rotation > 40) camera.rotation = 40;
		else if (camera.rotation < -40) camera.rotation = -40;

		camera.zoom += ((float)GetMouseWheelMove()*0.05f);

		if (IsKeyPressed(KEY_R))
		{
			camera.zoom = 1.0f;
			camera.rotation = 0.0f;
		}

		BeginMode2D(camera);

		ClearBackground(BLACK);

		Vector2 mouse_pos = (Vector2){
			GetMouseX(),
			GetMouseY()
		};

		Vector2 mouse_pos_w = GetScreenToWorld2D(mouse_pos, camera);
		Vector2 mouse_pos_chunk = (Vector2){
			mouse_pos_w.x / pixel_size / w->chunk_size,
			mouse_pos_w.y / pixel_size / w->chunk_size
		};

		// printf("Mouse chunk X: %f\n", mouse_pos_chunk.x);
		// printf("Mouse chunk Y: %f\n", mouse_pos_chunk.y);
		// printf("Mouse chunk pos X: %d\n", ((int)mouse_pos_w.x / pixel_size) % w->chunk_size);
		// printf("Mouse chunk pos Y: %d\n", ((int)mouse_pos_w.y / pixel_size) % w->chunk_size);

		if (
			mouse_pos_chunk.x > 0 &&
			mouse_pos_chunk.y > 0 &&
			mouse_pos_chunk.x < w->width &&
			mouse_pos_chunk.y < w->height
		) {
			Chunk *chunk = get_chunk_at(
				w,
				floor(mouse_pos_chunk.x),
				floor(mouse_pos_chunk.y)
			);
			// printf("MOUSE CHUNK: %d\n", chunk->simulated);
			// if (chunk) {
			// 	chunk->simulated = 1;
			// }
			if (IsMouseButtonPressed(0)) {
				chunk->simulated = 1;
			}
		}
		

		// int chunk_i = get_chunk_index(w, mouse_pos_w.x, mouse_pos_w.y);
		//
		// Chunk chunk = w->chunks[chunk_i];
		// if (!chunk.data) {
		// 		w->chunks[chunk_i].data = buffer_create(&a_main, w->chunk_size, w->chunk_size);
		// }

		for (int i = 0; i < w->height * w->width; i++) {
			int x = i % w->width;
			int y = floor((float)i / w->width);

			Chunk *chunk = get_chunk_at(w, x, y);
			if (!chunk->simulated) {
				continue;
			}

			chunk->data->rows[10].cells[15] = 1;
			int did_move = 0;
			int *p_did_move = &did_move;

			for (int c_row = 0; c_row < w->chunk_size; c_row++) {
				for (int c_col = 0; c_col < w->chunk_size; c_col++) {
					Pixel p = parse_pixel(chunk->data->rows[c_row].cells[c_col]);
					if (p.material == M_SAND) {
						*p_did_move = physics_powder(chunk->data, p, c_row, c_col, w->chunk_size, w->chunk_size, rand());
					}
				}
			}
			if (p_did_move) {
				printf("DID_MOVE %d\n", did_move);
				chunk->simulated = 1;
				chunk->rendered = 1;
			}
			if (!p_did_move && chunk->simulated) {
				chunk->simulated = 0;
			}
		}

		for (int i = 0; i < w->height * w->width; i++) {
			int x = i % w->width;
			int y = floor((float)i / w->width);

			int col = (x * w->chunk_size) * pixel_size;
			int row = (y * w->chunk_size) * pixel_size;
			int width = w->chunk_size * pixel_size;
			int height = w->chunk_size * pixel_size;

			Vector2 screen = GetWorldToScreen2D((Vector2){col, row}, camera);
			Chunk *chunk = get_chunk_at(w, x, y);

			if (
				screen.x + chunk_buffer < 0 ||
				screen.y + chunk_buffer < 0 ||
				screen.x - chunk_buffer > screenWidth ||
				screen.y - chunk_buffer > screenHeight
			) {
				chunk->rendered = 0;
				chunk->simulated = 0;
				continue;
			}

			chunk->rendered = 1;

			if (!chunk->data) {
				int ok = load_chunk(&a_main, chunk);
				if (!ok) {
					printf("Failed to load chunk (%d, %d)", x, y);
					abort();
				}
			} else {
				chunk->simulated = 1;
			}

			if (chunk->rendered) {
				if (chunk->data) {
					for (int c_row = 0; c_row < w->chunk_size; c_row++) {
						for (int c_col = 0; c_col < w->chunk_size; c_col++) {
							Pixel pixel = parse_pixel(chunk->data->rows[c_row].cells[c_col]);
							DrawRectangle(col + c_col * pixel_size, row + c_row * pixel_size, pixel_size, pixel_size, M_colors[pixel.material]);
						}
					}
				}
			}
			DrawRectangleLines(col, row, width, height, RED);


			// chunk chunk = get_chunk_at(w, x, y);

		}
		EndMode2D();
		// int mY = max(min(GetMouseY() / pixel_size, p_height), 0);
		// int mX = max(min(GetMouseX() / pixel_size, p_width), 0);
		// int radius = 10;

		// if (IsKeyPressed(KEY_ZERO)) currentMaterial = 0;
		// if (IsKeyPressed(KEY_ONE)) currentMaterial = 1;
		// if (IsKeyPressed(KEY_TWO)) currentMaterial = 2;

		// if (IsMouseButtonDown(0)) {
		// 	for (int64_t row = 0; row < p_height; row++) {
		// 		for (int64_t col = 0; col < p_width; col++) {
		// 			if ((col - mX) * (col - mX) + (row - mY) * (row - mY) <= radius * radius) {
		// 				buf->rows[row].cells[col] = 
		// 					currentMaterial | (rand() % 2 ? 0 : P_SPECIAL);
		// 			}
		// 		}
		// 	}
		// }

		// int64_t minX = p_width;
		// int64_t minY = p_height;
		// int64_t maxX = 0;
		// int64_t maxY = 0;

		// for (int64_t row = 0; row < p_height; row++) {
		// 	int seed = rand();
		// 	int even = row % 2;
		// 	for (
		// 		int64_t col = even ? 0 : p_width;
		// 		even ? (col < p_width) : (col > 0);
		// 		(even ? col++ : col--)
		// 	) {
		// 		int did_move = 0;
		// 		pixel p = parse_pixel(buf->rows[row].cells[col]);
		// 		if (p.data & P_NEW) {
		// 			buf->rows[row].cells[col] = p.data & ~P_NEW;
		// 			continue;
		// 		}
		// 		if (p.material == M_AIR) {
		// 			if (row > 0) {
		// 				pixel p = parse_pixel(buf->rows[row-1].cells[col]);
		// 				if (p.data & P_FROZEN) {
		// 					buf->rows[row-1].cells[col] = p.data;
		// 				}
		// 			}
		// 			if (row > 0 && (col > 0 || col < p_width-1)) {
		// 				pixel a_left = parse_pixel(buf->rows[row-1].cells[col-1]);
		// 				if (a_left.data & P_FROZEN) {
		// 					buf->rows[row-1].cells[col-1] = a_left.material;
		// 				}
		// 				pixel a_right = parse_pixel(buf->rows[row-1].cells[col+1]);
		// 				if (a_right.data & P_FROZEN) {
		// 					buf->rows[row-1].cells[col+1] = a_right.material;
		// 				}
		// 			}
		// 		}
		//
		// 		if (p.material == M_SAND) {
		// 			did_move = physics_powder(buf, p, row, col, p_height, p_width, seed);
		// 		}
		// 		if (p.material == M_WATER) {
		// 			did_move = physics_liquid(buf, p, row, col, p_height, p_width, 5, seed);
		// 		}
		//
		// 		if (did_move) {
		// 			if (col < minX) minX = max(col - 5, 0);
		// 			if (row < minY) minY = max(row - 5, 0);
		// 			if (col > maxX) maxX = min(col + 5, p_width);
		// 			if (row > maxY) maxY = min(row + 5, p_height);
		// 		}
		// 	}
		// }

		// if (
		// 	minX < p_width ||
		// 	minY < p_height ||
		// 	maxX > 0 ||
		// 	maxY > 0
		// ) {
		// 	DrawRectangleLines(minX * pixelSize, minY * pixelSize, (maxX-minX)*pixelSize, (maxY-minY)*pixelSize, GREEN);
		// }

		// if (frameTime >= 0.032) {
		// 	for (int64_t row = 0; row < p_height-1; row++) {
		// 		for (int64_t col = 0; col < p_width-1; col++) {
		// 			if (row > minY && row < maxY && col > minX && col < maxX) {
		// 				pixel p = parse_pixel(buf->rows[row].cells[col]);
		// 				DrawRectangle(
		// 					col*pixelSize,
		// 					row*pixelSize,
		// 					pixelSize,
		// 					pixelSize,
		// 					p.data & P_SPECIAL ? M_colors_special[p.material] : M_colors[p.material]
		// 				);
		// 			}
		// 		}
		// 	}
		// }

		DrawRectangle( 10, 10, 100, 50, BLACK);
		char fps[50];
		sprintf(fps, "%d", GetFPS());
		DrawText(fps, 20, 20, 30, RAYWHITE);
		//
		// frameTime += GetFrameTime();
		EndDrawing();
	}

	CloseWindow();

	arena_destroy(&a_main);

	return 0;
}

