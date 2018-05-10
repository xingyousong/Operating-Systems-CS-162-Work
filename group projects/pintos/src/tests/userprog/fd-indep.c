/* Tests independence of file descriptors of the same file */

#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  int handle1 = open ("sample.txt");
  int handle2 = open ("sample.txt");
  if (handle1 == handle2)
    fail ("both open() calls returned %d", handle1);
  msg ("tell = %d", tell (handle2));
  seek (handle1, 4);
  msg ("tell = %d", tell (handle2));
}
