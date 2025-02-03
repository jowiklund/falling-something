d_build:
	clang main.c ./lib/arena.c -std=c99 -Wall -Werror -o main -fsanitize=address -g -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

build:
	clang main.c ./lib/arena.c -std=c99 -Wall -Werror -o main -fsanitize=address -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

run: build
	./main

debug: d_build
	gdb ./main

