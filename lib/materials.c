#include "falling.h"
#include <assert.h>
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>

const uint8_t P_FROZEN = 1U<<5;
const uint8_t P_NEW = 1U<<6;
const uint8_t P_SPECIAL = 1U<<7;

const uint8_t MATERIAL_MASK = 0b00011111;
const uint8_t FLAGS_MASK = 0b11100000;

const uint8_t M_AIR = 0;
const uint8_t M_SAND = 1;
const uint8_t M_WATER = 2;

const uint8_t M_densities[3] = {
	0, // AIR
	10, // SAND
	1 // WATER
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

uint8_t create_pixel(uint8_t type, uint8_t flags) {
	assert(type <= 31);
	return type | flags;
}

pixel parse_pixel(uint8_t data) {
	return (pixel){
		.data = data,
		.material = data & MATERIAL_MASK
	};
}

int physics_powder(
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
			return 1;
		}
		int move_left = (col > 0 && (seed % 2));
		int direction = move_left ? -1 : 1;

		pixel b_left = parse_pixel(buf->rows[row + 1].cells[col + direction]);
		if (M_densities[b_left.material] < density) {
			buf->rows[row].cells[col] = b_left.data | P_NEW;
			buf->rows[row + 1].cells[col + direction] = p.data | P_NEW;
			return 1;
		}
	}
	return 0;
}

int physics_liquid(
	buffer *buf,
	pixel p,
	int64_t row,
	int64_t col,
	int p_height,
	int p_width,
	uint8_t fluidity,
	int seed
) {
	if (row < p_height - 1) {
		uint8_t density = M_densities[p.material];

		pixel below = parse_pixel(buf->rows[row + 1].cells[col]);
		if (M_densities[below.material] < density) {
			buf->rows[row].cells[col] = below.data;
			buf->rows[row + 1].cells[col] = p.data | P_NEW;
			return 1;
		}

		for (int i = 0; i < fluidity; i++) {
			int move_left = (col > 0 && (seed % 2));
			int direction = move_left ? -(i) : i;

			pixel b_dir = parse_pixel(buf->rows[row + 1].cells[col + direction]);
			if (M_densities[b_dir.material] < density) {
				buf->rows[row].cells[col] = b_dir.data;
				buf->rows[row + 1].cells[col + direction] = p.data | P_NEW;
				return 1;
			}
			pixel dir = parse_pixel(buf->rows[row].cells[col + direction]);
			if (M_densities[dir.material] < density) {
				buf->rows[row].cells[col] = dir.data;
				buf->rows[row].cells[col + direction] = p.data | P_NEW;
				return 1;
			}
		}
	}
	return 0;
}
