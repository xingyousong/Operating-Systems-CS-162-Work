Final Report for Project 2: User Programs
=========================================

## Group Members

* Jonathan Lin (<jlin703@berkeley.edu>)
* Jim Liu (<jim.liu@berkeley.edu>)
* Mong Ng (<monghim.ng@berkeley.edu>)
* Xingyou Song (<xsong@berkeley.edu>)

# Argument Passing

## Code Edits and Additions

### Ultimately, we edited the following functions/code for this task:
From userprog/process.c:

`static void start_process (void *file_name_)`

## Algorithms
We did not have to deviate from our design doc too much on this part. The biggest thing was that our TA brought up that we should use `strtok_r` to guard against synchronization error, which we did not mention in the spec.

We also parse again in the parent thread in order to get the program name separate from the argument, so that the test case's printing would match.

## Synchronization
No change from design doc


# Part 2: Process Control Syscalls

## Code Edits and Additions

### Ultimately, we edited the following code/functions for this task:
`static void syscall_handler (struct intr_frame *f UNUSED)`
`tid_t process_execute (const char *file_name)`
`static void start_process (void *file_name)`
`void process_exit (void)`


### We also added the functions:
`pid_t   exec (const   char *cmd_line)`
`int   practice (int i)`
`void   halt (void)`
`int   wait (pid_t pid)`

`int   is_valid_ptr (void *ptr)`
`void   validate_ptr (void *ptr)`
`void   validate_arg (void *ptr)`
`void   validate_str(char *str)`
`void   validate_str_with_len(char *str, int len)`

## Algorithms
In order to match test case output, we modified in exception.c the irrelvant debug information and replaced it with exit(-1) so that wait would work properly.

But otherwise, this part is rather similar to the design doc.

## Synchronization
No change from design doc

# File Operation Syscalls

## Code Edits and Additions

### Ultimately, we edited the following code/functions for this task:
`static void syscall_handler (struct intr_frame *f UNUSED)`

`static bool load (const char *cmdline, void (**eip) (void), void **esp)`
`void process_exit (void)`


### We also added the functions:
`int   read (int fd, void* buffer, unsigned size)`
`void   exit (int status)`
`int   write (int fd, const   void* buffer, unsigned size)`
`int   open (const   char *file)`
`void   close (int fd)`

`int thread_assign_fd (struct file *)`
`struct file *thread_get_file (int)`
`struct file *thread_close_fd (int)`

## Algorithms
We used the same methods as stated in the design document, but refactored the specific filesys calls into functions and added argument validation. We followed precisely the outline from design, because each syscall was relatively simple to implement. The main change was not using hash maps to store file descriptor mappings. We also had to make sure we did not close the executable in the `load` function to pass the rox tests.


## Synchronization
As stated in the design document, each file syscall acquires the lock, performs its relevant function, and releases the filesys lock.

# Reflection

## Member Tasks

**Jonathan Lin**: I worked on the filesys portion, including handling all file descriptor functionality, denying write to executables, and open/write/seek/tell/close. I also wrote the two new test cases.

**Jim Liu**: I worked on verification of arguments for syscalls including the stack pointer, up to 3 parameters of the syscalls and verifying char pointers for filesys operations.

**Mong Ng**: I worked on part 1, exec, wait, and exit, as well as debugging a lot of edge cases such as multi-oom. I also handled any merge conflicts the team might have.

**Xingyou Song**: I worked on the filesys portion, came up with possible strategies for testing/debugging these parts, made an initial design doc for group discussion, and cleaned the code.

## Jonathan Lin
I think our design document process was much better this time - a huge part of what helped was that we had multiple sets of eyes looking over *all* sections instead of just leaving each person to only do their own part. This would contribute to smoother coding.

During coding the actual concepts we had to implement were not too bad but there were several bugs that left me hung up for a very long time that were solved relatively quickly by asking a teammate. What I was convinced was some huge issue turned out to be the tiniest typo. Next time I should ask sooner, and also reach out to other members to see if they might be hung up on some bug.

## Jim Liu
The design document was much more refined compared to project 1 due to our low grade's on project 1's design document. The fact that we had a better design document meant we were more confident in coding.

I learned that spending time in advance to improve my git skills will end up saving me a lot of time in the future for group projects and a comprehensive design document improves your quality of life significantly.

## Mong Ng
ALWAYS FIX ANY WARNING YOU SEE! is probably the one true realization I had. Warnings are great for detecting passing the wrong argument ptr, because it will compile, but the bug is impossible to locate.

I also realized that sometimes to make a feature, I need to create certain tool to prepare myself. For example, I wanted to run specific test, so I made a script that records the output from make check, and just quickly find the gdb command I need for that test case. The time I spent developing that tool helps me a lot in the future.

Also, I should just go ask for help more often. Sometimes, a bug I stared for 1 day can be solved by others within 10s.


## Xingyou Song
I think this time around, we did much better with making a clean, fool-proof design document from which we could just implement the code by following the document precisely. This made writing code much faster, with less "rethinking".

I think I was less competent with debugging this time for some of the code (e.g. not realizing a simple fix to a bug was just a wrong variable input), and not knowing what certain bugs truly meant - this would've sped things much faster.

# Student Testing Report

## Test case 1: fd-indep
**Features tested:** This test case tested very basic functionality of the seek and tell syscalls (mainly just making sure they were implemented), but more importantly, the independence of file descriptors opened on the same file.

**Mechanics of test:** It retrieved two file descriptors by opening a file twice. It called seek on one of them and make sure the other did not move (by using the tell syscall). We used `msg` to display the return values of the tell syscalls, for comparison.

