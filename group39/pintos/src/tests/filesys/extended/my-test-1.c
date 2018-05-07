/*
Test your buffer cache’s effectiveness by measuring its cache hit rate.
First, reset the buffer cache. Open a file and read it sequentially,
to determine the cache hit rate for a cold cache. Then, close it, re-open it,
and read it sequentially again, to make sure that the cache hit rate improves.
*/

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  int fd = open("test.txt");

  // Reset the cache
  reset_cnt();

  static char buf[10000];
  read(fd, &buf, 10000);

  int coldHitRate = cache_hit_cnt();

  close(fd);

  fd = open("test.txt");
  read(fd, &buf, 10000);

  int newHitRate = cache_hit_cnt();

  msg ("improvedHitRate = %d", (newHitRate > coldHitRate ? 1 : 0));

  close(fd);

}
