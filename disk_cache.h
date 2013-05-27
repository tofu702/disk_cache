/* 
 *
 */

#ifndef DISKCACHE_H_
#define DISKCACHE_H_

#include <stdint.h>

/* Representation of a single cache line. Each cache line is 32 bytes (256 bit). 
 * This means we should be able to achieve a packing of 32 cache lines per KB.
 */ 
typedef struct {
  uint64_t last_access_time_in_ms_from_epoch; // 8 bytes
  uint64_t key_sha1[2]; // 16 bytes
  uint32_t size_in_bytes; // 4 byte
  uint32_t flags; // 4 bytes
} DCCacheLine_t;


typedef struct {
  uint8_t *data;
  uint64_t data_len;
} DCData;

typedef struct {
  DCCacheLine_t *lines;
  char *directory_path;
  uint32_t num_lines;
  uint32_t fd;
} DCCache_t;

typedef DCCache_t *DCCache;



DCCache DCMake(char *cache_directory_path, uint32_t num_lines);

DCCache DCLoad(char *cache_directory_path);

void DCCloseAndFree(DCCache cache);

void DCAdd(DCCache cache, char *key, uint8_t *data, uint64_t data_len);

DCData DCLookup(DCCache cache, char *key);

#endif
