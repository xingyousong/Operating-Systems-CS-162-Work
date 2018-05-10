/* Child process run by wait-granchild test. */
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

const char *test_name = "child-parent";

int
test_main (void)
{
  msg
  return (int) exec ("child-simple");
}
