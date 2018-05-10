Design Document for Project 3: File Systems
============================================

## Group Members

* Jonathan Lin <jlin703@berkeley.edu>
* Jim Liu <jim.liu@berkeley.edu>
* Mong Ng <monghim.ng@berkeley.edu>
* Xingyou Song <xsong@berkeley.edu>


## Task 1: Buffer Cache

### Data Structures and Functions
We create files fs_cache.h and fs_cache.c for better organization.
In fs_cache.h:
```
/*
* Represents a block being in the 
* buffer cache. Includes both metadata
* for eviction policy, as well as the
* actual block data
*/
struct cache_ent{
    block_sector_t block;

    bool valid; // true if this ent contains valid data
    bool dirty; // true if this ent was modified, needed for write back
    bool used; // true if this ent has been used recently, needed for eviction in clock algorithm
    
    uint32_t ref_cnt;
    struct lock lock; // to serialize any load, read, and write
    
    char data[BLOCK_SECTOR_SIZE]; // stores the actual block
};

#define CACHE_SIZE 64;

// Current clock hand. Between 0 to 63, inclusive.
uint32_t clk_hand = 0;

/*
* Ensure that when locating block, no other
* thread is evicting, also for sequentializing 
* eviction 
*/
struct lock cache_lock; 

// our buffer cache
struct cache_ent buff_cache[CACHE_SIZE];
```

In fs_cache.c, add:
```
/*
* initialize eviction_lock,
* 64 buff_cache_ent's lock,
* set all ref_cnt to 0
* and set all the valid bits to false
*/
void buff_cache_init(void);

/*
* Called when the cache is full and
* we need to evict a cache entry for
* new data. Implements the clock algorithm.
* Needs to be externally synchronized.
*/
struct cache_ent* cache_evict();

/*
* Find a cache entry corresponding to the desired
* block or load one. Will evict other blocks if 
* necessary. Will always acquire the entry's lock 
* before returning.
* 
* Caller must eventually release the entry's lock
*/
struct cache_ent* cache_find_ent(block_sector_t block);

/*
* read memory of len LEN starting at OFFSET of
* this block from the cache to the buffer
*/
void cache_read_len(block_sector_t block,
    const void* buffer,
    uint32_t offset,
    uint32_t len)

/*
* Call cache_read_len with offset 0 and len the 
* size of a block. For ease of integrating code.
*/
void cache_read(block_sector_t block, const void* buffer);



/*
* write buffer to this block in the cache starting
* at OFFSET to len LEN
*/
void cache_write_len(block_sector_t block,
    const void* buffer,
    uint32_t offset,
    uint32_t len)

/*
* Call cache_write_len with offset 0 and len the 
* size of a block. For ease of integrating code.
*/
void cache_write(block_sector_t block, const void* buffer);
```
as well as
* modify `inode_init` to call `buff_cache_init` after initializing the `open_inodes` list
* replace all calls (related to fs) to `block_write` and `block_read` to `cache_read` and `cache` write (or the len version if original code was using bounce buffer)

### Algorithms
In pintos, there is 1-1 mapping of sector and block; therefore, we use them interchangeably here. 

#### Integration with Existing Code:
The buffer cache will be injected between the inode level and the block level. All inode operations that involve the disk are:
* `inode_create`
* `inode_open`
* `inode_close`
* `inode_read_at`
* `inode_write_at`

They all use `block_read` and `block_write` one way or the other. Therefore, we create wrapper functions `cache_read` and `cache_write` that first access the buffer cache, and if necessary, access disk via `block_write` and `block_read`. Note that we will get rid of bounce buffer, since we can use `cache_read_len` or `cache_write_len`and memset the needed amount.

