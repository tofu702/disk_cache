#include <stdio.h>

#include "disk_cache.h"


int main(int argc, char **argv) {
  DCCache cache = DCMake("/tmp", 16);
  DCCloseAndFree(cache);
  DCPrint(cache);
  printf("Test Complete");
  return 0;
}
