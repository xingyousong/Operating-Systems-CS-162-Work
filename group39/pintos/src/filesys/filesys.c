#include "filesys/filesys.h"
#include "filesys/fs_cache.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"
#include "threads/malloc.h"


/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);


static int
get_next_part (char *part, const char **srcp) {
  const char *src = *srcp;
  char *dst = part;
  /* Skip leading slashes.  If it’s all slashes, we’re done. */
  while (*src == '/')
    src++;
  if (*src == '\0')
    return 0;
  /* Copy up to NAME_MAX character from SRC to DST.  Add null terminator. */
  while (*src != '/' && *src != '\0') {
    if (dst < part + NAME_MAX)
      *dst++ = *src;
    else
      return -1;
    src++;
  }
  *dst = '\0';
  /* Advance source pointer. */
  *srcp = src;
  return 1;
}

bool output_follow_file_path(const char * srcp_input, struct dir ** dir,
                             char part[NAME_MAX+1], bool * part_is_dir) {
  struct inode* cur_inode = NULL;
  struct inode* temp_inode = NULL;
  if (*srcp_input == '\0') {
    return false;
  }
  if (*srcp_input != '/') {
    cur_inode = inode_reopen(thread_current() -> cwd);
  } else {
    cur_inode = inode_open(ROOT_DIR_SECTOR);
  }
  bool found = true;
  while (found && get_next_part (part, &srcp_input)) {
    struct inode *in = inode_reopen(cur_inode);
    struct dir *cur_dir = dir_open(in);
    if (!dir_lookup(cur_dir, part, &temp_inode)) {
      found = false;
    }

    dir_close(cur_dir);
    if (temp_inode && inode_is_dir(temp_inode)) {
      inode_close(cur_inode);
      *part_is_dir = true;
      cur_inode = temp_inode;
    } else {
      *part_is_dir = false;
      found = false;
    }
  }

  *dir = dir_open(cur_inode);
  if (get_next_part(part, &srcp_input))
      return false;

  if (temp_inode && cur_inode != temp_inode) {
    inode_close(temp_inode);
  }
  return true;
}

bool chdir (const char *dir) {
  struct dir * new_dir = NULL;
  char part[NAME_MAX + 1];
  bool part_is_dir = true;
  if (!output_follow_file_path(dir, &new_dir, part, &part_is_dir)) {
    return false;
  }
  if (!part_is_dir) {
    return false;
  }

  inode_close(thread_current()->cwd);
  thread_current()->cwd = inode_reopen(dir_get_inode(new_dir));
  dir_close(new_dir);
  return true;
}

bool mkdir (const char *dir) {
  struct dir * curr_dir = NULL;
  char part[NAME_MAX + 1];
  bool part_is_dir = true;
  if (!output_follow_file_path(dir, &curr_dir, part, &part_is_dir))
    return false;

  block_sector_t sector = -1;
  if (!free_map_allocate(1, &sector)) {
    return false;
  }
  dir_create(sector, 25);
  if (!dir_add(curr_dir, part, sector)) {
    dir_close(curr_dir);
    return false;
  }
  struct inode * new_inode = inode_open(sector);
  inode_set_dir(new_inode, true);

  struct dir * new_dir = dir_open(new_inode);

  dir_add(new_dir, ".", sector);
  dir_add(new_dir, "..", inode_get_inumber(dir_get_inode(curr_dir)));

  dir_close(curr_dir);
  dir_close(new_dir);
  return true;
}


bool readdir (int fd, char *name){
  struct file * cur_file = thread_get_file(fd);
  struct inode * cur_inode = file_get_inode(cur_file);
  struct dir * cur_dir = dir_open(inode_reopen(cur_inode));

  dir_pos_change(cur_dir, file_tell(cur_file));

  if(dir_readdir(cur_dir, name)) {
    file_seek(cur_file, dir_get_pos(cur_dir));
    dir_close(cur_dir);
    return true;
  }
  else{
    dir_close(cur_dir);
    return false;
  }

}

bool isdir (int fd) {
  struct file * cur_dir = thread_get_file(fd);
  return inode_is_dir(file_get_inode(cur_dir));
}

int inumber (int fd) {
  struct file * cur_dir = thread_get_file(fd);
  return inode_get_inumber(file_get_inode(cur_dir));
}

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format)
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format)
    do_format ();

  free_map_open ();

  thread_current()->cwd = inode_open(ROOT_DIR_SECTOR);
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void)
{
  free_map_close ();
  cache_flush();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size)
{
  if (*name != '/' && !(thread_current() -> cwd))
    return NULL;
  block_sector_t inode_sector = 0;

  struct dir *dir = NULL;
  char part[NAME_MAX + 1];
  bool part_is_dir = true;
  if (!output_follow_file_path(name, &dir, part, &part_is_dir))
    return false;
  if (part_is_dir)
    return false;

  bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size)
                  && dir_add (dir, part, inode_sector));

  if (success) {
    struct inode * in = NULL;
    dir_lookup(dir, part, &in);
    inode_set_dir(in, false);
    inode_close(in);
  } else if (inode_sector != 0) {
    free_map_release (inode_sector, 1);
  }

  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  if (!strcmp(name, "/")) {
    return file_open(inode_open(ROOT_DIR_SECTOR));
  }
  if (*name != '/' && !(thread_current() -> cwd)) {
      return NULL;
  }

  struct dir *dir = NULL;
  char part[NAME_MAX + 1];
  bool part_is_dir = true;
  if (!output_follow_file_path(name, &dir, part, &part_is_dir)) {
      return NULL;
  }

  if (part_is_dir && strcmp(part, ".")) {

    struct inode * parent = NULL;
    dir_lookup(dir, "..", &parent);
    dir_close(dir);
    dir = dir_open(parent);
  }

  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, part, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name)
{
  struct dir *dir = NULL;
  char part[NAME_MAX + 1];
  bool part_is_dir = true;
  if(!output_follow_file_path(name, &dir, part, &part_is_dir))
    return false;
  if (part_is_dir) {

    block_sector_t inum = inode_get_inumber(dir_get_inode(dir));
    if (inum == ROOT_DIR_SECTOR) {
      dir_close(dir);
      return false;
    }
    if (inum == inode_get_inumber(thread_current()->cwd)) {
      thread_current()->cwd = NULL;
    }

    struct inode * parent = NULL;
    dir_lookup(dir, "..", &parent);

    dir_close(dir);
    dir = dir_open(parent);;
  }

  bool success = dir != NULL && dir_remove (dir, part);

  dir_close (dir);

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if(!dir_create(ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");

  /* Mark the root directory inode as a directory */
  struct inode * new_inode = inode_open(ROOT_DIR_SECTOR);
  inode_set_dir(new_inode, true);
  inode_close(new_inode);

  free_map_close ();
  printf ("done.\n");
}