#### Finding a Block in the Cache:
Each cache entry has a field called `block`, which uniquely identifies the block being cached. To locate a block,
* acquire `cache_lock`
* we loop through the 64 entries and checking valid bit and comparing `block`
* 2 routes from here on:
    1. Cache hit
        * increase the `ref_cnt` to prevent other thread from evicting it
        * release `cache_lock`
        * acquire the ent's lock
        * return this cache ent
    2. Cache miss - implies we reached the end without finding the sector
        * if there is invalid ent, we use it
        * otherwise we call cache_evict to get an invalid ent
        * increase the `ref_cnt` to prevent other thread from evicting it
        * Change the ent's `block` field to the new sector
        * acquire the ent's lock (note that since we just evicted this sector, there can never be deadlock because no other thread is using it)
        * release the `cache_lock`
        * load in the new data
* return this cache ent

#### Eviction Policy
* Assuming `cache_lock` has been acquired, the function `cache_evict` implements the clock algorithm
* Namely, we loop through the 64 cache entries until we found an entry satifying all of the following:
    * `ref_cnt` is 0, meaning it is not currently used or waited by other thread
    * use bit is 0
* As we loop, we decrement the use bit of each entry the clock hand passes
* Once we found a satisfying entry, write to disk if it was dirty, then return it

#### Reading and Write Sectors
* call `cache_find_ent` to locate a cache ent
* we then write or read this sector by memset with the specified `offset` and `len`
* if it was write, set dirty bit to 1
* set use bit to 1
* release ent's lock
* acquire `cache_lock`, decrease this ent's `ref_cnt`, release the `cache_lock`


### Synchronization
There are 2 kinds of data to be synchronized:
1. metadata about the cache entry
    * We have a global lock `cache_lock` here so that any locating of a cache entry and eviction are mutually exclusive. The `ref_cnt` mechanism ensures that once we found a cache entry, it won't be evicted until we are done, 
2. the cache entry's sector data
    * `cache_find_ent` will always acquire the ent's lock before returning. Therefore, when we read or write, we are guaranteed to be the only thread doing so

We enforce that a thread must first acquire global `cache_lock` before acquiring one of the 64 ent's lock. No circular wait implies no deadlock.

Note: We do have one case where we acquire an ent's lock while having already acquired the cache_lock. This is when we just evicted and need to load a block. There can also be no deadlock since we just evicted this ent, so no other thread can be using/waiting on it. Thus acquiring the ent's lock must succeed. This complication is needed to ensure loading the sector can be done outside of holding the global lock, for performance reason

#### Some questions to ponder about
* When one process is actively reading or writing data in a buffer cache block, how are other processes prevented from evicting that block?

When we are reading or writing the data, we would have had incremented `ref_cnt` to this cache ent, and the eviction algorithm doesn't evict blocks with `ref_cnt` greater than 0

* During the eviction of a block from the cache, how are other processes prevented from attempting to access the block?

Eviction doesn't start until `ref_cnt` hit 0; thus it only starts when no thread is assessing it. `cache_lock` serializes eviction and finding cache ent, thus other threads cannot be assessing it.

* If a block is currently being loaded into the cache, how are other processes prevented from also loading it into a different cache entry? How are other processes prevented from accessing the block before it is fully loaded?

As a block is being loaded, current thread will have acquired this ent's lock. However, other thread will note that the block is being loaded (since valid bit is set by the former), thus it won't try to load again. It also cannot assess because it doesn't have the ent's lock. Therefore, it must wait.

### Rationale
We do not use hash map here because internally, the pintos `struct hash` uses an underlying list and has to iterate through every element anyway. Using an array will be simpler with roughly the same performance.

We use one global `cache_lock` to serialize any (1) find of a cache entry and (2) eviction. However, when we know we are for sure touching this one cache entry, we release `cache_lock` to allow other threads to proceed, while acquiring the smaller-scoped entry's lock to work exclusively with this cache ent. This allows concurrent read/write/load to a cache ent.

We decided to synchronize across all assesses to a specific block. This automatically synchronizes directory and files. We decided to serialize read as well since this simplifies the implementation a lot more.

## Task 2: Extensible files

### Data Structures and Functions
#### `inode.c:`

