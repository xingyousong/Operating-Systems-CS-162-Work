#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "lib/user/syscall.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "threads/vaddr.h"
#include "pagedir.h"
#include "filesys/filesys.h"
#include "filesys/fs_cache.h"
#include "devices/block.h"


static void syscall_handler (struct intr_frame *);
int is_valid_ptr(void* ptr);
void validate_ptr(void* ptr);
void validate_arg(void* ptr);
void validate_str(char* str);
void validate_str_with_len(char *str, int len);








void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

int read (int fd, void* buffer, unsigned size)
{
  if (fd == 0)
    return size;

  struct file *f = thread_get_file(fd);

  if (f == NULL)
    return -1;

  return file_read(f, buffer, size);
}

pid_t exec (const char *cmd_line)
{
	return process_execute(cmd_line);
}

int practice (int i)
{
	return i + 1;
}

void halt (void)
{
	shutdown_power_off();
}

void exit (int status)
{
	thread_current()->wait_status->exit_code = status;
  printf("%s: exit(%d)\n", thread_current ()->name, status);
	thread_exit();
}

int wait (pid_t pid)
{
	return process_wait(pid);
}

int write (int fd, const void* buffer, unsigned size)
{
  if (fd == 1)
  {
  	putbuf(buffer, size);
  	return size;
  }

  struct file *f = thread_get_file(fd);

  if (f == NULL)
    return 0;

  return file_write(f, buffer, size);
}

int open (const char *file)
{ // needs to be edited
  struct file *f = filesys_open(file);

  if (f == NULL)
    return -1;

  return thread_assign_fd(f);
}

void close (int fd)
{ // needs to be edited
  struct file *f = thread_close_fd(fd);

  if (f != NULL)
    file_close(f);
}

int is_valid_ptr (void *ptr)
{
  //Null verification
  if (ptr == NULL ||
    !is_user_vaddr(ptr) ||
    !pagedir_get_page(thread_current()->pagedir, ptr))
  {
    return 0;
  } else return 1;
}

/**
* Just validate the single pointer at ptr and exit if it's not valid
**/
void validate_ptr (void *ptr)
{
  if (!is_valid_ptr(ptr))
    exit(-1);
}

/**
* Validate the entire 4 bytes the args points to.
* Prevents ptr across mapped and unmapped memory
**/
void validate_arg (void *ptr)
{
  char *p = (char*) ptr;
  validate_ptr(p);
  validate_ptr(p + 3);
}

/*
* validate the str and copy to buff if it is valid
*/
void validate_str(char *str)
{
  int max_len = 1024;
  int i;

  for (i = 0; i < max_len && is_valid_ptr(str + i); i++) {
    if (str[i] == '\0')
      return;
  }

  exit(-1);
}

void validate_str_with_len(char *str, int len) {
  if (!is_valid_ptr(str) || !is_valid_ptr(str + len - 1)) {
    exit(-1);
  }
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
  uint32_t* args = ((uint32_t*) f->esp);

  //Validation of stack pointer
  validate_arg(args);

  uint32_t syscall_num = args[0];
  if (syscall_num == SYS_HALT) {
  	halt();
    return;
  }


  // buffer cache's stats syscall
  if (syscall_num == SYS_HIT_CNT) {
    f->eax = hit_cnt;
    return;
  }
  if (syscall_num == SYS_RESET_CNT) {
    reset_cnt();
    return;
  }
  if (syscall_num == SYS_READ_CNT) {
    f->eax = get_read_cnt(fs_device);
    return;
  }
  if (syscall_num == SYS_WRITE_CNT) {
    f->eax = get_write_cnt(fs_device); 
    return;
  }

  //Validation of the first argument of the syscall
  validate_arg(args + 1);

  if (syscall_num == SYS_PRACTICE) {
  	f->eax = practice((int) args[1]);
    return;
  }
  if (syscall_num == SYS_EXIT) {
  	exit((int) args[1]);
    return;
  }
  if (syscall_num == SYS_WAIT) {
  	f->eax = wait((pid_t) args[1]);
    return;
  }
  if (syscall_num == SYS_EXEC) {
    validate_str((char*) args[1]);
  	f->eax = exec(args[1]);
    return;
  }
  if(syscall_num == SYS_OPEN){
    validate_str((char*) args[1]);
    f->eax = open(args[1]);
    return;
  }
  if(syscall_num == SYS_REMOVE){
    validate_str((char*) args[1]);
    f->eax = filesys_remove (args[1]);
    return;
  }
  if (syscall_num == SYS_CHDIR){
    validate_str((char*) args[1]);
    f -> eax = chdir (args[1]);
    return;
  }
  if (syscall_num == SYS_MKDIR){
    validate_str((char*) args[1]);
    f -> eax = mkdir (args[1]);
    return;
  }
  if (syscall_num == SYS_ISDIR){
    f -> eax = isdir (args[1]);
    return;
  }
  if (syscall_num == SYS_INUMBER){
    f -> eax = inumber (args[1]);
    return;
  }

  struct file* file_input = thread_get_file (args[1]);

  if(syscall_num == SYS_FILESIZE){
    f->eax = file_length (file_input);
    return;
  }
  if(syscall_num == SYS_TELL){
    f->eax = file_tell(thread_get_file(args[1]));
    return;
  }
  if(syscall_num == SYS_CLOSE){
    close(args[1]);
    return;
  }

  //Validation of the second argument of the syscall
  validate_arg(args + 2);

  if(syscall_num == SYS_SEEK){
    file_seek(thread_get_file(args[1]), args[2]);
    return;
  }
  if(syscall_num == SYS_CREATE){
    validate_str((char*) args[1]);
    f->eax = filesys_create (args[1], args[2]);
    return;
  }

  if(syscall_num == SYS_READDIR){
    // printf("syscall read\n");
    // validate_str((char*) args[1]);
    f->eax = readdir(args[1], args[2]);
    return;
  }

  //Validation of the third argument of the syscall
  validate_arg(args + 3);

  if (syscall_num == SYS_WRITE) {
    validate_str_with_len(args[2], (uint32_t) args[3]);
    f->eax = write((int) args[1],
      (void*) args[2],
      (unsigned) args[3]);
    return;
  }

  if(syscall_num == SYS_READ){
    validate_str_with_len(args[2], (uint32_t) args[3]);
    f->eax = read(args[1],
      (void *) args[2],
      ((int) args[3]) );
    return;
  }
}
