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
#include "devices/block.h"
#include "filesys/inode.h"


// Current clock hand. Between 0 to 63, inclusive.
uint32_t clk_hand = 0;

/*
* Ensure that when locating block, no other
* thread is evicting, also for sequentializing
* eviction
*/
struct lock cache_lock;

// Our buffer cache
struct cache_ent buff_cache[CACHE_SIZE];

int hit_cnt;
int miss_cnt;

void reset_cnt(void) {
	hit_cnt = 0;
	miss_cnt = 0;
}

/*
* initialize eviction_lock,
* 64 buff_cache_ent's lock,
* set all ref_cnt to 0
* and set all the valid bits to false
*/
void buff_cache_init(void) {

	// init global cache lock
	lock_init(&cache_lock);

	// for each cache entry
	int i;
	for (i = 0; i < CACHE_SIZE; i++) {
		struct cache_ent *e = buff_cache + i;

		// init the cache entry's lock
		lock_init(&e->lock);

		// default values for some metadata
		e->ref_cnt = 0;
		e->valid = 0;
		e->use = 0;
	}

	reset_cnt();
}

/*
* Called when the cache is full and
* we need to evict a cache entry for
* new data. Implements the clock algorithm.
* Needs to be externally synchronized.
*/
struct cache_ent* cache_evict() {
	while (1) {

		// move clock hand
		clk_hand = (clk_hand + 1) % CACHE_SIZE;

		struct cache_ent *e = buff_cache + clk_hand;

		/* we don't check valid bit because if we have to evict,
		   all cache entry should have been invalid */
		if (!e->ref_cnt && !e->use) {

			// write to disk if it was dirty
			if (e->dirty) {
				block_write(fs_device, e->block, e->data);
				e->dirty = 0;
			}

			e->valid = 0;
			return e;
		}

		e->use = 0;
	}
}

/*
* Find a cache entry corresponding to the desired
* block or load one. Will evict other blocks if
* necessary. Will always acquire the entry's lock
* before returning.
*
* Caller must eventually release the entry's lock
*/
struct cache_ent* cache_find_ent(block_sector_t block) {
	// acquire cache_lock to prevent other threads from evicting
	lock_acquire(&cache_lock);

	int i;
	// for recording an free invalid ent
	struct cache_ent *free_ent = NULL;
	struct cache_ent *e;

	// change the point increment size
	struct cache_ent* buff_cache_ptr = buff_cache;

	for (i = 0; i < CACHE_SIZE; i++) {

		e = buff_cache_ptr + i;

		// record any invalid free ent
		if (!e->valid) {
			free_ent = e;
		}

		// if valid, compare desired BLOCK to each cache entry
		else if(e->block == block) {
			hit_cnt++;

			// cache hit
			e->ref_cnt++;
			lock_release(&cache_lock);

			// acquire cache ent's lock to wait to assess block data
			// it is caller's responsibility to release ent's lock
			lock_acquire(&e->lock);
			return e;
		}
	}

	// cache miss
	miss_cnt++;
	if (free_ent) {
		e = free_ent;
	} else {
		e = cache_evict();
	}

	// increase ref_cnt to prevent other thread from evicting it
	e->ref_cnt++;

	// declare this ent is now loaded and valid
	e->block = block;
	e->valid = 1;

	// since we just evicted this block, no other thread is using
	// this block; therefore, we must succeed acquiring this lock
	lock_acquire(&e->lock);

	// note that we must release cache_lock AFTER we acquire ent's
	// lock to ensure that loading new data happens before any assess
	lock_release(&cache_lock);

	// load in the new data
	block_read (fs_device, block, &e->data);

	return e;
}

/*
* Read memory of len LEN starting at OFFSET of
* this block from the cache to the buffer.
*/
void cache_read_len(block_sector_t block,
    void* buffer,
    uint32_t offset,
    uint32_t len) {

	// if read involves an unallocated block, just
	// read out all zeros
	if (block == UNALLOCATED) {
		memset(buffer, 0, len);
		return;
	}
	// call cache_find_ent to locate a cache ent
	// this funcion also handled eviction
	struct cache_ent *e = cache_find_ent(block);

	// copy from ent's data to buffer
	memcpy(buffer, e->data + offset, len);

	// for eviction clock algorithm
	e->use = 1;

	lock_release(&e->lock);

	// decrease ref_cnt to signal we are done with this block
	lock_acquire(&cache_lock);
	e->ref_cnt--;
	lock_release(&cache_lock);
}

/*
* Call cache_read_len with offset 0 and len the
* size of a block. For ease of integrating code.
*/
void cache_read(block_sector_t block, void* buffer) {
	cache_read_len(block, buffer, 0, BLOCK_SECTOR_SIZE);
}



/*
* Write buffer to this block in the cache starting
* at OFFSET to len LEN.
*/
void cache_write_len(block_sector_t block,
    const void* buffer,
    uint32_t offset,
    uint32_t len) {

	// call cache_find_ent to locate a cache ent
	// this funcion also handled eviction
	struct cache_ent *e = cache_find_ent(block);

	// copy from ent's data to buffer
	memcpy(e->data + offset, buffer, len);
	e->dirty = 1;

	// for eviction clock algorithm
	e->use = 1;

	lock_release(&e->lock);

	// decrease ref_cnt to signal we are done with this block
	lock_acquire(&cache_lock);
	e->ref_cnt--;
	lock_release(&cache_lock);
}

/*
* Call cache_write_len with offset 0 and len the
* size of a block. For ease of integrating code.
*/
void cache_write(block_sector_t block, const void* buffer) {
	cache_write_len(block, buffer, 0, BLOCK_SECTOR_SIZE);
}

void cache_flush() {
		// for each cache ent
	int i;
	for (i = 0; i < CACHE_SIZE; i++) {
		struct cache_ent *e = buff_cache + i;

		if (e->valid && e->dirty) {
			block_write(fs_device, e->block, e->data);
		}
	}
}