For each file, we need to add pointers to the blocks containing the data. We have chosen to use one direct and one doubly-indirect pointer.
```
struct inode_disk { 
    ...
    block_sector_t direct;
    block_sector_t doubly_indirect;
}
```
Note: we will have to modify the size of `unused` attribute to make sure our struct fits exactly on one block.

`block_sector_t inode_fetch_block(struct inode_disk*, off_t pos)`
Given an inode and a desired byte offset, traverse pointers and find the relevant block, or -1 if no block is allocated for that offset yet. This function is essentially replacing `byte_to_sector` (we also just wanted to rename it).

`block_sector_t inode_extend(struct inode_disk*, off_t length)`
Given an inode and a desired length of file, find and allocate the needed free block pointers to the inode, updating its pointers as necessary.

#### `free-map.c:`
`struct lock free_lock`
Must be acquired before obtaining a free block.

#### We will also modify:
`inode_read_at`
`inonde_write_at`
`syscall_handler` (for the inumber syscall)
`inode_create` to work with the new index structure

### Algorithms

#### Fetching a block from a file given byte sector offset
1. If the offset is less than 512, we just return the block referenced by the direct pointer.
2. If the offset is at least 8MiB + 512B, this is impossible so we return -1.
3. Otherwise, to determine which indirect block pointer within the doubly-indirect block we should follow, we floor divide (byte offset - 512) by 2^16, the number of bytes mapped by each indirect block. 
4. Once we have the indirect block, to decide which block pointer within it to return, we take (byte offset - 512) mod 2^16.

If at some point the pointer to the next-level block is NULL, then we return -1, which indicates that the block for that byte offset has not been allocated yet. -1 is a reserved address which is only returned when the block is not allocated - we use it just as the current `byte_to_sector` function uses it.

#### Extending the file
1. Acquire the free-map lock
2. Calculate what the current sector offset is (how many sectors there are currently, use `inode_fetch_block`), and what we want it to be. Find the difference.
3. Do this once for however many new blocks we need to allocate:
    * Use `free_map_allocate` to get one free block
    * Use similar math as above to determine where within the direct/doubly-indirect blocks the new block should go, allocating indirect blocks as necessary.
4. If at any point `free_map_allocate` fails (as in, we run out of free blocks):
    * We free every block we have acquired thus far (so we need to keep track of what blocks we acquire through this function).
    * Return -1
5. Increase the file size attribute of the struct
6. Release the free-map lock
7. Return how many new blocks were allocated

#### Seeking
Seeking will only involve changing the file position (including going past EOF). Read and write will handle possible consequences of going past EOF.

#### Reading from files
1. First we translate from a byte offset (indicated by the file position) to a block number by calling `inode_fetch_block`. This is our first block.
    * We are not using a sparse file implementation, and we will check if the current file position is before EOF, so this will never return -1.
2. We then use the `cache_read_len` function created in task 1 to copy data into the caller's buffer until the end of the block, or until EOF (if it's within this block). 
    * Note that if the block is not in the cache this function take care of all the eviction and copying of data)
    * The way we know where within the block to start to read is by taking the byte offset mod the block size - the `cache_read_len` 
3. If we read the entire block and need the next one, `inode_fetch_block` is called again with a new byte offset, and the process (checking if in cache, fetching if not, reading) is repeated until EOF is reached.
    * If at any time we try to read past EOF,`inode_read_at` will return less than the bytes requested to be read, and will return how many bytes were actually read at the time (and copied into the user-passed buffer), before hitting EOF.
    * If a read starts past EOF in the first place, it will return 0 bytes and do nothing.

#### Writing to files
1. If the byte offset will cause the file size to increase, then call `inode_extend` with the inode and the new file length.
    * If this call returns -1, this write syscall "fails" and returns 0, doing nothing else
