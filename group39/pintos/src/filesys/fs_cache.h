#ifndef FILESYS_CACHE
#define FILESYS_CACHE

#include "filesys/fs_cache.h"
#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include <debug.h>
#include "filesys/inode.h"
#include "threads/malloc.h"
#include "threads/synch.h"

#define CACHE_SIZE 64

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
    // true if this ent has been used recently, needed for eviction in clock algorithm
    bool use;

    uint32_t ref_cnt;
    struct lock lock; // to serialize any load, read, and write

    char data[BLOCK_SECTOR_SIZE]; // stores the actual block
};

// our buffer cache
extern struct cache_ent buff_cache[CACHE_SIZE];

// stats for testing
extern int hit_cnt;
extern int miss_cnt;

void reset_cnt(void);

/*
* Initialize eviction_lock,
* 64 buff_cache_ent's lock,
* set all ref_cnt to 0
* and set all the valid bits to false.
*/
void buff_cache_init(void);

/*
* Read memory of len LEN starting at OFFSET of
* this block from the cache to the buffer.
*/
void cache_read_len(block_sector_t block,
    void* buffer,
    uint32_t offset,
    uint32_t len);

/*
* Call cache_read_len with offset 0 and len the
* size of a block. For ease of integrating code.
*/
void cache_read(block_sector_t block, void* buffer);



/*
* write buffer to this block in the cache starting
* at OFFSET to len LEN
*/
void cache_write_len(block_sector_t block,
    const void* buffer,
    uint32_t offset,
    uint32_t len);

/*
* Call cache_write_len with offset 0 and len the
* size of a block. For ease of integrating code.
*/
void cache_write(block_sector_t block, const void* buffer);

void cache_flush();

#endif /* filesys/fs_cache.h */
