#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/fs_cache.h"

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

char ZEROS_BLOCK[BLOCK_SECTOR_SIZE];
char UNALLOCATED_BLOCK[BLOCK_SECTOR_SIZE];

block_sector_t inode_fetch_block(struct inode_disk *id, off_t pos);
bool inode_extend(struct inode *id, off_t size);

block_sector_t inode_fetch_block(struct inode_disk *id, off_t pos)
{
  // This is the case in which we can pull the block from direct pointers
  if (pos < DIRECT_POINTER_SIZE)
  {
    int pointerNumber = pos / BLOCK_SECTOR_SIZE;
    return id->direct_pointers[pointerNumber];
  }
  // This is the case in which our block is in the doubly indirect pointer
  if (pos < MAX_FILE_LEN)
  {
    if (id->doubly_indirect == UNALLOCATED) {
      return UNALLOCATED;
    }
    // Do this to get rid of direct pointer math
    pos = pos - DIRECT_POINTER_SIZE;
    // Do this to get the indirect block number,
    // an indirect pointer stores 2^7 pointers which store 2^9 bytes in total = 2^16
    int indirectPointerNumber = pos / TWOTO16;

    block_sector_t doubly_indirect_buffer[128];
    cache_read(id->doubly_indirect, doubly_indirect_buffer);
    // Now buffer contains the pointers to indirect blocks

    block_sector_t indirect = doubly_indirect_buffer[indirectPointerNumber];

    // Do this to figure out the new offset within the indirect pointer
    pos = pos - indirectPointerNumber * TWOTO16;

    if (indirect == UNALLOCATED) {
      return UNALLOCATED;
    }

    block_sector_t indirect_buffer[128];
    cache_read(indirect, indirect_buffer);

    // Figuring out the block index to go into
    int pointerNumber = pos / BLOCK_SECTOR_SIZE;

    return indirect_buffer[pointerNumber];
  }
  // This is the case in which the offset is larger than the maximum supported file.
  return -1;
}

bool inode_extend(struct inode *inode, off_t size)
{
  off_t original_size = size;
  struct inode_disk *id = &inode->data;

  block_sector_t sector;
  bool success;

  // Direct pointers
  int i = 0;

  for (i = 0; i < DIRECT_POINTER_COUNT; i++) {
    if (size <= 512 * i && id->direct_pointers[i] != UNALLOCATED)
    {
      free_map_release(id->direct_pointers[i], 1);
      id->direct_pointers[i] = UNALLOCATED;
    }
    if (size > 512 * i && id->direct_pointers[i] == UNALLOCATED)
    {
      success = free_map_allocate(1, &sector);
      if (!success) {
        inode_extend(inode, id->length);
        return false;
      }

      //Zero out the newly allocated block
      cache_write(sector, ZEROS_BLOCK);
      id->direct_pointers[i] = sector;
    }
  }

  // This allows the function to return when the size can fit within the direct blocks
  if (id->doubly_indirect == UNALLOCATED && size <= DIRECT_POINTER_SIZE) {
    id->length = original_size;
    cache_write(inode->sector, id);
    return true;
  }

  block_sector_t doubly_indirect_buffer[128];

  // Allocate a doubly indirect block if needed,
  // also update the doubly_indirect_pointer to point
  // to this new block.
  if (id->doubly_indirect == UNALLOCATED) {
    memset(doubly_indirect_buffer, UNALLOCATED, 512);
    success = free_map_allocate(1, &sector);

    if (!success) {
      inode_extend(inode, id->length);
      return false;
    }

    // Zero out the newly allocated block
    cache_write(sector, doubly_indirect_buffer);
    id->doubly_indirect = sector;
  } else {
    cache_read(id->doubly_indirect, doubly_indirect_buffer);
  }

  // Adjust size and eliminate the direct pointers space to make math easier
  size = size - DIRECT_POINTER_SIZE;
  block_sector_t indirect_buffer[128];

  // Indirect pointers
  int j = 0;
  for (i = 0; i < 128; i++)
  {
    if (size <= i * 128 * BLOCK_SECTOR_SIZE && doubly_indirect_buffer[i] != UNALLOCATED)
    {
      free_map_release(doubly_indirect_buffer[i], 1);
      doubly_indirect_buffer[i] = UNALLOCATED;
    }
    if (size > i * 128 * BLOCK_SECTOR_SIZE && doubly_indirect_buffer[i] == UNALLOCATED)
    {
      success = free_map_allocate(1, &sector);
      if (!success)
      {
        inode_extend(inode, id->length);
        return false;
      }

      // Make all the needed indirect block to hold unallocated pointers
      cache_write(sector, UNALLOCATED_BLOCK);
      doubly_indirect_buffer[i] = sector;
    }

    // If the indirect pointer doesn't exist,
    // we need to set indirect_buffer to -1s.
    if(doubly_indirect_buffer[i] == UNALLOCATED)
    {
      break;
    }
    else
    {
      cache_read(doubly_indirect_buffer[i], indirect_buffer);
    }

    for (j = 0; j < 128; j++)
    {
      int block_space = j * BLOCK_SECTOR_SIZE;
      if (size <= i * 128 * BLOCK_SECTOR_SIZE + block_space && indirect_buffer[j] != UNALLOCATED)
      {
        free_map_release(indirect_buffer[j], 1);
        indirect_buffer[j] = UNALLOCATED;
      }
      if (size > i * 128 * BLOCK_SECTOR_SIZE + block_space && indirect_buffer[j] == UNALLOCATED)
      {
        success = free_map_allocate(1, &sector);
        if (!success)
        {
          inode_extend(inode, id->length);
          return false;
        }

        //Zero out the newly allocated block
        cache_write(sector, ZEROS_BLOCK);
        indirect_buffer[j] = sector;
      }
    }
    cache_write(doubly_indirect_buffer[i], indirect_buffer);
  }
  cache_write(id->doubly_indirect, doubly_indirect_buffer);
  id->length = original_size;

  cache_write(inode->sector, id);
  return true;
}