**`fd-indep.output`:**
```
Copying tests/userprog/fd-indep to scratch partition...
Copying ../../tests/userprog/sample.txt to scratch partition...
qemu -hda /tmp/cMz3Bj3TIo.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading..........
Kernel command line: -q -f extract run fd-indep
Pintos booting with 4,088 kB RAM...
382 pages available in kernel pool.
382 pages available in user pool.
Calibrating timer...  419,020,800 loops/s.
hda: 5,040 sectors (2 MB), model "QM00001", serial "QEMU HARDDISK"
hda1: 167 sectors (83 kB), Pintos OS kernel (20)
hda2: 4,096 sectors (2 MB), Pintos file system (21)
hda3: 104 sectors (52 kB), Pintos scratch (22)
filesys: using hda2
scratch: using hda3
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'fd-indep' into the file system...
Putting 'sample.txt' into the file system...
Erasing ustar archive...
Executing 'fd-indep':
(fd-indep) begin
(fd-indep) tell = 0
(fd-indep) tell = 0
(fd-indep) end
fd-indep: exit(0)
Execution of 'fd-indep' complete.
Timer: 57 ticks
Thread: 0 idle ticks, 56 kernel ticks, 1 user ticks
hda2 (filesys): 95 reads, 214 writes
hda3 (scratch): 103 reads, 2 writes
Console: 963 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
```

**`fd-indep.result`:**
```
PASS
```

**Two non-trivial kernel bugs:**
0. (Simplest case, already sort of tested) If the kernel returned the same file descriptor number multiple times for a given file name instead of returning unique file descriptors, the test specifies this as a failing condition.
1. If the kernel maps file descriptors to file names and then mapped file names to a `file *` pointer instead of mapping file descriptors directly to `file *` pointers, then seeking on the first file descriptor would result in a different output (i.e. not the same as the first call to tell) when calling tell for the second time on the other file descriptor. Basically, seeking the first fd will seek the second.
2. If the kernel used `file_seek` on *all* relevant file pointers for a file name upon calling seek on one of the file descriptors for that file, instead of calling it on just the one, then the second call to tell would output 4 instead of 0.

## Test case 2: write-bad-buff
**Features tested:** This test case tested the write syscall, and made sure it could handle a buffer that started with a valid memory address but did not end in one (after reading the requested number of bytes). The correct implementation should check that all of the specified length of addresses is not valid and exit(-1) and kill the user program. The provided test suite would only catch this is the buffer *started* in bad memory in the first place.

**Mechanics of test:** We created and opened a file to write into. Then called write on the relevant fd and a buffer that starts very close to `PHYS_BASE` but still in user space, and a number so large that it would not only go into kernel memory but also surpass virtual memory space (thus overflowing the virtual address). We check to make sure the process exits with exit code -1.

**`write-bad-buff.output`:**
```
Copying tests/userprog/write-bad-buff to scratch partition...
Copying ../../tests/userprog/sample.txt to scratch partition...
qemu -hda /tmp/JNPS4thmXQ.dsk -m 4 -net none -nographic -monitor null
PiLo hda1
Loading..........
Kernel command line: -q -f extract run write-bad-buff
Pintos booting with 4,088 kB RAM...
382 pages available in kernel pool.
382 pages available in user pool.
Calibrating timer...  419,020,800 loops/s.
hda: 5,040 sectors (2 MB), model "QM00001", serial "QEMU HARDDISK"
hda1: 167 sectors (83 kB), Pintos OS kernel (20)
hda2: 4,096 sectors (2 MB), Pintos file system (21)
hda3: 105 sectors (52 kB), Pintos scratch (22)
filesys: using hda2
scratch: using hda3
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'write-bad-buff' into the file system...
Putting 'sample.txt' into the file system...
Erasing ustar archive...
Executing 'write-bad-buff':
(write-bad-buff) begin
(write-bad-buff) create "test.txt"
(write-bad-buff) open "test.txt"
write-bad-buff: exit(-1)
Execution of 'write-bad-buff' complete.
Timer: 58 ticks
Thread: 0 idle ticks, 56 kernel ticks, 2 user ticks
hda2 (filesys): 114 reads, 221 writes
hda3 (scratch): 104 reads, 2 writes
Console: 1014 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
```

**`write-bad-buff.result`:**
```
PASS
```

**Two non-trivial kernel bugs:**
1. If the kernel doesn't have any of the arg checks for chars, then it will try to deference values from all of the addresses of the buffer. The addresses specified start at a valid address, but we write so much that it will go overflow and go back to zero, and trying to dereference zero will crash the machine. Even simpler, just like that test case from additional question, we would be accessing memory below code section, and this will definitely crash the machine. Therefore, the output would be a bunch of faulting information.
2. If the kernel only checks that the addresses are within possible memory (namely 4 GB) but doesn't check that the addr is below PHYS_BASE, then the kernel would output "exit(0)" because the program will successully exit. However, the user program would then be able to print kernel memory onto a file. We would have leaked kernel information.

## Pintos test-writing experience

My experience writing tests was very fun. The pintos testing system can be improved by providing more testing frameworks, library functions that are not available to user but available to developers. This way, we can test many more functionalities. Through this, I learned that there are many ways a malicious user can try to take advantage of the kernel bugs, therefore, writing quality test cases is essential for development.

I had previously also wanted to write some tests involving locks (initialized and acquired within the test code) but had troubles getting code involving locks to compile. Also it would be nice if there was a way to write a test that did not require the output file to exactly match - there was a test that I ended up not writing because I needed to exit with a status that was a thread id (which of course is not predictable and so can't be concretely put in the perl script).
