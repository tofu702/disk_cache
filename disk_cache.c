#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>>
#include <sys/mman.h>
#include <sys/time.h>
#include <strings.h>
#include <unistd.h>

#include "sha1/sha1.h"

#include "disk_cache.h"

/***CONSTANTS***/
#define FILENAME_MAX_LEN 128 //The max length of the cache filename
#define CACHE_FN "cache_data"
#define NUM_LOOKUP_INDICIES 4
#define UNUSED_LAST_ACCESS_TIME 0

/***PREPROCESSOR FUNCTION DECLARATIONS***/
static size_t computeMaxFilePathSize(char *cache_directory_path);
static void computeCachePath(char *cache_directory_path, char *dest, int dest_len);
static void createDataFile(char *file_path, uint32_t num_lines);
static void computeLookupIndiciesForKey(uint64_t key_sha1[2], uint32_t indicies[NUM_LOOKUP_INDICIES], uint32_t num_lines);
static void SHA1ForKey(char *key, uint64_t sha1[2]);
static void pathForSHA1(DCCache cache, uint64_t sha1[2], char *dest);
static void removeFileForLine(DCCache cache, DCCacheLine_t *line);
static uint64_t currentTimeInMSFromEpoch();

//DCAdd Helpers
static DCCacheLine_t *findBestLineToWriteKeyTo(DCCache cache, uint64_t key_sha1[2]);
static void saveDataFileForKey(DCCache cache, uint64_t sha1[2], uint8_t *data, uint64_t data_len);

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
  cache->mmap_start = mmap(0, total_file_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, cache->fd, 0);
  if (cache->mmap_start == MAP_FAILED) {
    fprintf(stderr, "Map Failed! fd=%d, lines_size=%d, error:%s\n", (int)cache->fd, (int)lines_size, 
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
  uint64_t key_sha1[2];
  SHA1ForKey(key, key_sha1);

  // TODO: Evict here

  DCCacheLine_t *line_to_replace = findBestLineToWriteKeyTo(cache, key_sha1);
  if (line_to_replace->last_access_time_in_ms_from_epoch != UNUSED_LAST_ACCESS_TIME) {
    // We need to get rid of the old line
    removeFileForLine(cache, line_to_replace);
  }

  // Set the line state
  line_to_replace->last_access_time_in_ms_from_epoch = currentTimeInMSFromEpoch();
  line_to_replace->key_sha1[0] = key_sha1[0];
  line_to_replace->key_sha1[1] = key_sha1[1];
  line_to_replace->size_in_bytes = data_len;
  line_to_replace->flags = 0; // We currently don't have any flags

  // Save the actual file
  saveDataFileForKey(cache, key_sha1, data, data_len);
}

DCData DCLookup(DCCache cache, char *key) {
  DCData returnme = {.data=NULL, .data_len=0};
  return returnme;
}

void DCPrint(DCCache cache) {
  printf("***PRINTING CACHE DATA***\n");
  printf("\tDirectory Path: %s\n", cache->directory_path);
  printf("\tHeader num_lines: %d\n", cache->header.num_lines);
  printf("\tHeader evict_limit_in_bytes: %d\n", cache->header.evict_limit_in_bytes);
  printf("\tfd: %d\n", cache->fd);
  printf("\tlines address: %llx\n", (uint64_t) cache->lines);
  printf("\tmmap_start address: %llx\n", (uint64_t) cache->mmap_start);
  for (uint32_t i=0; i < cache->header.num_lines; i++) {
    DCCacheLine_t *line = cache->lines + i;
    if (line->last_access_time_in_ms_from_epoch == 0) {
      printf("\t\tLine %03d: EMPTY\n", i);
    } else {
      printf("\t\tLine %03d: %llx%llx | %llu ms | %x flags | %u bytes\n", i, line->key_sha1[0],
             line->key_sha1[1], line->last_access_time_in_ms_from_epoch, line->flags,
             line->size_in_bytes);
    }
  }
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


static void computeLookupIndiciesForKey(uint64_t key_sha1[2], uint32_t indicies[NUM_LOOKUP_INDICIES], uint32_t num_lines) {
  uint32_t *key_in_32_bit_chunks = (uint32_t*) key_sha1;
  for (int i=0; i < NUM_LOOKUP_INDICIES; i++) {
    indicies[i] = key_in_32_bit_chunks[i] % num_lines;
  }
}

// Remove the file associated with this line
static void removeFileForLine(DCCache cache, DCCacheLine_t *line) {
  char path_to_remove[computeMaxFilePathSize(cache->directory_path)];
  pathForSHA1(cache, line->key_sha1, path_to_remove);
  printf("Dry Run: Would delete file '%s'\n", path_to_remove);
}

static void SHA1ForKey(char *key, uint64_t sha1[2]) {
  SHA1Context ctx;
  SHA1Reset(&ctx);
  SHA1Input(&ctx, (const unsigned char*) key, strlen(key));
  SHA1Result(&ctx);
  memcpy(sha1, ctx.Message_Digest, sizeof(uint64_t) * 2);
}

static void pathForSHA1(DCCache cache, uint64_t sha1[2], char *dest) {
  //TODO: USE SUBDIRECTORIES
  sprintf(dest, "%s/%llx%llx.cache_data", cache->directory_path, sha1[0], sha1[1]);
}

static uint64_t currentTimeInMSFromEpoch() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((uint64_t) (tv.tv_sec)) * 1000 +  ((uint64_t) (tv.tv_usec/1000));
}

/***DCAdd Helpers***/

/* Try to determine the best line to save the data to. The best one is based at look at all the
 * indicies in the associative set and picking either:
 * 1. The first empty one
 * 2. The one with the oldest last_access_time_in_ms_from_epoch
 */ 
static DCCacheLine_t *findBestLineToWriteKeyTo(DCCache cache, uint64_t key_sha1[2]) {
  uint32_t lookup_idxs[NUM_LOOKUP_INDICIES];
  computeLookupIndiciesForKey(key_sha1, lookup_idxs, cache->header.num_lines);
  DCCacheLine_t *best_line = cache->lines + lookup_idxs[0];

  for(int i=0; i < NUM_LOOKUP_INDICIES; i++) {
    uint32_t idx = lookup_idxs[i];
    DCCacheLine_t *line = cache->lines + idx;

    //If this line is empty, we can break
    if (line->last_access_time_in_ms_from_epoch == UNUSED_LAST_ACCESS_TIME) {
      best_line = line;
      break; 
    }

    //This is the oldest line we've seen so far
    if (line->last_access_time_in_ms_from_epoch < best_line->last_access_time_in_ms_from_epoch) {
      best_line = line;
    }
  }
  return best_line;
}

/* Take the provided cache/sha1 to compute a path for the data and save it to a file there
 */ 
static void saveDataFileForKey(DCCache cache, uint64_t sha1[2],  uint8_t *data, uint64_t data_len) {
  char file_path[computeMaxFilePathSize(cache->directory_path)];
  pathForSHA1(cache, sha1, file_path);

  FILE *outfile = fopen(file_path, "w");
  fwrite(data, sizeof(uint8_t), data_len, outfile);
  fclose(outfile);
}
