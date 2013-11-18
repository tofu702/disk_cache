/* disk_cache.h
 * Principle header file for disk_cache
 */


#ifndef DISKCACHE_H_
#define DISKCACHE_H_

#include <stdint.h>


/*****NOTE*****/
/* The below data structures should not normally be accessed directly, but are provided here for
 * non-production use (ex: debugging).
 */
/**************/


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
  int fd;
  uint64_t current_size_in_bytes;
  void *mmap_start;
} DCCache_t;


//Abstract Types
typedef DCData_t *DCData;
typedef DCCache_t *DCCache;


/*****Production API Functions*****/
/* The below functions specify the production API for the disk_cache and should be the sole means
 * of accessing and manipulating the contents of the cache.
 */
/**********************************/


/* Create a DCCache. Arguments
 * -cache_directory_path: A (preferably empty) directory where the cache data will be stored
 * -num_lines: The maximum number of elements to store in the cache, dictates memory usage
 * -max_bytes: The maximum number of bytes to store in the cache. SPECIFY 0 FOR NO LIMIT
 */
DCCache DCMake(char *cache_directory_path, uint32_t num_lines, uint64_t max_bytes);

/* Load a pre-existing disk cache. If the disk cache doesn't exist or is corrupt, this function
 * will return NULL.
 * Arguments:
 * -cache_directory_path: A directory where the cache data is stored
 * Returns: An instance of a DCCache. This DCCache instance must be disposed of with DCCloseAndFree
 * or memory will leak.
 */
DCCache DCLoad(char *cache_directory_path);

/* Free all state associated with a cache and close all open files that it is using.
 * Arguments:
 * -cache: A DCCache instance
 */
void DCCloseAndFree(DCCache cache);

/* Add a key and associated data (value) to the cache. The should the cache not have room for this
 * key, the cache will automatically evict older keys to make room.
 * Arguments:
 * -cache: A DCCache instance
 * -key: A null terminated string for a key to add
 * -data: A byte array of the data to store in the cache
 * -data_len: The length of data, in bytes
 */
void DCAdd(DCCache cache, char *key, uint8_t *data, uint64_t data_len);

/* Lookup a key in the provided DCCache.
 * Arguments:
 * -cache: An instance of a DCCache
 * -key: A null terminated string for a key to look up
 * Returns: If the key is found, a DCData instance. This DCData instance must be disposed of with
 * DCDataFree. If the key is not found, NULL is returned instead.
 */
DCData DCLookup(DCCache cache, char *key);

/* If the key exists in the cache, remove it
 * Arguments:
 * -cache: A DCCache instance
 * -key: A key to remove
 */
void DCRemove(DCCache cache, char *key);

/* Evict to the specified number of bytes. This normally isn't needed but could be done to clear
 * a lot of space in the cache before adding a lot of elements. Oldest elements are always evicted
 * first.
 * Arguments:
 * -cache: The cache
 * -allowedBytes: The maximum number of bytes that will still be in the cache after the operation
 */
void DCEvictToSize(DCCache cache, uint64_t allowed_bytes);

/* Free all memory associated with a DCData abstract type
 * Arguments
 * -data: A DCData abstract type
 */
void DCDataFree(DCData data);


/*****Debugging Functions*****/
/* These functions are provided for non=production debugging but are too slow for production use.
 */
/*****************************/


/* Print the cache contents to stdout */
void DCPrint(DCCache cache);

/* How many items are currently stored in the cache.
 * NOTE: This is a slow operation and should not be called in production
 */
int DCNumItems(DCCache cache);

#endif
