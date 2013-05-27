#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>>
#include <sys/mman.h>
#include <strings.h>
#include <unistd.h>

#include "disk_cache.h"

/***CONSTANTS***/
#define FILENAME_MAX_LEN 32 //The max length of the cache filename
#define CACHE_FN "cache_data"

/***PREPROCESSOR FUNCTION DECLARATIONS***/
static void computeCachePath(char *cache_directory_path, char *dest, int dest_len);
static void createDataFile(char *file_path, uint32_t num_lines);


DCCache DCMake(char *cache_directory_path, uint32_t num_lines) {
  size_t file_path_size = strlen(cache_directory_path) + FILENAME_MAX_LEN;
  char file_path[file_path_size];
  size_t lines_size = num_lines * sizeof(DCCacheLine_t);

  computeCachePath(cache_directory_path, file_path, file_path_size);
  createDataFile(file_path, num_lines);

  DCCache cache = calloc(1, sizeof(DCCache_t));
  cache->fd = open(file_path, O_RDWR);
  cache->num_lines = num_lines;
  cache->directory_path = strdup(cache_directory_path);
  cache->lines = mmap(0, lines_size, PROT_READ | PROT_WRITE, MAP_FILE, cache->fd, 0);
  
  return cache;
}

DCCache DCLoad(char *cache_directory_path) {
  return NULL;
}

void DCCloseAndFree(DCCache cache) {
  size_t lines_size = cache->num_lines * sizeof(DCCacheLine_t);
  munmap(cache->lines, lines_size);
  close(cache->fd);
  free(cache);
}

void DCAdd(DCCache cache, char *key, uint8_t *data, uint64_t data_len) {

}

DCData DCLookup(DCCache cache, char *key) {
  DCData returnme = {.data=NULL, .data_len=0};
  return returnme;
}

/***STATIC HELPERS***/
static void computeCachePath(char *cache_directory_path, char *dest, int dest_len) {
  sprintf(dest, "%s/%s", cache_directory_path, CACHE_FN);
}

static void createDataFile(char *file_path, uint32_t num_lines) {
  FILE *outfile = fopen(file_path, "w");
  uint32_t line_size = sizeof(DCCacheLine_t);
  printf("Line Size=%d\n", line_size);
  uint8_t line_buf[line_size];
  bzero(line_buf, line_size);
  for (uint32_t i=0; i < num_lines; i++) {
    fwrite(line_buf, sizeof(uint8_t), line_size, outfile);
  }
  fclose(outfile);
}

