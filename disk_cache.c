#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>>
#include <sys/mman.h>
#include <strings.h>
#include <unistd.h>

#include "sha1/sha1.h"

#include "disk_cache.h"

/***CONSTANTS***/
#define FILENAME_MAX_LEN 32 //The max length of the cache filename
#define CACHE_FN "cache_data"

/***PREPROCESSOR FUNCTION DECLARATIONS***/
static size_t computeMaxFilePathSize(char *cache_directory_path);
static void computeCachePath(char *cache_directory_path, char *dest, int dest_len);
static void createDataFile(char *file_path, uint32_t num_lines);

DCCache DCMake(char *cache_directory_path, uint32_t num_lines) {
  size_t file_path_size = computeMaxFilePathSize(cache_directory_path);
  char file_path[file_path_size];
  computeCachePath(cache_directory_path, file_path, file_path_size);
  createDataFile(file_path, num_lines);

  return DCLoad(cache_directory_path);
}

DCCache DCLoad(char *cache_directory_path) {
  size_t file_path_size = computeMaxFilePathSize(cache_directory_path);
  char file_path[file_path_size];
  computeCachePath(cache_directory_path, file_path, file_path_size);
  
  DCCache cache = calloc(1, sizeof(DCCache_t));
  cache->fd = open(file_path, O_RDWR);
  cache->directory_path = strdup(cache_directory_path); // cache_directory_path could be freed

  //Read in the header
  read(cache->fd, &(cache->header), sizeof(DCCacheHeader_t));

  //mmap the lines
  size_t lines_start_offset = sizeof(DCCacheHeader_t); // The lines starts after the header
  size_t lines_size = cache->header.num_lines * sizeof(DCCacheLine_t);
  size_t total_file_size = lines_start_offset + lines_size;
  cache->mmap_start = mmap(0, lines_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, cache->fd, 0);
  if (cache->mmap_start == MAP_FAILED) {
    fprintf(stderr, "Map Failed! fd=%d, lines_size=%d, error:%s\n", cache->fd, lines_size, 
            strerror(errno));
    assert(0);
  }
  cache->lines = cache->mmap_start + lines_start_offset;
  return cache;
}

void DCCloseAndFree(DCCache cache) {
  size_t lines_size = cache->header.num_lines * sizeof(DCCacheLine_t);
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
static size_t computeMaxFilePathSize(char *cache_directory_path) {
  return FILENAME_MAX_LEN + strlen(cache_directory_path);
}

static void computeCachePath(char *cache_directory_path, char *dest, int dest_len) {
  sprintf(dest, "%s/%s", cache_directory_path, CACHE_FN);
}

static void createDataFile(char *file_path, uint32_t num_lines) {
  FILE *outfile = fopen(file_path, "w");

  // Create the header
  // TODO: FIGURE OUT evict_limit_in_bytes
  DCCacheHeader_t header = {.num_lines=num_lines, .evict_limit_in_bytes=0};
  fwrite(&header, sizeof(DCCacheHeader_t), 1, outfile);

  // Create the empty lines
  uint32_t line_size = sizeof(DCCacheLine_t);
  uint8_t line_buf[line_size];
  bzero(line_buf, line_size);
  for (uint32_t i=0; i < num_lines; i++) {
    fwrite(line_buf, sizeof(uint8_t), line_size, outfile);
  }
  fclose(outfile);
}

