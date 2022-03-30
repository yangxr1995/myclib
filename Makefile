
all:test
test:src/memchk.c src/main.c src/assert.c src/palloc.c src/fmt.c
	gcc -I./include $^ -g -O0 -o $@ -rdynamic

clean:
	rm -f test
