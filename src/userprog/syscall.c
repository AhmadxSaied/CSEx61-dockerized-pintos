#include "userprog/syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "lib/user/syscall.h"
#include "threads/vaddr.h"
#include <pagedir.c>

static void syscall_handler(struct intr_frame *);
int get_int(int **esp);
char *get_char_ptr(char ***esp);
void *get_void_ptr(void ***esp);
void validate_void_ptr(void *ptr);

// system_dependent
void sys_halt_wrapper();
void sys_exit_wrapper(struct intr_frame *f);
void sys_exec_wrapper(struct intr_frame *f);
void sys_wait_wrapper(struct intr_frame *f);

// file dependent
void sys_create_wrapper(struct intr_frame *f);
void sys_remove_wrapper(struct intr_frame *f);
void sys_open_wrapper(struct intr_frame *f);
void sys_filesize_wrapper(struct intr_frame *f);
void sys_read_wrapper(struct intr_frame *f);
void sys_write_wrapper(struct intr_frame *f);
void sys_seek_wrapper(struct intr_frame *f);
void sys_tell_wrapper(struct intr_frame *f);
void sys_close_wrapper(struct intr_frame *f);

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// SYS_HALT = 0,                   /* Halt the operating system. */
// SYS_EXIT = 1,                   /* Terminate this process. */
// SYS_EXEC = 2,                   /* Start another process. */
// SYS_WAIT = 3,                   /* Wait for a child process to die. */
// SYS_CREATE = 4,                 /* Create a file. */
// SYS_REMOVE = 5,                 /* Delete a file. */
// SYS_OPEN = 6,                   /* Open a file. */
// SYS_FILESIZE = 7,               /* Obtain a file's size. */
// SYS_READ = 8,                   /* Read from a file. */
// SYS_WRITE = 9,                  /* Write to a file. */
// SYS_SEEK = 10,                   /* Change position in a file. */
// SYS_TELL = 11,                   /* Report current position in a file. */
// SYS_CLOSE = 12,                  /* Close a file. */

static void
syscall_handler(struct intr_frame *f UNUSED)
{
  printf("system call!\n");
  int system_call_type = f->esp;
  f->esp +=1;

  if (system_call_type == SYS_HALT)
  {
    sys_halt_wrapper();
  }
  else if (system_call_type == SYS_EXIT)
  {
    sys_exit_wrapper(f);
  }
  else if (system_call_type == SYS_EXEC)
  {
    sys_exec_wrapper(f);
  }
  else if (system_call_type == SYS_WAIT)
  {
    sys_wait_wrapper(f);
  }
  else if (system_call_type == SYS_CREATE)
  {
    sys_create_wrapper(f);
  }
  else if (system_call_type == SYS_REMOVE)
  {
    remove(f);
  }
  else if (system_call_type == SYS_OPEN)
  {
    open(f);
  }
  else if (system_call_type == SYS_FILESIZE)
  {
    filesize(f);
  }
  else if (system_call_type == SYS_READ)
  {
    sys_read_wrapper(f);
  }
  else if (system_call_type == SYS_WRITE)
  {
    sys_write_wrapper(f);
  }
  else if (system_call_type == SYS_SEEK)
  {
    sys_seek_wrapper(f);
  }
  else if (system_call_type == SYS_TELL)
  {
    sys_tell_wrapper(f);
  }
  else if (system_call_type == SYS_CLOSE)
  {
    sys_close_wrapper(f);
  }
  thread_exit();
}

// terminated pintos
void halt(void)
{
  shutdown_power_off();
}
/* terminates the current user program, returning status to kernel
   status = 0 indicates success while status != 0 means an error
*/
void exit(int status)
{
  // TODO
}
/* runs a new process and returns its pid
  parent process cannot return from the exec until it knows whether child
  process syccessfully loaded its executable
  has synchronization
*/
pid_t exec(const char *cmd_line)
{
  return process_execute(cmd_line);
}

/*  wait for a child process pid and retrieves the child_exit status
    fails in the following
    pid does refer to a direct child of the process
    // pid is valid iff it is returned from a successfull exec call

 */
