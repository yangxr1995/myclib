

all:test
test:src/memchk.c src/main.c src/assert.c src/except.c src/arena.c src/arena.c
	gcc -I./include $^ -g -O0 -o $@

clean:
	rm -f test
