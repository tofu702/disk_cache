#include <stdio.h>
#include <string.h>

#include "disk_cache.h"


int createTest() {
  DCCache cache = DCMake("/tmp", 16);
  DCCloseAndFree(cache);
  printf("createTest Complete");
}

int addTest() {
  char *test_key = "TEST KEY";
  char *test_val = "TEST VAL";

  DCCache cache = DCMake("/tmp", 16);
  DCCloseAndFree(cache);

  DCCache cache2 = DCLoad("/tmp");
  DCAdd(cache2, test_key, test_val, strlen(test_val));

  return 0;
}

int main(int argc, char **argv) {
  createTest();
  addTest();
  return 0;
}
