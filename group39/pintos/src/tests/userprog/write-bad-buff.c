/* Tries to write from a buffer that starts in valid memory but exceeds it.
Should exit with exit code -1 */

#include <syscall.h>
#include "tests/userprog/sample.inc"
#include "tests/lib.h"
#include "tests/main.h"

static char *PHYS_BASE = (char *) 0xC0000000;

void
test_main (void)
{
  int handle;
  CHECK (create ("test.txt", sizeof sample - 1), "create \"test.txt\"");
  CHECK ((handle = open ("test.txt")) > 1, "open \"test.txt\"");
  char *buffer = PHYS_BASE - 1;
  write (handle, buffer, 0x41000000);
}
