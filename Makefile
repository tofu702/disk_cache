CC=gcc
CFLAGS=-Wall -std=c99 -g
SOURCES=test.c disk_cache.c sha1/sha1.c


build: $(SOURCES)
	$(CC) -o test $(SOURCES) $(CFLAGS)

test: build
	./test
