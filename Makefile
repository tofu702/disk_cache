CC=gcc
COMPILER_DEFINES=-D _XOPEN_SOURCE=500
CFLAGS=-Wall -std=c99 -g $(COMPILER_DEFINES)
SOURCES=disk_cache.c sha1/sha1.c
TEST_SOURCES=$(SOURCES) test.c
BENCHMARK_SOURCES=$(SOURCES) benchmark.c


build: $(TEST_SOURCES) $(BENCHMARK_SOURCES)
	$(CC) -o test $(TEST_SOURCES) $(CFLAGS)
	$(CC) -o benchmark $(BENCHMARK_SOURCES) $(CFLAGS)

test: build
	./test

benchmark: build
	./benchmark
