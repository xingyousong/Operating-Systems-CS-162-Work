Final Report for Project 3: File Systems
=========================================

## Group Members

* Jonathan Lin (<jlin703@berkeley.edu>)
* Jim Liu (<jim.liu@berkeley.edu>)
* Mong Ng (<monghim.ng@berkeley.edu>)
* Xingyou Song (<xsong@berkeley.edu>)

## Part 1: Buffer Cache

### Code Edits and Additions
We added a flush function to flush all cache entries when the system shuts down, to pass the persistence tests. Other than that, our design doc was very thorough and no significant changes needed to be made.

### Algorithms
Pretty much the same as doc.

### Synchronization
No change from design doc.


## Part 2: Extensible Files

### Code Edits and Additions
We had to add a lock inside of inode. We also ended up using more direct pointers than we had planned because there was no reason to not use up all the space available in the sector. 

### Algorithms
We made the decision to zero out blocks as we acquired them, which made for easier implementation of a non-sparse file system.

### Synchronization
No change from design doc aside from the lock inside the inode.

## File Directory

### Code Edits and Additions
Instead of using a separate array to map directory file descriptors, all file descriptors (for both files and directories) were placed in one array for simplicity.

Added functions to inode.c and directory.c to be able to edit and access directory positions, and whether or not an inode was a directory or not.

### Algorithms
To follow the file path passed in as arguments was probably the trickiest part of this task, and required a lot more thought than was put in the design doc. Specifically, deciding whether to return an inode, how to handle opening/closing all the different structs, and how to suit the demands of all the different sycalls. We ended up taking in an inode and returning (through setting a memory address) an opened directory and a boolean indicating if the last part was a file or not.

We also had to change the way we assigned file descriptors by allowing for use of file descriptor numbers that had been used before but had been closed, in order to pass the `dir-vine` test.

### Synchronization
We use the lock from task 2 for synchronization in this task as well.

## Reflection

### Member Tasks

**Jonathan Lin**: I worked on the directory portion, including handling all the new syscalls and functionality for subdirectories for the old syscalls, debugged a bunch and helped write test cases.

**Jim Liu**: I worked on the extensible file portion and for writing cache tests. I wrote code for extending the inode, fetching a block and creating, reading and writing to an inode. Once we were passing all autograder tests, I wrote the cache hit-rate test which passed and the 0 read block-cnt test that ended up not passing. 

**Mong Ng**: I worked on task 1, and helped debug task 2. I also helped debugging until the end of times when we all smile and weep as 162 has come to an end. It's been fun.

**Xingyou Song**: I worked on the directory portion, came up with possible strategies for testing/debugging these parts, made an initial design doc for group discussion, and cleaned the code.

### Jonathan Lin
We definitely should have worked on this before midterm 3 came along. I relied a little too heavily that our slip days would suffice but stuff should never really be cut that close.

Debugging for task 3 was very interesting, I feel like I should communicate more effectively with teammates so that others can debug the code without too much learning/catching up overhead. I should also get much better at gdb.

### Jim Liu
This was not a great experience because most of us worked on this too late and during a period where a lot was going on. Debugging for this class in general was not something I personally learned how to do well and it really hurt me in this project in particular. I did have a better conceptual grasp of the ideas presented in the project and the various misc tasks that needed to be done however.

### Mong Ng
GDB is not always the best tool. Sometimes mere print statements can serve better function. I should be flexible in my debugging techniques.

As last project, I started very early and completed my part 3 weeks before deadline. However, last minute deadline still came. I believe I should have not believed my teammates, or any college student for that matter, that they will finish their part within 3 days. Because 162 is hard. A better option would be encouraging them to work a little at a time.

### Xingyou Song
This project was unfortunately put at the wrong time (end of the semester) and wrong length for me, which made it more stressful. I should have had a better conceptual understanding of what the project entailed to better contribute. 


## Student Testing Report

### Test case 1: my-test-1
**Features tested:** This test was the first option given in the spec. It tests the buffer cache's effectiveness, making sure that reading a file the second time should result in a higher hit rate.

**Mechanics of test:** First we reset the cache (so we start with a cold cache). We opened `test.txt`, read in the entire file, and measured the cache hit rate. We closed the file descriptor, re-opened the same file, read the entire file again, and measured the cache hit rate again. We check that the hit rate increased. The expected output is a message that is reached only if the hit rate increased. To write this test we implemented syscalls `reset_cnt` and `cache_hit_cnt`. 

