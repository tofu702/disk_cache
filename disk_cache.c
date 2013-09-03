#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>>
#include <sys/mman.h>
#include <sys/stat.h>
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
#define EVICT_TO_THIS_RATIO .75 //Eviction is expensive, so we want to evict more than what we need

/***INTERNAL STRUCTS***/
typedef struct {
  DCCacheLine_t *line;
  int line_idx; //Probably not needed
  uint64_t last_access_time_in_ms_from_epoch;
} LineSortable_t;

/***PREPROCESSOR FUNCTION DECLARATIONS***/
static size_t computeMaxFilePathSize(char *cache_directory_path);
static void computeCachePath(char *cache_directory_path, char *dest, int dest_len);
static void createDataFile(char *file_path, uint32_t num_lines, uint64_t max_bytes);
static void createSubDirs(char *cache_directory_path);
static void computeLookupIndiciesForKey(uint64_t key_sha1[2], uint32_t indicies[NUM_LOOKUP_INDICIES], uint32_t num_lines);
static void SHA1ForKey(char *key, uint64_t sha1[2]);
static void pathForSHA1(DCCache cache, uint64_t sha1[2], char *dest);
static void removeLine(DCCache cache, DCCacheLine_t *line);
static void removeFileForLine(DCCache cache, DCCacheLine_t *line);
static uint64_t currentTimeInMSFromEpoch();
static void recomputeCacheSizeFromLines(DCCache cache);
static void maybeEvict(DCCache cache, uint64_t proposed_increase_bytes);

//DCAdd Helpers
static DCCacheLine_t *findBestLineToWriteKeyTo(DCCache cache, uint64_t key_sha1[2]);
static void saveDataFileForKey(DCCache cache, uint64_t sha1[2], uint8_t *data, uint64_t data_len);

//DCLookup Helpers
static DCCacheLine_t *findLineThatMatchesKey(DCCache cache, uint64_t key_sha1[2]);
static DCData readDataFileForKey(DCCache cache, uint64_t key_sha1[2]);

//Evict Helpers
static LineSortable_t *lineSortablesFromOldestToNewest(DCCache cache);
static int sortableCompareFunc(const void *a, const void *b);


/***IMPLEMENTATION OF PUBLIC FUNCTIONS***/


DCCache DCMake(char *cache_directory_path, uint32_t num_lines, uint64_t max_bytes) {
  size_t file_path_size = computeMaxFilePathSize(cache_directory_path);
  char file_path[file_path_size];
  computeCachePath(cache_directory_path, file_path, file_path_size);
  createDataFile(file_path, num_lines, max_bytes);
  createSubDirs(cache_directory_path);

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
  cache->mmap_start = mmap(0, total_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, cache->fd, 0);
  if (cache->mmap_start == MAP_FAILED) {
    fprintf(stderr, "Map Failed! fd=%d, lines_size=%d, error:%s\n", (int)cache->fd, (int)lines_size, 
            strerror(errno));
    assert(0); // TODO: This should never happen, but fail more gracefully
  }
  cache->lines = cache->mmap_start + lines_start_offset;
  recomputeCacheSizeFromLines(cache);
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

  maybeEvict(cache, data_len);

  DCCacheLine_t *line_to_replace = findBestLineToWriteKeyTo(cache, key_sha1);
  if (line_to_replace->last_access_time_in_ms_from_epoch != UNUSED_LAST_ACCESS_TIME) {
    removeLine(cache, line_to_replace);
  }

  // Set the line state
  line_to_replace->last_access_time_in_ms_from_epoch = currentTimeInMSFromEpoch();
  line_to_replace->key_sha1[0] = key_sha1[0];
  line_to_replace->key_sha1[1] = key_sha1[1];
  line_to_replace->size_in_bytes = data_len;
  line_to_replace->flags = 0; // We currently don't have any flags
  
  // Increment the cache size
  cache->current_size_in_bytes += data_len;

  // Save the actual file
  saveDataFileForKey(cache, key_sha1, data, data_len);
}

DCData DCLookup(DCCache cache, char *key) {
  uint64_t key_sha1[2];
  DCCacheLine_t *line;
  
  SHA1ForKey(key, key_sha1);

  line = findLineThatMatchesKey(cache, key_sha1);

  // None was found we don't have this data
  if (!line) {
    return NULL;
  }

  //Update the line's last accessed time
  line->last_access_time_in_ms_from_epoch = currentTimeInMSFromEpoch();

  //Return the file
  // TODO: Handle a NULL return value by clearing the cache line
  return readDataFileForKey(cache, key_sha1);
}

