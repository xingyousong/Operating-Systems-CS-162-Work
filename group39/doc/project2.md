Design Document for Project 2: User Programs
============================================

## Group Members

* Jonathan Lin <jlin703@berkeley.edu>
* Jim Liu <jim.liu@berkeley.edu>
* Mong Ng <monghim.ng@berkeley.edu>
* Xingyou Song <xsong@berkeley.edu>


# Task 1: Argument Passing 

## Data Structures and Functions:

We only edit the function ```static void start_process (void *file_name_) ```. 


## Algorithms
We follow the conventions stated in part 3.1.9 in the spec. 

After the load function call, we make sure that it succeeded and proceed to tokenize the file_name, separating the string by spaces (being sure to treat consecutive spaces as one separation); we will also null-terminate each token. We will then obtain the pointers to the individual, space-separated arguments.

We then, push the following onto the stack in order: the argv data strings, word-align, the pointers to the argv, the argv pointer to the start of the array of the argv, argc, then a fake return address.

To "push" on the stack, we decrement ``` if_.esp``` by the appropriate amount, for example, the tokens' len or the size of int for argc, then use `memcpy()` or simple assignment to write the prepared data.

Word alignment needs to be considered when we push our arguments on the stack and only then because this is the only case where we are potentially decrementing the stack pointer by a non-multiple of 4. This word alignment should be done after all char arguments have been pushed onto the stack. 

## Synchronization 
According to 3.1.6, since we're accessing virtual memory in the function, this means that we will not arrive at cases where e.g. user-process 1 will memcpy over user-process 2's memory. Nothing breaks because even if the function is interrupted, it can still finish copying into virtual memory when it resumes. Thus, we do not need to implement interruption handlers. 

## Rationale
From the initial document, we understood that ```if_.esp``` is the stack pointer, which is the destination pointer we will use in the first argument of ```memcpy()``` (which copies memory forward, starting from ```if_.esp```, which explains why we decrement ```if_.esp``` first). We then followed the convention specified in the spec  to order our copies. 


# Task 2: Process Control Syscalls 


## Data Structures and Functions:
### In userprog/syscall.c
* `static void syscall_handler (struct intr_frame *f UNUSED) ` to handle cases of process control syscalls. 

### In userprog/process.c
We modify
* `tid_t process_execute (const char *file_name)` to initialize wait_status between parent and child, as well as to enforce that this function exits only after load in child has finished
* `static void start_process (void *file_name_)` to fully initialize the wait_status struct, and to confirm the status of load
*`int process_wait (tid_t child_tid UNUSED)` for wait syscall
* `void process_exit (void)` for exit syscall

### In threads/thread.h
- In `struct thread`, add
    * `struct wait_status wait_status //this process's completion status` 
    * `struct list children_wait_status //a list of children's wait status`
- Add
 ```
    struct wait_status
    {
        char *fn_copy; /* stores the command the child process needs to load, just so we can pass this to thread_create */
        struct list_elem elem; /* ‘children’ list element. */
        struct lock lock; /* Protects ref_cnt. */
        int ref_cnt; /* 2=child and parent both alive, 1=either child or parent alive, 0=child and parent both dead. Initialized to 2.*/
        tid_t child_tid; /* Child thread id. */
        int exit_code; /* Child exit code, if dead. Also used to indicate status of loaded executable*/
        struct semaphore finished; /* parent sema_down to wait, child sema_up to signal finished */
    };
```
to store the wait status of the child for the parent process to access

## Algorithms

### Retrieving and verifying user-provided pointers point to valid memory addresses
In `syscall_handler()` of userprog/syscall.c, we do the following in order (some parts in a separate function to avoid repeated code):
* Validate `esp`
* Get the syscall in `esp`
* If the syscall doesn't take in arguments, perform the syscall
* Verify the next arg
* If the syscall takes in exactly one argument, perform the syscall
* and so on until we have processed enough arguments, or one of them is invalid (see below)

Note: each time we validate an arg, we first validate the arg, and then we copy it to a local variable to avoid the user from meddling with it. This is suggested by a TA to prevent kernel check and then user change.

For each argument verification, we cast it to `char *`, and for those 4 bytes, byte by byte (to account for memory on the edge of a page) we perform:
1. Null check by simple comparison
2. Check that it is within user virtual memory by `bool is_user_vaddr` from `threads/vaddr.h`
3. Check that the pointer points to *mapped* virtual memory (use `pagedir_get_page` from `pagedir.c`)
4. check using below method for char* arguments
5. If any one of these checks fail, we free any locks it held and kill the process (`thread_exit`), setting the exit status (if its a child process) to -1. 

### Validating the actual string pointed by the char ptr
If an argument is a `char *`, we do then a version of strcpy that validates that every char is in a valid address and copy those individual chars. We stop when we see a null terminator or 1024 is reached.

