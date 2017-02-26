#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <unistd.h>

#include "test_helpers.h"
#include "disk_cache.h"

#define WORKING_PATH  "/tmp/dc_test"
#define NON_EXISTANT_PATH WORKING_PATH "/non_existant"
#define NON_WRITABLE_PATH WORKING_PATH "/not_writable"
#define CACHE_FN "cache_data"

int createTest() {
  DCCache no_cache = DCLoad(NON_EXISTANT_PATH);
  if (no_cache != NULL) {
    printf("FAILED: Loaded non-existant cache, createTest failed\n");
    return 1;
  }
  
  DCCache cache = DCMake(WORKING_PATH, 16, 0);
  DCCloseAndFree(cache);
  printf("PASSED: createTest\n");

  // If it didn't crash here, we'll assume it worked
  return 0;
}

int createFailsIfPathNotWritable() {
  DCCache no_cache;
  mkdir(NON_WRITABLE_PATH, 0555);
  no_cache = DCMake(NON_WRITABLE_PATH, 16, 0);

  chmod(NON_WRITABLE_PATH, 0777);
  recursiveDeletePath(NON_WRITABLE_PATH);

  if (no_cache) {
    printf("FAILED: createFailsIfPathNotWritable Created a cache when supposed to be impossible\n");
    return 1;
  } else {
    printf("PASSED: createFailsIfPathNotWritable\n");
    return 0;
  }
}

int loadAndAddWithCorruptCacheDataFileTest() {
  char path[256];
  FILE *outfile;

  sprintf(path, "%s/%s", WORKING_PATH, CACHE_FN);
  outfile = fopen(path, "w");
  fclose(outfile);

  fprintf(stderr, "** IGNORE THIS: ");
  DCCache cache = DCLoad(WORKING_PATH);
  if (!cache) {
    return 0;
  } else {
    printf("Loaded the cache when we shouldn't have\n");
    return 1;
  }
}

int simpleAddTest() {
  char *test_key = "TEST KEY";
  char *test_val = "TEST VAL";

  DCCache cache = DCMake(WORKING_PATH, 16, 0);
  DCCloseAndFree(cache);

  DCCache cache2 = DCLoad(WORKING_PATH);
  DCAdd(cache2, test_key, (uint8_t *)test_val, strlen(test_val) + 1); //+1 to capture the \0
  DCCloseAndFree(cache2);

  DCCache cache3 = DCLoad(WORKING_PATH);
  DCData result = DCLookup(cache3, test_key);
  DCCloseAndFree(cache3);

  if (strncmp((char *)result->data, test_val, strlen(test_val)) == 0) {
    printf("PASSED: addTest '%s' == '%s'\n", test_val, result->data);
    DCDataFree(result);
    return 0;
  } else {
    printf("FAILED: addTest '%s' != '%s'\n", test_val, result->data);
    return 1;
  }
}

int addTestWithOverwrites() {
  DCCache cache = DCMake(WORKING_PATH, 2, 0);
  // It should evict the oldest
  DCAdd(cache, "key1", (uint8_t *)"val1", 5);
  usleep(2000);
  DCAdd(cache, "key2", (uint8_t *)"val2", 5);
  usleep(2000);
  DCAdd(cache, "key3", (uint8_t *) "val3", 5);

  DCData r1 = DCLookup(cache, "key1");
  DCData r2 = DCLookup(cache, "key2");
  DCData r3 = DCLookup(cache, "key3");
  DCCloseAndFree(cache);

  if (r1) {
    printf("Test Failure: Should have deleted 'key1'\n");
    return 1;
  }
  if (strcmp((char *) r2->data, "val2") != 0) {
    printf("FAILED: Should have found 'val2' as value for 'key2', instead got '%s'\n",
           (char *) r2->data);
    return 1;
  }
  if (strcmp((char *) r3->data, "val3") != 0) {
    printf("FAILED: Should have found 'val3' as value for 'key3', instead got '%s'\n",
           (char *) r3->data);
    return 1;
  }

  DCDataFree(r2);
  DCDataFree(r3);

  printf("PASSED: addTestWithOverwrites\n");
  return 0;
}

int addRecoversIfDirectoryDoesntExist() {
  char *test_key = "TEST KEY";
  char *test_val = "TEST VAL";
  bool add_success;

  DCCache cache = DCMake(WORKING_PATH, 16, 0);
  DCCloseAndFree(cache);

  DCCache cache2 = DCLoad(WORKING_PATH);
  recursiveDeletePath(WORKING_PATH"/2b");
  add_success = DCAdd(cache2, test_key, (uint8_t *)test_val, strlen(test_val) + 1);
  DCCloseAndFree(cache2);

  if (add_success) {
    printf("PASSED: addRecoversIfDirectoryDoesntExist\n");
    return 0;
  } else {
    printf("FAIL: addRecoversIfDirectoryDoesntExist should have worked, but fails\n");
    return 1;
  }
}

