#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void)
{
	create("write1.txt", 0);
	int fd = open("write1.txt");
	int i;
	for (i = 0; i < 65536; i += 1) {
		write(fd, "f", 1);
	}
	seek(fd, 0);
	static char buf[1];
	for (i = 0; i < 65536; i += 1) {
		read(fd, buf, 1);
	}
	int writeCalls1 = write_block_cnt();
	close(fd);

	create("write2.txt", 0);
	fd = open("write2.txt");
	for (i = 0; i < 65536 * 2; i += 1) {
		write(fd, "f", 1);
	}
	seek(fd, 0);
	for (i = 0; i < 65536 * 2; i += 1) {
		read(fd, buf, 1);
	}
	int writeCalls2 = write_block_cnt() - writeCalls1;

	if (writeCalls2 - writeCalls1 <= 130)
		msg ("writeCalls succeed");
	else
		msg ("writeCalls exceeded acceptable amount");
}
