#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <sys/stat.h>
#include <sys/time.h>

static inline double fTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((double) (tv.tv_sec))  +  ((double) (tv.tv_usec/1000000.0));
}

static void recursiveDeletePath(char *path) {
  char buf[256 + strlen(path)];
  sprintf(buf, "/bin/bash -c 'rm -rf %s'", path);
  pclose(popen(buf, "r"));
}


#endif

