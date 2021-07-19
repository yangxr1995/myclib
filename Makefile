
all:
	gcc main.c memchk.c
	./a.out

clean:
	rm -f a.out
