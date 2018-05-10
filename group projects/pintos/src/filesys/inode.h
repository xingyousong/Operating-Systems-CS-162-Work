#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"
#include "list.h"
#include "threads/synch.h"

struct bitmap;

void inode_init (void);
bool inode_create (block_sector_t, off_t);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);


/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define DIRECT_POINTER_COUNT 123
#define DIRECT_POINTER_SIZE 62976
#define MAX_FILE_LEN 8451584
#define UNALLOCATED (block_sector_t) -1
#define TWOTO16 65536

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    /* In order to use up all space we will have a magic number, length,
     * doubly indirect block_sector_t and we will fill up the rest with direct pointers
     * This means we'll have 123 direct pointers.
     * We can represent this with an array of block_sector_t.
     */
    block_sector_t direct_pointers[DIRECT_POINTER_COUNT]; /* Direct pointers array */
    block_sector_t doubly_indirect;
    int is_dir;                         /* 1 = directory, 0 = regular file */
    off_t length;                       /* File size in bytes. */
    int children;                       /* Number of children */
    unsigned magic;                     /* Magic number. */
  };


/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
    struct lock inode_lock;
  };


bool inode_is_dir(struct inode *);
void inode_set_dir(struct inode *, bool);

void inode_add_child(struct inode *);
void inode_remove_child(struct inode *);
int inode_get_children(struct inode *);

#endif /* filesys/inode.h */