int addFailsIfDirectoryNotExistsAndNotWriteable() {
  char *test_key = "TEST KEY";
  char *test_val = "TEST VAL";
  bool add_success;

  mkdir(NON_WRITABLE_PATH, 0777);
  DCCache cache = DCMake(NON_WRITABLE_PATH, 16, 0);
  DCCloseAndFree(cache);

  DCCache cache2 = DCLoad(NON_WRITABLE_PATH);
  recursiveDeletePath(NON_WRITABLE_PATH"/2b");
  chmod(NON_WRITABLE_PATH, 0555);
  add_success = DCAdd(cache2, test_key, (uint8_t *)test_val, strlen(test_val) + 1);
  DCCloseAndFree(cache2);
  chmod(NON_WRITABLE_PATH, 0777);

  if (!add_success) {
    printf("PASSED: addFailsIfDirectoryNotExistsAndNotWriteable\n");
    return 0;
  } else {
    printf("FAILED: addFailsIfDirectoryNotExistsAndNotWriteable: add worked but should have failed\n");
    return 1;
  }
}

int testLookupSetsAccessTimeAndReplacesEarliestAccessed() {
  DCCache cache = DCMake(WORKING_PATH, 2, 0);
  DCAdd(cache, "key1", (uint8_t *)"val1", 5);
  DCAdd(cache, "key2", (uint8_t *)"val2", 5);
  usleep(2000);
  DCDataFree(DCLookup(cache, "key2"));
  usleep(2000);
  DCDataFree(DCLookup(cache, "key1"));
  usleep(2000);
  DCAdd(cache, "key3", (uint8_t *) "val3", 5);
  
  DCData r1 = DCLookup(cache, "key1");
  DCData r2 = DCLookup(cache, "key2");
  DCData r3 = DCLookup(cache, "key3");
  DCCloseAndFree(cache);
  
  if (!r1) {
    printf("FAILED: Should have gotten a non-null r1\n");
    return 1;
  }
  if (strcmp((char *)r1->data, "val1") != 0) {
    printf("FAILED: Should have found 'val1' for key 'key1', instead got '%s'\n",
           (char *) r1->data);
    return 1;
  }
  if (r2) {
    printf("FAILED: Should have delete 'key2'\n");
    return 1;
  }
  if (strcmp((char *) r3->data, "val3") != 0) {
    printf("FAILED: Should have found 'val3' as value for 'key3', instead got '%s'\n",
           (char *) r3->data);
    return 1;
  }

  DCDataFree(r1);
  DCDataFree(r3);

  printf("PASSED: testLookupSetsAccessTimeAndReplacesEarliestAccessed\n");
  return 0;
}

int evictionTest() {
  DCCache cache = DCMake(WORKING_PATH, 64, 11);
  DCAdd(cache, "key1", (uint8_t *)"val1", 5);
  usleep(2000);
  DCAdd(cache, "key2", (uint8_t *)"v2", 3);
  usleep(2000);
  // Both Keys should be here
  DCData r1 = DCLookup(cache, "key1");
  usleep(2000);
  DCData r2 = DCLookup(cache, "key2");
  DCCloseAndFree(cache);

  //At this point key2 should have a newer access time than key1

  if (!r1 || !r2) {
    printf("Test Failure, both r1 & r2 should have existed\n");
    return 1;
  }
  DCDataFree(r1);
  DCDataFree(r2);

  DCCache cache2 = DCLoad(WORKING_PATH);
  usleep(2000);
  // Will trigger an eviction down to size 8-5 = 3 (IE: nuke both val1)
  DCAdd(cache2, "key3", (uint8_t *)"v3", 3); 
  usleep(2000);

  r1 = DCLookup(cache2, "key1");
  usleep(2000);
  r2 = DCLookup(cache2, "key2");
  usleep(2000);
  DCData r3 = DCLookup(cache2, "key3");


  if (r1) {
    printf("Test failure: r1 should have been evicted\n");
    return 1;
  }
  if (!r2) {
    printf("Test failure: r2 should not have been evicted\n");
    return 1;
  }

  usleep(2000);
  // Will trigger an eviction down to size 3 (IE: nukes v2)
  DCAdd(cache2, "key4", (uint8_t *)"val4", 5);

  // We don't need sleeps here because we're done with adds
  r1 = DCLookup(cache2, "key1");
  r2 = DCLookup(cache2, "key2");
  r3 = DCLookup(cache2, "key3");
  DCData r4 = DCLookup(cache2, "key4");
  DCCloseAndFree(cache2);

  if (r1 || r2) {
    printf("FAILED: r1 & r2 should have been evicted\n");
    return 1;
  }
  if (!r3) {
    printf("FAILED: r3 should not have been evicted\n");
    return 1;
  }
  if (!r4) {
    printf("FAILED: r4 should be here?!?\n");
    return 1;
  }
  
  DCDataFree(r3);
  DCDataFree(r4);
  printf("PASSED: evictionTest\n");
  return 0;
}

int main(int argc, char **argv) {
  //SETUP
  mkdir(WORKING_PATH, 0777);

  // BEGIN TESTS
  createTest();
  createFailsIfPathNotWritable();
  loadAndAddWithCorruptCacheDataFileTest();
  simpleAddTest();
  addTestWithOverwrites();
  addRecoversIfDirectoryDoesntExist();
  addFailsIfDirectoryNotExistsAndNotWriteable();
  testLookupSetsAccessTimeAndReplacesEarliestAccessed();
  evictionTest();

  // Cleanup
  recursiveDeletePath(WORKING_PATH);

  return 0;
}
