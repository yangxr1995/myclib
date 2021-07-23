
mem:
	gcc memchk*.c
	./a.out

table:
	gcc table*.c memchk.c

clean:
	rm -f a.out  log