bool inode_is_dir(struct inode * inode) {
  return (inode->data).is_dir;
}

void inode_set_dir(struct inode * inode, bool is_dir) {
  (inode->data).is_dir = is_dir;
}

void inode_add_child(struct inode * inode) {
  (inode->data).children += 1;
}

void inode_remove_child(struct inode * inode) {
  (inode->data).children -= 1;
}

int inode_get_children(struct inode * inode) {
  return (inode->data).children;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
  memset(ZEROS_BLOCK, 0, BLOCK_SECTOR_SIZE);
  memset(UNALLOCATED_BLOCK, UNALLOCATED, BLOCK_SECTOR_SIZE);
  buff_cache_init();
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length)
{
  bool success = false;

  ASSERT (length >= 0);

  // make a dummy struct inode so that we can call inode_extend
  struct inode i;
  i.sector = sector;
  struct inode_disk *disk_inode = &i.data;

  if (disk_inode != NULL)
    {
      memset(disk_inode, UNALLOCATED, BLOCK_SECTOR_SIZE);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;

      //Init directs and indirect to -1
      memset(&disk_inode->direct_pointers, (char) UNALLOCATED,
        DIRECT_POINTER_COUNT * 4);
      disk_inode->doubly_indirect = UNALLOCATED;
      disk_inode->length = length;

      cache_write(sector, disk_inode);
      disk_inode->children = 0;
      success = true;
    }

    // pre allocate that size
    inode_extend(&i, length);
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;
  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {

      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }
  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;
  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;

  lock_init(&inode->inode_lock);
  cache_read(inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  lock_acquire(&inode->inode_lock);
  if (inode != NULL)
    inode->open_cnt++;
  lock_release(&inode->inode_lock);
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          free_map_release (inode->sector, 1);
          inode_extend(inode, 0);
        }

      cache_write(inode->sector, (void *) &(inode->data));

      free (inode);
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  lock_acquire(&inode->inode_lock);
  inode->removed = true;
  lock_release(&inode->inode_lock);
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;

  lock_acquire (&(inode->inode_lock));

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = inode_fetch_block(&(inode->data), offset);

      if (sector_idx == -1)
      {
        lock_release (&(inode->inode_lock));
        return bytes_read;
      }

      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode->data.length - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          cache_read(sector_idx, buffer + bytes_read);
        }
      else
        {
          // Read that CHUNK_SIZE amount of memory starting at
          // SECTOR_OFS at the sector numbered at SECTOR_IDX.
          cache_read_len(sector_idx, buffer + bytes_read, sector_ofs, chunk_size);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }

  lock_release (&(inode->inode_lock));
  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  struct inode_disk *id = &(inode->data);

  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  lock_acquire(&(inode->inode_lock));
  if (inode->deny_write_cnt) {
    lock_release(&(inode->inode_lock));
    return 0;
  }


  // Check to make sure that we can allocate, and if so, extend the inode.
  off_t needed_at_least = offset + size;
  if(needed_at_least > id->length && !inode_extend(inode, needed_at_least))
  {
    lock_release (&(inode->inode_lock));
    return 0;
  }

  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = inode_fetch_block(&(inode->data), offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;

      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          cache_write(sector_idx, buffer + bytes_written);
        }
      else
        {
          cache_write_len(sector_idx, buffer + bytes_written, sector_ofs,
            chunk_size);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }

  lock_release (&(inode->inode_lock));
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  lock_acquire(&inode->inode_lock);
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  lock_release(&inode->inode_lock);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  lock_acquire(&inode->inode_lock);
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
  lock_release(&inode->inode_lock);
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}