### Validating the value of the args
We also make sure after we copied the arguments, we perform a logical check on the values. For example, `wait` should be waiting on valid child. These are done within the individual below parts.

### halt
Calls `shutdown_power_off()` in devices/shutdown.h. 

### exec
Passes the input into ```tid_t process_execute (const char *file_name) ```. 
* In that function, right before the child thread is created, we malloc and initialize a `struct wait_status` 
    * We do not yet initialize its `tid` value
    * We set its `fn_copy` to the passed in argument, initialize the lock/semaphore, set `ref_cnt` to 2 
    * Everything else to 0
* We pass this as the last argument to `thread_create()` (so we will modify `start_process()` to take in this struct as its argument). 
* After the child thread is created (if successful), we set the `tid` value of the struct and add the struct to the list `children_wait_status` of the parent, and we set the `wait_status` of the child thread. 
* The parent should call `sema_down()` on the semaphore in the struct. 
    * After making it past the semaphore, the parent can then return the pid (stored in `tid` of the `wait_status` struct) or -1 (knowing the status of loading the executable through `exit_status`) and should reset the `exit_status` to 0.
* `start_process()` (which is the child), after knowing the status of loading the executable, should set `exit_status` to a  value indicating success/failure and call `sema_up()` so `process_execute` can return. 


### wait
* Pass the `arg[1]` (which is `pid` of child) to `process_wait`, in which we look at the parent thread's children in `children_wait_status` to see if it has a child with the specified pid (or tid).
* If the child exists,
    * Call `sema_down()` on the semaphore in the corresponding `wait_status` struct.
    * Once the (parent) thread is unblocked, all that remains is to assign `f->eax` to the `exit_status`, which should be stored in the `wait_status` struct.
    * To prevent waiting on this child again, we free its related `wait_status` struct and remove it from the list of children
* Otherwise, returns -1

### exit
Calls `thread_exit()`, which eventually calls `process_exit()`

In addition to existing code, at the end of `process_exit()`: 

1) If the thread is a child (i.e. `wait_status` is not NULL):
    * Assign the `wait_status` exit code to the argument passed in to the syscall
    * Acquire the `wait_status` lock
    * Decrement its `ref_cnt`
    * If `ref_cnt` is now 0, free the `wait_status` struct
    * Otherwise, release the lock and up the semaphore

2) If the thread is a parent (i.e. `children_wait_status` has length >0): For each `wait_status` struct, acquire the lock, decrement its `ref_cnt`, release the lock, and if `ref_cnt` is now 0, free the `wait_status` struct.
3) Close all of the thread's file descriptors (see `close` syscall in task 3)

### practice
Assign `f-> eax` to one more than `args[1]`.

## Synchronization 
Synchronization is needed for exec and wait. 

`exec`:
* We need to ensure that the exec syscall does not return before knowing whether or not the child thread successfully loaded its executable; this is done by the parent sema down after thread_create, and the child sema_up to signal load completed.

`wait` and `exit`:
* sema_down when the parent wait, and sema_up when the child finishes
    * this allows the child to finish either before or after the parent
* We need a lock that is shared between parent and child processes (one per relationship, in the `wait_status` struct). This lock is to ensure that `ref_cnt` is accurate to ensure that structs are freed when no longer needed, so the lock must be acquired before changing or reading its value. 
* Freeing the wait_status struct:
    * We only free the struct when ref_cnt reaches 0, meaning that the other thread has died and we are the only one holding a reference to this struct.
    * We have 4 cases of parent and child patterns:
        * P (parent) spawns C (child) and waits on it
            * C should have decremented its ref_cnt when it exits/crashes, therefore P will be the last thread to live after it returns from wait
        * C finishes first and P waits on it
            * C should have decremented its ref_cnt when it exits/crashes, and set up the exit_code correctly, then P will just free the struct when it dies
        * P finishes first and no wait
            * at the end of the day, all threads will free the struct, so it doesn't matter whether P or C finishes first, whoever sees that ref_cnt is 0 will free the struct
        * C finishes first and no wait from P
            * same as above

## Rationale
We realized that there was a set of information (child exit status, parent-child relationship) that we had to store someplace not within any particular parent or child. Storing in parent does not work because if the parent exits before the child without calling `wait`, the child would try to place its exit code in deallocated memory. Storing in the child does not work because if the child exits before the parent calls `wait`, the information on exit code is lost. So we created a separate struct pointed to by both the parent and child.

We also figured that once we had this protocol set up for wait functionality, we might as well use it as well for the exec syscall - making sure it doesn't return before knowing if the executable loaded or not. We can just reuse `exit_code` and `struct semaphore finished`

