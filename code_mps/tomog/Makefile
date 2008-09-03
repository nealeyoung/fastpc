all: tomog

tomog: tomog.o
	gcc -lm tomog.o -o tomog

tomog.o: tomog.c
	gcc -c -O3 tomog.c

clean:
	rm -rf *.o tomog