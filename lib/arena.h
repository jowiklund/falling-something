#ifndef W_ARENA
#define W_ARENA

#include <stddef.h>
#include <stdint.h>

typedef struct {
	int cap;
	int len;
	int *data;
} stack;

typedef struct {
	int *base;
	int *start;
	int *end;
	uint8_t needs_free;
	uint8_t tracked;
	stack *allocs;
} arena;

#define new(...) newx(__VA_ARGS__,new4,new3,new2)(__VA_ARGS__)
#define newx(a,b,c,d,e,...) e
#define new2(a, t) (t *)arena_alloc(a, sizeof(t), _Alignof(t), 1, 0)
#define new3(a, t, n) (t *)arena_alloc(a, sizeof(t), _Alignof(t), n, 0)
#define new4(a, t, n, f) (t *)arena_alloc(a, sizeof(t), _Alignof(t), n, f)

#define F_SOFTFAIL 1U<<0
#define F_NOZERO 1U<<1
#define F_REALLOC  1U<<2
#define F_TRACKED  1U<<3

void *arena_alloc(arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count, int flags);
arena arena_new(ptrdiff_t cap);
void arena_destroy(arena *a);
void arena_pop(arena *a);
arena arena_slice(arena *a, ptrdiff_t cap);

#endif