Checking the validity of user-provided pointers follows rather directly from the spec. Except we found out that the actual characters pointed by the provided string might not be valid, so we validate them as well. We also copy all necessary arguments to local variables to avoid check and change.

We never malloc anything in the syscall because we copy the arguments using stack local variables that are 4 byte variables and can be copied easily and with the reasonable assumption that file_name strings do not exceed 1024 chars. Therefore, we don't need to free any malloc should syscall fail, as the spec suggested, and the user space cannot malloc so we don't need to worry about that.

### Releasing process's resources should it crash due to invalid arguments
In the spec, it mentioned that we should free any resource should the arguments are invalid. We interpret this as followed:
There is no malloc to be free
* user cannot malloc, so we should have to free those memory because they don't exist
* kernel malloced memory, but we also don't need to worry about them since kernel thread should not call syscall
* Inside the syscall, we only use stack variables, so no need to free any memory

There is no lock that needs to be freed
* This is because we only acquire any locks after checking that all the arguments are valid

# Task 3: File Operation Syscalls


## Data Structures and Functions:

In userprog/syscall.h
* A global variable`struct lock filesys_lock `inside  as a "global lock" for the file system synchronization

In userprog/syscall.c
* modify `syscall_handler (struct intr_frame *f UNUSED)`
    * we'll add an if conditional for each file syscall in static void, where we will handle each case individually.

In thread/thread.c
* We modify `struct thread`: 
    * `int next_fd`: represents the next file descriptor available to assign
    * `struct hash fd_map`: maps file descriptors to file pointers
    * `file * executable`: the executable that the process executes

We also add the following functions to threads/thread.c:
`int thread_assign_fd(file *)`: Maps a new file descriptor to the given file pointer, returns the file descriptor.
`file * thread_get_file(int)`: Retrieves the pointer corresponding to the file descriptor, returns NULL if not a valid file descriptor.

## Algorithms
(See "Retrieving and verifying user-provided pointers" and "Releasing process’s resources should it crash due to invalid arguments" from task 2 - it applies here as well)

In `syscall_init`, we initialize the lock to the unlocked state.

When threads are created, we give them a `next_fd` of 2. 

For the following commands (where we use continued conditional checks to see where the syscall argument goes to), we will call the relevant functions from the filesys directory (specifically from file.c and filesys.c), and assign `f-> eax ` to the relevant output/return value, all within the `static void syscall_handler (struct intr_frame *f UNUSED)`. Unless otherwise specified, the conditional chunk for the syscall just consists of the filesys function call.

### `create` - `filesys_create`
### `remove` - `filesys_remove`
### `open` - `filesys_open`
Here, we first use filesys_open to obtain a pointer. If the pointer is null we return -1. Otherwise, we pass in the pointer to our `thread_assign_fd` function and assign its output (the file descriptor) to `f-eax`. 
### `filesize` - `file_length`
This case **and all others following** will first pass the `fd` input into our `thread_get_file` to get the file pointer corresponding to the file descriptor, then pass the file pointer into the relevant function along with any other arguments. If the file descriptor is invalid, -1 is placed in `f->eax` and we return.
### `read` - `file_read`
If fd is 0, will read from keyboard using `input_getc()` instead of using `file_read`. If fd is 1, do nothing.
### `write` - `file_write`
If fd is 1, will write to console using `putbuf()` instead of using `file_write`. If fd is 0, do nothing.
### `seek` - `file_seek`
### `tell` - `file_tell`
### `close` - `file_close` 
Will remove the entry corresponding to fd from the `fd_map` in `struct thread` for the current thread, returning -1 if that fd is not present.

The code for every syscall listed above MUST acquire the lock at the beginning, and release the lock at the end (with the corresponding `lock_acquire` and `lock_release` functions in synch.h). Trying to close stdin or stdout (0 or 1) returns -1.

### `thread_assign_fd`
We map the value of `next_fd` to the file pointer in `fd_map`, then increment it, then return the original `next_fd` value.

### `thread_get_file`
We use the `hash_find` function to retrieve the file pointer corresponding to the file descriptor passed in, or NULL if that file descriptor is not assigned to any file pointer for that thread.

### Writing to executables
To ensure executables are not written to while being executed, we will call `file_deny_write()` on the executable in `load()` after the file is opened and assign the field `file * executable` to the file pointer of the executable. We call `file_allow_write()` on that file pointer in `process_exit()`. We acquire `filesys_lock` before and after each of the allow/deny calls.

## Synchronization 
To ensure thread safety, every syscall will first obtain the single lock before executing the relevant filesys function, and will block if the lock is already acquired, so synchronization is handled properly.

Even when we call `file_deny_write()` and `file_allow_write()`, we first have to acquire the global filesystem call, therefore, the internal counter that prevents writing to that file could not have any race condition, nor would any other syscall be modifying that file when we call allow/deny.

