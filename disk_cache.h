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
  uint32_t evict_limit_in_bytes;
} DCCacheHeader_t;

/* Representation of a single cache line. Each cache line is 32 bytes (256 bit). 
 * This means we should be able to achieve a packing of 32 cache lines per KB.
 */ 
typedef struct __attribute__ ((__packed__)) {
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
} DCData;

typedef struct {
  DCCacheHeader_t header;
  DCCacheLine_t *lines;
  char *directory_path;
  uint32_t fd;
  void *mmap_start; 
} DCCache_t;

typedef DCCache_t *DCCache;


/***Primary Functions***/


DCCache DCMake(char *cache_directory_path, uint32_t num_lines);

DCCache DCLoad(char *cache_directory_path);

void DCCloseAndFree(DCCache cache);

void DCAdd(DCCache cache, char *key, uint8_t *data, uint64_t data_len);

DCData DCLookup(DCCache cache, char *key);


/***Debugging Functions***/
/* Print the cache contents to stdout */
void DCPrint(DCCache cache);

#endif
