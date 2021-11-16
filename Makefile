

all:test
test:src/main.c src/except.c src/assert.c src/memchk.c	
	gcc -I./include $^ -g -O0 -o $@

clean:
	rm -f test