2. If the file position starts before the current EOF, skip the next step
3. All the data from EOF to the file position needs to be zeroed out on a block by block basis. Use `block_write` and a buffer of all zeroes to do so. 
    * See bullets below next number for how to do this, but starting from EOF (or file length) instead of file position.
4. We then need to write in the data from the user buffer, starting from the file position.
    * To write, we call `inode_fetch_block` on the file position to determine the starting block number.
    * We use `cache_write_len` function from task 1 to start writing in data.
    * Once finished with a block (and still having more to write) we fetch the next block using `inode_fetch_block` again and repeat.
5. Return the total number of bytes written (should be all of them, since we require that we acquire all necessary blocks before even starting to write)

#### inumber syscall
We have a thread function written in proj2 that converts fd handle to `struct file *`. We look at the `struct inode *` of that file pointer and pass it into `inode_get_inumber`.

### Synchronization 
We need a lock when allocating new blocks for a file, to prevent two processes from trying to allocate the same free block for the file they're working with.

Writing on a sector by sector basis (and therefore file by file basis) is taken care by the synchronization of task 1. Whenever we read or write anything, it happens through the functions written specifically for the cache, which will acquire the necessary locks and ensure thread safety. 

### Rationale
Since the block size is 512 bytes, or 2^9, a block can store 128 or 2^7 pointers.
Therefore, an indirect block can map to 2^7 blocks, or 2^9 * 2^7 = 64 KiB of data.
A doubly-indirect block will contain references to 2^7 indirect blocks, so it can map 2^16 * 2^7 bytes = 8 MiB.

We will use 1 direct pointer and 1 double-indirect pointer which means the total file size supported by an inode is a little over 8 MiB which satisfies the maximum file size pintos requires. We include 1 direct pointer for higher performance for small files (<= 512b), and a doubly-indirect pointer is needed to ensure we can support the maximum file size. We did not feel like the performance gain for some medium sized files was worth the extra complication of having an indirect pointer as well.

Whenever it found that we need to extend a file, we chose to allocate **all** blocks needed for the entire extension before doing a single write. This ensures that the call will be able to successfully write everything, instead of potentially finding out in the middle of the write that no more blocks can be allocated, making it more difficult to "roll back" to a previous good state.

We chose to support non-sparse files because we can simply allocate all the blocks we need once we know start writing from somewhere past EOF. This is a much simpler and robust design than having implicitly zeroed blocks that will need to be allocated in the middle of a write. We think the simplicity in implementation outweighs the gains made in saved blocks.

## Task 3: Subdirectories

### Data Structures and Functions

Note: This uses previous project 2 code - only additions and edits for project 3 are included here.

In thread.h: 
```
struct thread{
 ...
 struct dir* cwd; // current working directory
 struct dir* dir_map[128]; // maps fd with dir*
 ...
}
// the bottom functions are helper functions to provide mappings from fd ints to struct dir*'s
int thread_assign_dir (struct dir *);

/*
* look at the current thread's dir_map and find
* the corresponding struct dir*
*/
struct dir *thread_get_dir (int);
struct dir *thread_close_fd_dir(int);
```

In inode.c:
```
struct inode_disk{
    bool is_dir;
    struct inode_disk* parent;
}
```

We will remove the `extern struct lock filesys_lock` found in syscall.h. Instead, 

We will add/edit the following functions:

In filesys.c: 
```
bool chdir (const char *dir);
bool mkdir (const char *dir);
bool readdir (int fd, char *name);
bool isdir (int fd);
int inumber (int fd);
```
In process.c:
```tid_t
process_execute (const char *file_name){
 ...
 // Gives spawned process the same working directory as parent
}

void process_exit (void) {
// close the cwd
}
```
In syscall.c:
```
void close (int fd){
...
// needs to add synchronization by removing count
}  
int open (const char *file) {
...
// needs to add synchronization by adding to count
// use the algorithm delineated in section "Dealing with path arguments"
}
```
In filesys.c:
```
bool
filesys_remove (const char *name) {
// look up the directory containing the file(subdir) and remove the entry in the directory
// also free the file by flipping the bit in the free map using free_map_release (after acquiring the free_lock of course)
}
```
### Algorithms
**Anytime we mention "process `char* dir`", we will use the code provided in the spec for tokenizing a directory string, and then use the method below which deals with path arguments. **

