/* 
 *
 */

#ifndef DISKCACHE_H_
#define DISKCACHE_H_

#include <stdint.h>

/* The data file format for the DC Data file
 * 1. The DCCacheHeader: A single DCCacheHeader_t
 * 2. A number of DCCacheLine_t structs whose count is specified by the num_lines field of the 
 *    DCCacheHeader_t
 */

/* The representation of the cache header.
 */
typedef struct __attribute__ ((__packed__)) {
  uint32_t num_lines;
  uint64_t max_bytes; // 0 = no limit
} DCCacheHeader_t;

/* Representation of a single cache line. Each cache line is 32 bytes (256 bit). 
 * This means we should be able to achieve a packing of 32 cache lines per KB.
 */ 
typedef struct __attribute__ ((__packed__)) {
  // If this field is 0, this entry is considered unoccupied
  uint64_t last_access_time_in_ms_from_epoch; // 8 bytes

  uint64_t key_sha1[2]; // 16 bytes
  uint32_t size_in_bytes; // 4 byte
  uint32_t flags; // 4 bytes
} DCCacheLine_t;

/* A struct used to return a data result
 */ 
typedef struct {
  uint8_t *data;
  uint64_t data_len;
} DCData_t;

typedef struct {
  DCCacheHeader_t header;
  DCCacheLine_t *lines;
  char *directory_path;
  uint32_t fd;
  uint64_t current_size_in_bytes;
  void *mmap_start; 
} DCCache_t;


//Abstract Types
typedef DCData_t *DCData;
typedef DCCache_t *DCCache;


/***Primary Functions***/


/* Create a DCCache. Arguments
 * -cache_directory_path: A (preferably empty) directory where the cache data will be stored
 * -num_lines: The maximum number of elements to store in the cache, dictates memory usage
 * -max_bytes: The maximum number of bytes to store in the cache. SPECIFY 0 FOR NO LIMIT
 */
DCCache DCMake(char *cache_directory_path, uint32_t num_lines, uint64_t max_bytes);

DCCache DCLoad(char *cache_directory_path);

void DCCloseAndFree(DCCache cache);

void DCAdd(DCCache cache, char *key, uint8_t *data, uint64_t data_len);

DCData DCLookup(DCCache cache, char *key);

/* Evict to the specified number of bytes. This normally isn't needed but could be done to clear
 * a lot of space in the cache before adding a lot of elements. Oldest elements are always evicted
 * first.
 * Arguments:
 * -cache: The cache
 * -allowedBytes: The maximum number of bytes that will still be in the cache after the operation
 */
void DCEvictToSize(DCCache cache, uint64_t allowed_bytes);

void DCDataFree(DCData data);


/***Debugging Functions***/
/* Print the cache contents to stdout */
void DCPrint(DCCache cache);

#endif
