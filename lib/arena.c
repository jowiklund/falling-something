#include "arena.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int arena_grow(arena *a, ptrdiff_t min_additional) {
	ptrdiff_t current_size = a->end - a->base;
	ptrdiff_t new_size = current_size ? current_size * 2 : 1024;
	if (new_size < current_size + min_additional) {
		new_size = current_size + min_additional;
	}

	int *new_base = realloc(a->base, new_size);
	if (!new_base) {
		return -1;
	}

	a->start = new_base + (a->start - a->base);
	a->end = new_base + new_size;
	a->base = new_base;

	return 0;
}

static stack *stack_create(int cap) {
	stack *s = malloc(sizeof(stack));
	if (!s) return NULL;

	s->data = malloc(sizeof(int) * cap);
	if (!s->data) return NULL;
	s->cap = cap;
	s->len = 0;

	return s;
}

static void stack_push(stack *s, int val) {
	if (s->len >= s->cap) {
		printf("Stack overflow");
		abort();
	}
	s->data[s->len] = val;
	s->len++;
}

static int stack_pop(stack *s) {
	if (s->len <= 0) return -1;
	int val = s->data[s->len-1];
	s->len--;
	return val;
}

static void stack_destroy(stack *s) {
	free(s->data);
	free(s);
}

void *arena_alloc(arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, int flags) {
	ptrdiff_t padding = -(uintptr_t)a->start & (align - 1);
	ptrdiff_t available = a->end - a->start - padding;
	if (available < 0 || count > available / size) {
		if (flags & F_SOFTFAIL) {
			return NULL;
		}
		if (flags & F_REALLOC) {
			ptrdiff_t required = padding + count * size;
			if (arena_grow(a, required) == 0) {
				return arena_alloc(a, size, align, count, flags & ~F_REALLOC);
			}
		}
		abort();
	}
	if (flags & F_TRACKED) {
		if (!a->allocs) {
			a->allocs = stack_create(100);
		}
		stack_push(a->allocs, padding + count * size);
	}
	void *p = a->start + padding;
	a->start += padding + count * size;
	return flags & F_NOZERO ? p : memset(p, 0, count * size);
}

arena arena_new(ptrdiff_t cap) {
	arena a = {0};
	a.start = malloc(cap);
	a.base = a.start;
	a.end = a.start ? a.start + cap : 0;
	a.allocs = stack_create(100);
	a.needs_free = 1;
	return a;
}

void arena_destroy(arena *a) {
	if (a->allocs) {
		stack_destroy(a->allocs);
	}
	if (a->base) {
		if (a->needs_free) {
			free(a->base);
		}
		a->base = NULL;
		a->start = NULL;
		a->end = NULL;
		a->allocs = NULL;
	}
}

void arena_pop(arena *a) {
	int ptr = stack_pop(a->allocs);
	a->start -= ptr;
}

arena arena_slice(arena *a, ptrdiff_t cap) {
	arena new_a = {0};
	ptrdiff_t alignment = sizeof(void*) > sizeof(double) ? sizeof(void*) : sizeof(double);
	new_a.start = arena_alloc(a, 1, alignment, cap, 0);
	new_a.base = new_a.start;
	new_a.end = new_a.start ? new_a.start + cap : 0;
	new_a.allocs = stack_create(100);
	new_a.needs_free = 0;
	return new_a;
}