**`my-test-1.output`:**
```
Copying tests/filesys/extended/my-test-1 to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
qemu -hda /tmp/jY5WkF69Cs.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading...........
Kernel command line: -q -f extract run my-test-1
Pintos booting with 4,088 kB RAM...
382 pages available in kernel pool.
382 pages available in user pool.
Calibrating timer...  419,020,800 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 177 sectors (88 kB), Pintos OS kernel (20)
hda2: 236 sectors (118 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'my-test-1' into the file system...
Putting 'tar' into the file system...
Erasing ustar archive...
Executing 'my-test-1':
(my-test-1) begin
(my-test-1) improvedHitRate = 1
(my-test-1) end
my-test-1: exit(0)
Execution of 'my-test-1' complete.
Timer: 65 ticks
Thread: 0 idle ticks, 63 kernel ticks, 2 user ticks
hdb1 (filesys): 498 reads, 470 writes
hda2 (scratch): 235 reads, 2 writes
Console: 1026 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
```
**`my-test-1.result`:**
```
PASS
```

**Two non-trivial kernel bugs:**
1. If the kernel fails to mark the necessary metadata (such as invalid bit) to signal that the block is in the cache, then the new hit rate would be as bad as the old hit rate. This is because even though the blocks are physically in the cache, the cache could not figure out whether they are valid entries. Output would be `improvedHitRate = 0`.
2. If the kernel flushes and invalidates cache blocks after closing a file, then the new hit rate would be as bad as the old hit rate. Output would be `improvedHitRate = 0`.

### Test case 2: my-test-2
**Features tested:** This test was the second option given in the spec. It tested the buffer cache's ability to coalesce writes to the same sector.

**Mechanics of test:** We first created and wrote a 64KB file, byte-by-byte, and read it in byte-by-byte. We called our custom syscall `write_block_cnt` to see how many device writes were used, and stored this amount. We then did the same thing, except now with a 128 KB file (also written and read byte-by-byte), and called `write_block_cnt` again. We checked that the amount that the count increased by was around 128 (specifically less than 130), since 64 KB (the difference in file size from the first time to the second) takes 128 extra blocks. The reason we do a relative comparison instead of an absolute count is to account for any possible overhead from creating a new file.

**`my-test-2.output`:**
```
Copying tests/filesys/extended/my-test-2 to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
qemu -hda /tmp/pVpEX5ukmR.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading...........
Kernel command line: -q -f extract run my-test-2
Pintos booting with 4,088 kB RAM...
382 pages available in kernel pool.
382 pages available in user pool.
Calibrating timer...  419,020,800 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 177 sectors (88 kB), Pintos OS kernel (20)
hda2: 237 sectors (118 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'my-test-2' into the file system...
Putting 'tar' into the file system...
Erasing ustar archive...
Executing 'my-test-2':
(my-test-2) begin
(my-test-2) writeCalls succeed
(my-test-2) end
my-test-2: exit(0)
Execution of 'my-test-2' complete.
Timer: 745 ticks
Thread: 0 idle ticks, 63 kernel ticks, 682 user ticks
hdb1 (filesys): 1289 reads, 873 writes
hda2 (scratch): 236 reads, 2 writes
Console: 1029 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...

```

**`my-test-2.result`:**
```
PASS
```

**Two non-trivial kernel bugs:**
1. If the cache writes to disk everytime the user write to the cache, then we would get a lot of disk writes, each per user writes, and outputs `writeCalls exceeded acceptable amount`.
2. If the kernel fails to locate a block in the cache, and keeps adding same blocks over and over again, our cache would soon be full and every writes correspond to an eviction. This cause a disk write per user write. Then outputing: `writeCalls exceeded acceptable amount`.

### Pintos test-writing experience
I wrote a test for whether or not the cache hit rate improved upon reading the same file twice. The idea was very simple but hard to execute. It involved using syscalls we wrote for file systems and additional file syscalls to gather cache information and then calling those cache sycalls at two different moments in time to compare. I was not a fan of having to just look at other test cases and the perl files to decrypt what was going on. I think there should be a better way to teach us how to write test cases for pintos. It took me longer than expected to write the test case but I did learn how to work better with something that was not well documented. 
