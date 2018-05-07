/* Child process run by wait-dead */

#include <syscall.h>
#include "wait-dead.h"
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
  lock_acquire(&test_lock);
  exit(39);
}