#### Dealing with path arguments

For dealing with relative and absolute file paths, we will tokenize the `const char *dir` which is the input of every function. We check if the first character is '/' to see if the addresss is absolute or relative. We then look up the inode as followed:
1. if it was relative, we start with cwd, otherwise we start with root inode, which is referenced globally
2. get next token
3. find the next inode via dir_lookup
4. repeat step 2 and 3 until either an error occurred or we ran out of tokens, in which case we have located the file's or dir's inode

(Mini rationale: in this scheme, the algorithm does not discriminate against . and .. and any subdirectory name. This is because when we mk_dir, we insert . and .. right away)

#### New syscalls
Whenever we search in cwd, we reset the position to 0. 

- chdir: Close the old working directory (found in struct thread as ```cwd```). Then process the ```char *dir``` string to find corresponding ```struct dir* dir``` and replace the value of variable ```cwd```.
- mkdir: Process the ```char *dir```, then check sequentially each "folder" (except very last one) by checking if that "folder" exists in the block - if one doesn't exist during the sequential check, return False, otherwise then allocate a sector using ```free_map_allocate``` then pass output into `dir_create`. We also add a struct dir_entry pointing to the parent, as well as "." pointing to this current inode's sector.
- readdir: Use the `thread_get_dir()` function to translate fd int to `struct dir*`, and check sequentially with `dir_readdir` passing the name
- isdir: Use the `thread_get_dir()` to get the `dir*` which gets the `inode_disk` which has a `isdir` boolean variable.
- inumber: Use the `thread_get_dir()` to get the `struct dir*` which gets the `inode_disk` which gets the sector number. 



Also, we have the following changes:
- In process_execute (which is the function used in the exec syscall), we will copy cwd with pos reset of the old process to the new process. 
- In process_exit, we close the cwd. 


### Synchronization 
We've removed the global filesys lock from project2 (and therefore we must also delete all acquire/reacquires in the syscalls). 

For open/closes, instead use the counting variable found in `inode` to count the number of threads using that `inode`. If the counting variable is above zero, we disallow any removes for that directory. By default, every thread's working directory accounts for a +1 to the corresponding `inode`'s counting variable'.This synchronizes open/closes.

For synchronization of data creation and access (i.e. read_dir, file creation, file deletion), our method specifically comes from the portion using `cache_find_ent` in Part 1's synchronization. Any dir operation works with a specific inode of that dir, and accessing an inode implies accessing a block, whose synchronization is covered in part 1.


### Rationale
We'll use the array method from Project 2 Part 3 for ```struct dir*``` as we did for ```struct file*```. This allows a translation between ```int fd``` with ```struct dir*```. The directory structure is essentially made through `inode` having a parent, with all sequential blocks related to the `inode` corresponding to the child files and subdirectories. 

## Additional Questions 

### 1) Optional buffer cache feature
- write_behind: We use the timer mechanism found in project 1 (alarm), which will perform a timer interrupt every 10 seconds (or some other period we choose). We then use this timer interrupt to check all cache entries which have dirty bits as 1, and we will write all dirty blocks to disk. More specificially, for each dirty block, we
    * acquire cache_lock
    * increase ref_cnt to prevent other thread from evicting this block
    * release cache_lock
    * acquire the ent's lock
    * write it to disk
    * release ent's lock
    * acquire cache_lock, decrease ref_cnt, set dirty bit to 0, release cache_lock
- read_ahead: Fetch next 2 blocks within the file that we're currently reading or writing (or one or none if that goes past EOF), since it's likely that reading is always sequential access. This will involve usage of our `inode_fetch_block` function to determine which blocks to bring in our cache while a read/write is going on.