And since once a thread acquires the lock successfully, it will then only continue the operation, eventually it must release the lock. Therefore, all other threads waiting will eventually get the chance to keep running.

## Rationale
Each process has an independent set of file descriptors; however, in Pintos each process consists of one thread so each thread maintains its own set of file descriptor pointers. This is why we make this mapping a part of the thread struct.

Also, this mapping allows us to have multiple files being opened at the same time, or multiple file descriptors for the same file, as we can have multiple mapping of file descriptors to file pointers.

To actually store the mapping, we decided on a hashmap instead of a simple list (where indices represent fd) because the hashmap can grow and can account for more file descriptors being assigned whereas a list cannot. However, if we are guaranteed that no more than a certain number of open calls are made per thread, we would just use a list.

# Additional Questions 

## 1) Test case using an invalid stack pointer
We found such a test in **sc-bad-sp.c**. In line 18, it calls ```asm   volatile ("movl $.-(64\*1024\*1024), %esp; int $0x30");``` which means literally, get the value of the pointer to the current location, subtract by the value ```$(64\*1024\*1024)``` (which is 64 MB), and push this value into the the ```%esp```  register. Basically the stack pointer will then move 64 MB down (from the code segment it starts at), which is guaranteed to be far out of range (below the code segment) and must be rejected by the program, leading to a killed process. If line 19 is reached the process was not killed and the test fails.

## 2) Test case using a stack pointer too close to page boundary
We found such a test in **sc-bad-arg.c**. Line 14 sets the stack pointer to `0xbffffffc`, which is four bytes less than `PHYS_BASE` (whose default value is `0xc0000000`, i.e. only four bytes away from kernel virtual memory. Line 15 places the `SYS_EXIT` syscall number there at that location. The exit syscall has an argument, and arguments, by convention, are at higher addresses on the stack, so the user program would supposedly be trying to access an argument which is in kernel virtual memory (since the `SYS_EXIT` number was put at the very edge) - it should recognize this and kill the process. So if line 16 is reached the process was not killed and the test fails.

## 3) One requirement not fully tested
We propose adding a test that calls `open` on a file twice, calls `seek` on one of file descriptors returned to move it to a different file position, and checks if the other file descriptor moved by calling `tell` on it - it should not have moved. It then closes one file descriptor and ensures the other file descriptor is not closed (i.e. does not return -1 when `close` is called on it. This ensures that file descriptors "are closed independently and do not share a file position" (spec section 3.2.3, description of `open`).

## 4) GDB Questions
### 1. 
current thread is "main".
address is 0xc000e000.
Including main, we have main and idle:

0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000ee0c "\210", <incomplete sequence \357>, priority = 31, allelem = {prev = 0xc0034b50 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc0034b58 <all_list+8>}, elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
### 2. 
(gdb) bt
#0  process_execute (file_name=file_name@entry=0xc0007d50 "args-none") at ../../userprog/process.c:32
#1  0xc002025e in run_task (argv=0xc0034a0c <argv+12>) at ../../threads/init.c:288
#2  0xc00208e4 in run_actions (argv=0xc0034a0c <argv+12>) at ../../threads/init.c:340
#3  main () at ../../threads/init.c:133

In reverse order, we starts here in the main function:

/* Run actions specified on kernel command line. */
run_actions (argv);

/* Invoke action and advance. */
a->function (argv);

process_wait (process_execute (task));

process_execute (const char *file_name)

### 3. 
addr is 0xc010a000
name is "args-none\000\000\000\000\000\000"
We now have main, idle, and the process we just started called args-none:

pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_BLOCKED, name = "main", '\000' <repeats 11 times>, stack = 0xc000eebc "\001", priority = 31, allelem = {prev = 0xc0034b50 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0036554 <temporary+4>, next = 0xc003655c <temporary+12>}, pagedir = 0x0, magic = 3446325067}

pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc010a020}, elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_RUNNING, name = "args-none\000\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc0104020, next = 0xc0034b58 <all_list+8>}, elem = {prev = 0xc0034b60 <ready_list>, next = 0xc0034b68 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
### 4. 
It is created here:   tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);

### 5. 
0x0804870c
### 6. 
#0  _start (argc=<error reading variable: can't compute CFA for this frame>, argv=<error reading variable: can't compute CFA for this frame>) at ../../lib/user/entry.c:9

### 7. 
We get a page fault because the test is trying to start another process passing in arguments, but we haven't implemented argument passing yet. Therefore, Pintos was not able to read the value of those variables.

Our guess is that since we didn't push the arguments, Pintos tried to interpret the memory locations above the stack points as argc and argv, but those are above the pages we have obtained, and therefore we are accessing unmmapped or invalid or memory that we don't have access to. Therefore a  page fualt occurs.
