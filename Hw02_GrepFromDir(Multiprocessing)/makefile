CC = gcc
CFLAGS = -ansi -pedantic -errors

moreOrLess: grepFromFile.o
	$(CC) -o moreOrLess grepFromFile.c

grepFromFile.o: grepFromFile.c
	$(CC) $(CFLAGS) -c grepFromFile.c

clean:
	rm -f moreOrLess *.o

install: moreOrLess
	cp moreOrLess /usr/bin
