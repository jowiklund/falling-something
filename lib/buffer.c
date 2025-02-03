#include "falling.h"
#include <stdlib.h>
#include <stdio.h>

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