void DCEvictToSize(DCCache cache, uint64_t allowed_bytes) {
  // We don't have evict if we are already below allowed_bytes
  if (cache->current_size_in_bytes <= allowed_bytes) {
    return;
  }

  LineSortable_t *sortables = lineSortablesFromOldestToNewest(cache);
  //Sort {line_pos, last_access_time_in_ms_from_epoch} by last_access_time_in_ms_from_epoch asc
    // Keep deleting until we're under allowed_bytes
  for (int i=0; i < cache->header.num_lines; i++) {
    // We've hit the target size; we're done
    if (cache->current_size_in_bytes <= allowed_bytes) {
      break;
    }

    // Otherwise let's cheap lopping lines out of the cache
    removeLine(cache, sortables[i].line);
  }

  free(sortables);
}

void DCDataFree(DCData data) {
  free(data->data);
  free(data);
}


/***Public debugging functions***/


void DCPrint(DCCache cache) {
  printf("***PRINTING CACHE DATA***\n");
  printf("\tDirectory Path: %s\n", cache->directory_path);
  printf("\tHeader num_lines: %d\n", cache->header.num_lines);
  printf("\tHeader max_bytes: %llu\n", cache->header.max_bytes);
  printf("\tfd: %d\n", cache->fd);
  printf("\tcurrent_size_in_bytes: %llu\n", cache->current_size_in_bytes);
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

static void createDataFile(char *file_path, uint32_t num_lines, uint64_t max_bytes) {
  FILE *outfile = fopen(file_path, "w");

  // Create the header
  DCCacheHeader_t header = {.num_lines=num_lines, .max_bytes=max_bytes};
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


// We want to create create subdirs from 00 to FF
static void createSubDirs(char *cache_directory_path) {
  char subdir_path[strlen(cache_directory_path) + 16];
  for (int i=0; i < 256; i++) {
    sprintf(subdir_path, "%s/%02x", cache_directory_path, i);
    mkdir(subdir_path, 0777);
  }
}

static void computeLookupIndiciesForKey(uint64_t key_sha1[2], uint32_t indicies[NUM_LOOKUP_INDICIES], uint32_t num_lines) {
  uint32_t *key_in_32_bit_chunks = (uint32_t*) key_sha1;
  for (int i=0; i < NUM_LOOKUP_INDICIES; i++) {
    indicies[i] = key_in_32_bit_chunks[i] % num_lines;
  }
}

static void removeLine(DCCache cache, DCCacheLine_t *line) {
  cache->current_size_in_bytes -= line->size_in_bytes;
  removeFileForLine(cache, line);
  
  // Zero the line (is bzero faster?)
  line->last_access_time_in_ms_from_epoch = UNUSED_LAST_ACCESS_TIME;
  line->key_sha1[0] = 0;
  line->key_sha1[1] = 0;
  line->size_in_bytes = 0;
  line->flags = 0;
}

// Remove the file associated with this line
static void removeFileForLine(DCCache cache, DCCacheLine_t *line) {
  char path_to_remove[computeMaxFilePathSize(cache->directory_path)];
  pathForSHA1(cache, line->key_sha1, path_to_remove);
  remove(path_to_remove);
}

static void SHA1ForKey(char *key, uint64_t sha1[2]) {
  SHA1Context ctx;
  SHA1Reset(&ctx);
  SHA1Input(&ctx, (const unsigned char*) key, strlen(key));
  SHA1Result(&ctx);
  memcpy(sha1, ctx.Message_Digest, sizeof(uint64_t) * 2);
}

static void pathForSHA1(DCCache cache, uint64_t sha1[2], char *dest) {
  uint8_t subdir = sha1[0] / 65536 / 65536 / 65536 / 256; // Use division to be byte order agnostic
  sprintf(dest, "%s/%02x/%016llx%016llx.cache_data", cache->directory_path, subdir, sha1[0], sha1[1]);
}

static uint64_t currentTimeInMSFromEpoch() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((uint64_t) (tv.tv_sec)) * 1000 +  ((uint64_t) (tv.tv_usec/1000));
}

static void recomputeCacheSizeFromLines(DCCache cache) {
  uint64_t total_size_in_bytes = 0;
  uint32_t num_lines = cache->header.num_lines; //Cache this here since it's in the comparison
  for (int i=0; i < num_lines; i++) {
    total_size_in_bytes += cache->lines[i].size_in_bytes;
  }
  cache->current_size_in_bytes = total_size_in_bytes;
}

/* Evict the contents of the of the cache if the combined size of the current data and the proposed
 * element to be inserted is greater than the cache size.
 */
static void maybeEvict(DCCache cache, uint64_t proposed_increase_bytes) {
  // Never evict if eviction is turned off
  if (cache->header.max_bytes == 0) {
    return;
  }

  // There's enough space for both the current data and the proposed increase
  if ((cache->current_size_in_bytes + proposed_increase_bytes) < cache->header.max_bytes) {
    return;
  }
  
  // Ok, we have to evict. After the insertion we want the size to be:
  //   max_bytes * EVICT_TO_THIS_RATIO
  // So we need to decrease that number by proposed_increase_bytes
  uint64_t post_increase_desired_size = cache->header.max_bytes * EVICT_TO_THIS_RATIO;
  uint64_t current_desired_size = post_increase_desired_size - proposed_increase_bytes;
  DCEvictToSize(cache, current_desired_size);
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


/***DCLookup Helpers***/


/* Return the line that exactly matches the provided key_sha1. If none is found we return NULL.
 */ 
static DCCacheLine_t *findLineThatMatchesKey(DCCache cache, uint64_t key_sha1[2]) {
  uint32_t indicies[NUM_LOOKUP_INDICIES];

  computeLookupIndiciesForKey(key_sha1, indicies, cache->header.num_lines);
  for (int i=0; i < NUM_LOOKUP_INDICIES; i++) {
    DCCacheLine_t *line = cache->lines + indicies[i];
    if(line->key_sha1[0] == key_sha1[0] && line->key_sha1[1] == key_sha1[1]) {
      return line;
    }
  }

  return NULL;
}

/* Read the data for the file that the key points to and return a DCData if it's readable or NULL if it's not
 */
static DCData readDataFileForKey(DCCache cache, uint64_t key_sha1[2]) {
  DCData returnme;
  char file_path[computeMaxFilePathSize(cache->directory_path)];
  pathForSHA1(cache, key_sha1, file_path);
  
  FILE *infile = fopen(file_path, "r");
  
  // For some reason we couldn't open the file
  if (!infile) {
    fprintf(stderr, "Unable to open cache file '%s'\n", file_path);
    return NULL;
  }

  returnme = calloc(1, sizeof(DCData_t));
  // Determine the file's size, not that this only works up to 2 GB on 32 bit machines
  // TODO: Change this to fgetpos/fsetpos to support larger files
  fseek(infile, 0, SEEK_END);
  returnme->data_len = ftell(infile);
  fseek(infile, 0, SEEK_SET);

  returnme->data = calloc(returnme->data_len, sizeof(uint8_t));

  // TODO: Use a less nuclear error handling mechanism
  assert(returnme->data); // Will exit if a null pointer (not enough memory)

  fread(returnme->data, sizeof(uint8_t), returnme->data_len, infile);
  fclose(infile);

  return returnme;
}


/***EVICTION HELPERS***/

LineSortable_t *lineSortablesFromOldestToNewest(DCCache cache) {
  int num_lines = cache->header.num_lines;
  LineSortable_t *sortables = calloc(cache->header.num_lines, sizeof(LineSortable_t));

  // Construct the sortables
  for (int i=0; i < num_lines; i++) {
    sortables[i].line_idx = i;
    sortables[i].line = cache->lines + i;
    sortables[i].last_access_time_in_ms_from_epoch = cache->lines[i].last_access_time_in_ms_from_epoch;
  }

  // Sort the sortables (what a novel idea)
  qsort(sortables, num_lines, sizeof(LineSortable_t), sortableCompareFunc);

  return sortables;
}

static int sortableCompareFunc(const void *a, const void *b) {
  LineSortable_t *left = (LineSortable_t *) a;
  LineSortable_t *right = (LineSortable_t *) b;
  return left->last_access_time_in_ms_from_epoch - right->last_access_time_in_ms_from_epoch;
}
