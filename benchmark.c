#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "disk_cache.h"


#define MAX_KEY_SIZE 8
#define DIR_PATH "/tmp/cache_bench"

/***Function Prototypes***/
double fTime();
double standardBenchmark(int cache_size, int max_bytes, int num_adds, int num_gets, int max_file_size);
void computeKey(int key_num, char dest[MAX_KEY_SIZE]);
uint8_t *dataForKeyNum(int key_num, int max_file_size);
void recursiveDeletePath(char *path);

/* Run the standard benchmark and return the run time
 * This benchmark will create num_adds files up to max_size, add them to the cache
 * and then attempt to get the keys out of the cache num_gets times
 */
double standardBenchmark(int cache_size, int max_bytes, int num_adds, int num_gets, int max_file_size) {
  char keys[num_adds][MAX_KEY_SIZE]; 
  uint8_t *data[num_adds];
  DCCache cache;
  int hit_counter=0, miss_counter=0;
  double start_time, end_of_make_time, end_of_add_time, end_of_get_time, end_of_close_time;

  // Setup
  for (int i=0; i < num_adds; i++) {
    computeKey(i, keys[i]);
    data[i] = dataForKeyNum(i, max_file_size);
  }
  mkdir(DIR_PATH, 0777);

  start_time = fTime();
  cache = DCMake(DIR_PATH, cache_size, max_bytes);

  end_of_make_time = fTime();
  
  for(int i=0; i < num_adds; i++) {
    DCAdd(cache, keys[i], data[i], max_file_size);
  }

  end_of_add_time = fTime();

  srand(1);

  for (int i=0; i < num_gets; i++) {
    int idx = rand() % num_adds;
    DCData result = DCLookup(cache, keys[idx]);
    if (result) {
      hit_counter ++;
      DCDataFree(result);
    } else {
      miss_counter ++;
    }
  }

  end_of_get_time = fTime();

  DCCloseAndFree(cache);

  end_of_close_time = fTime();

  printf("Create time: %f; Add time: %f; Get time: %f; Close time: %f\n", 
        (end_of_make_time - start_time),
        (end_of_add_time - end_of_make_time),
        (end_of_get_time - end_of_add_time),
        (end_of_close_time - end_of_get_time));
  printf("Hit Rate: %f; Add Keys/s: %f; Get Keys/s: %f; Get Hit Keys/s: %f\n", 
         ((double)hit_counter) / num_gets,
         ((double)num_adds) / (end_of_add_time - end_of_make_time),
         ((double)num_gets) / (end_of_get_time - end_of_add_time),
         ((double)hit_counter) / (end_of_get_time - end_of_add_time)
        );


  // Cleanup
  for(int i=0; i < num_adds; i++) {
    free(data[i]);
  }
  recursiveDeletePath(DIR_PATH);
  return 0.0; // TODO: Return something less stupid
}

/***Helpers for standardBenchmark***/

void computeKey(int key_num, char dest[MAX_KEY_SIZE]) {
  sprintf(dest, "%d", key_num);
}

uint8_t *dataForKeyNum(int key_num, int max_file_size) {
  int num_vals = max_file_size / sizeof(uint64_t);
  uint8_t *returnme = calloc(num_vals, sizeof(uint64_t));
  uint64_t *returnme_as_uint64_t_arr = (uint64_t *) returnme;

  for (int i=0; i < num_vals; i++) {
    returnme_as_uint64_t_arr[i] = (uint64_t) i;
  }
  return returnme;
}

/*** main function ***/
int main (int argc, char **argv) {
  printf("Starting benchmark\n");
  standardBenchmark(1024, 0, 1024, 65536, 2048);
}