int wait(pid_t pid)
{
  // TODO
  return process_wait (pid) ;
}
/*
  creates a new file called file with size initial size
  return true on success and false otherwise
*/
bool create(const char *file, unsigned initial_size)
{
  // TODO
}
/*
 deletes a file with name given in argument
 true returned on success and false otherwise
 --- removing an open file doesnt close it ---
*/
bool remove(const char *file)
{
  // TODO
}
/*
  returns a non-negatve integer called file descriptor (fd) or -1 if we couldnt open the file
  fd == 0 STDIN
  fd == 1 STDOUT
*/
int open(const char *file)
{
  // TODO
}
/*
  returns file size in bytes
*/
int filesize(int fd)
{
  // TODO
}
/*
  reads (size) bytes from the file with fd into buffer
  return the number of bytes actually read or -1 if couldnt read
  Fd 0 reads from the keybboard using input_getc()
*/
int read(int fd, void *buffer, unsigned size)
{
  // TODO
}
/*
  writes (size) bytes from buffer to open file fd
  returns number of bytes actually written
*/
int write(int fd, const void *buffer, unsigned size)
{
  // TODO
}
/*
  changes the next byte to be read or written in open file fd
*/
void seek(int fd, unsigned position)
{
  // TODO
}
/*
  returns the position of the next byte to be read or written in open file fd
*/
unsigned tell(int fd)
{
  // TODO
}
/*
  closes file descriptor fd
*/
void close(int fd)
{
  // TODO
}

char *get_char_ptr(char ***esp)
{
  return **esp;
}
int get_int(int **esp)
{
  return **esp;
}
void *get_void_ptr(void ***esp)
{
  return **esp;
}
void validate_void_ptr(void *ptr)
{
  if (!(ptr != NULL && is_user_vaddr(ptr)))
  {
    exit(-1);
  }
}

void sys_halt_wrapper()
{
  halt();
}
void sys_exit_wrapper(struct intr_frame *f)
{
  int status = get_int((int **)&f->esp);
  f->esp +=1;
  exit(status);
}
void sys_exec_wrapper(struct intr_frame *f)
{
  char *cmd_line = get_char_ptr((char ***)&f->esp);
  validate_void_ptr((void *)cmd_line);
  f->esp +=1;
  f->eax = exec(cmd_line);
}
void sys_wait_wrapper(struct intr_frame *f)
{
  pid_t pid = get_int((int **)&f->esp);
  f->esp +=1;
  f->eax = wait(pid);
}

// file dependent
void sys_create_wrapper(struct intr_frame *f)
{
  char *file = get_char_ptr((char ***)&f->esp);
  validate_void_ptr((void *)file);
  f->esp +=1;
  unsigned int initial_size = (unsigned int)get_int((int **)&f->esp);
  f->esp +=1;

  f->eax = create(file, initial_size);
}
void sys_remove_wrapper(struct intr_frame *f)
{
  char *file = get_char_ptr((char ***)&f->esp);
  validate_void_ptr((void *)file);
  f->esp +=1;
  f->eax = remove(file);
}
void sys_open_wrapper(struct intr_frame *f)
{
  char *file = get_char_ptr((char ***)&f->esp);
  validate_void_ptr((void *)file);
  f->esp +=1;
  f->eax = open(file);
}
void sys_filesize_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp +=1;
  f->eax = filesize(fd);
}
void sys_read_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp +=1;
  void *buffer = get_void_ptr((void **)f->esp);
  validate_void_ptr(buffer);
  f->esp +=1;
  unsigned int size = (unsigned int)get_int((int **)&f->esp);
  f->esp +=1;
  f->eax = read(fd, buffer, size);
}
void sys_write_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp +=1;
  void *buffer = get_void_ptr((void **)f->esp);
  validate_void_ptr(buffer);
  f->esp +=1;
  unsigned int size = (unsigned int)get_int((int **)&f->esp);
  f->esp +=1;
  f->eax = write(fd, buffer, size);
}
void sys_seek_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp +=1;
  unsigned int position = (unsigned int)get_int((int **)&f->esp);
  f->esp +=1;
  seek(fd, position);
}
void sys_tell_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp +=1;
  f->eax = tell(fd);
}
void sys_close_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp +=1;
  close(fd);
}