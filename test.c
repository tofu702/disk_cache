#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "disk_cache.h"


int createTest() {
  DCCache cache = DCMake("/tmp", 16, 0);
  DCCloseAndFree(cache);
  printf("createTest Complete");
  return 0;
}

int simpleAddTest() {
  char *test_key = "TEST KEY";
  char *test_val = "TEST VAL";

  DCCache cache = DCMake("/tmp", 16, 0);
  DCCloseAndFree(cache);

  DCCache cache2 = DCLoad("/tmp");
  DCAdd(cache2, test_key, (uint8_t *)test_val, strlen(test_val) + 1); //+1 to capture the \0
  DCCloseAndFree(cache2);

  DCCache cache3 = DCLoad("/tmp");
  DCData result = DCLookup(cache3, test_key);
  DCCloseAndFree(cache3);

  if (strncmp((char *)result->data, test_val, strlen(test_val)) == 0) {
    printf("addTest passed '%s' == '%s'\n", test_val, result->data);
    DCDataFree(result);
    return 0;
  } else {
    printf("addTest failed '%s' != '%s'\n", test_val, result->data);
    return 1;
  }
}

int addTestWithOverwrites() {
  DCCache cache = DCMake("/tmp", 2, 0);
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
    printf("Test Failure: Should have found 'val2' as value for 'key2', instead got '%s'\n",
           (char *) r2->data);
    return 1;
  }
  if (strcmp((char *) r3->data, "val3") != 0) {
    printf("Test Failure: Should have found 'val3' as value for 'key3', instead got '%s'\n",
           (char *) r3->data);
    return 1;
  }

  DCDataFree(r2);
  DCDataFree(r3);

  printf("Passed addTestWithOverwrites\n");
  return 0;
}

int testLookupSetsAccessTimeAndReplacesEarliestAccessed() {
  DCCache cache = DCMake("/tmp", 2, 0);
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
    printf("Test Failure: Should have gotten a non-null r1\n");
    return 1;
  }
  if (strcmp((char *)r1->data, "val1") != 0) {
    printf("Test Failure: Should have found 'val1' for key 'key1', instead got '%s'\n",
           (char *) r1->data);
    return 1;
  }
  if (r2) {
    printf("Test failure: Should have delete 'key2'\n");
    return 1;
  }
  if (strcmp((char *) r3->data, "val3") != 0) {
    printf("Test Failure: Should have found 'val3' as value for 'key3', instead got '%s'\n",
           (char *) r3->data);
    return 1;
  }

  DCDataFree(r1);
  DCDataFree(r3);

  printf("Passed testLookupSetsAccessTimeAndReplacesEarliestAccessed\n");
  return 0;
}

int main(int argc, char **argv) {
  createTest();
  simpleAddTest();
  addTestWithOverwrites();
  testLookupSetsAccessTimeAndReplacesEarliestAccessed();
  return 0;
}
