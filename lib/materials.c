#include "falling.h"
#include <assert.h>
#include <raylib.h>

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

