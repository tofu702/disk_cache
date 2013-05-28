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
  DCAdd(cache2, test_key, test_val, strlen(test_val) + 1); //+1 to capture the \0
  DCCloseAndFree(cache2);

  DCCache cache3 = DCLoad("/tmp");
  DCData result = DCLookup(cache3, test_key);
  DCCloseAndFree(cache3);

  if (strncmp(result->data, test_val, strlen(test_val)) == 0) {
    printf("addTest passed '%s' == '%s'\n", test_val, result->data);
    return 0;
  } else {
    printf("addTest failed '%s' != '%s'\n", test_val, result->data);
    return 0;
  }
}

int main(int argc, char **argv) {
  createTest();
  addTest();
  return 0;
}
