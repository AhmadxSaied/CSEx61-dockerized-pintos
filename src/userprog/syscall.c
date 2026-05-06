#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "lib/user/syscall.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
#include <stdlib.h>

struct lock fs_lock;

static void syscall_handler(struct intr_frame *);
int get_int(int **esp);
char *get_char_ptr(char ***esp);
void *get_void_ptr(void ***esp);
void validate_void_ptr(void *ptr);
void validate_buffer(void* buffer,unsigned size);
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
  lock_init(&fs_lock);
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
syscall_handler(struct intr_frame *f UNUSED) //gom3a
{
  // printf("system call!\n");
  validate_void_ptr(f->esp);
  void * orig = f->esp;
  int system_call_type = *(int *)f->esp;  // ✓ CORRECT - dereference to get value
  f->esp = (int *)f->esp + 1;             // ✓ CORRECT - increment by 4 bytes
  
  switch(system_call_type) {
    case SYS_HALT:
      sys_halt_wrapper();
      break;
    case SYS_EXIT:
      sys_exit_wrapper(f);
      break;
    case SYS_EXEC:
      sys_exec_wrapper(f);
      break;
    case SYS_WAIT:
      sys_wait_wrapper(f);
      break;
    case SYS_CREATE:
      sys_create_wrapper(f);
      break;
    case SYS_REMOVE:
      sys_remove_wrapper(f);
      break;
    case SYS_OPEN:
      sys_open_wrapper(f);
      break;
    case SYS_FILESIZE:
      sys_filesize_wrapper(f);  // ✓ FIXED - was calling filesize(f)
      break;
    case SYS_READ:
      sys_read_wrapper(f);
      break;
    case SYS_WRITE:
      sys_write_wrapper(f);
      break;
    case SYS_SEEK:
      sys_seek_wrapper(f);
      break;
    case SYS_TELL:
      sys_tell_wrapper(f);
      break;
    case SYS_CLOSE:
      sys_close_wrapper(f);
      break;
    default:
      exit(-1);  // Invalid syscall
      break;
  }
  f->esp = orig;
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
  struct thread *cur = thread_current();

  // my child struct (current thread IS the child), the one in my parent's list
  struct child *cur_child = cur->self_child;
  if (cur_child != NULL) {
    cur_child->exitStatus = status;
    sema_up(&cur_child->exit);
  }
  thread_exit();
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

static struct open_file *findfile_by_fd(int fd){
  struct thread *cur = thread_current();
  struct list_elem *e;

  for (e=list_begin(&cur->files); e!=list_end(&cur->files); e= list_next(e)){
    struct open_file *of = list_entry(e,struct open_file,elem);
    if(of->fd ==fd){
      return of;
    }
  }
  return NULL; //couldn't get the file descriptor
}


bool create(const char *file, unsigned initial_size)
{
  if(file ==NULL)exit(-1);

  lock_acquire(&fs_lock);
  bool success = filesys_create(file,initial_size);
  lock_release(&fs_lock);

  return success;
}
/*
 deletes a file with name given in argument
 true returned on success and false otherwise
 --- removing an open file doesnt close it ---
*/
bool remove(const char *file)
{
  if(file==NULL) exit(-1);

  lock_acquire(&fs_lock);
  bool success =filesys_remove(file);
  lock_release(&fs_lock);

  return success;
}
/*
  returns a non-negatve integer called file descriptor (fd) or -1 if we couldnt open the file
  fd == 0 STDIN
  fd == 1 STDOUT
*/
int open(const char *file)
{
  if(file==NULL) exit(-1);

  lock_acquire(&fs_lock);
  struct file *opened_file = filesys_open(file);
  if(opened_file ==NULL){
    lock_release(&fs_lock);
    return -1;
  }
  struct open_file *of =malloc(sizeof(struct open_file));
  if(of==NULL){
    file_close(opened_file);
    lock_release(&fs_lock);
    return -1;
  }
  struct thread *cur = thread_current();
  of->file = opened_file;
  of->fd = cur->next_fd++; // increamenet for next file
  list_push_back(&cur->files,&of->elem);

  lock_release(&fs_lock);
  return of->fd;
}
/*
  returns file size in bytes
*/
int filesize(int fd)
{
  lock_acquire(&fs_lock);
  struct open_file *of =findfile_by_fd(fd);
  if(of ==NULL){
    lock_release(&fs_lock);
    return -1;
  }
  int size = file_length(of->file);
  lock_release(&fs_lock);
  return size ;
}
/*
  reads (size) bytes from the file with fd into buffer
  return the number of bytes actually read or -1 if couldnt read
  Fd 0 reads from the keybboard using input_getc()
*/
int read(int fd, void *buffer, unsigned size)
{
  if(buffer ==NULL) exit(-1);

  //handle keyboard stdin
  if(fd ==0){
    uint8_t *buf = (uint8_t *)buffer;
    for(unsigned i =0 ; i <size ;i++){
      buf[i] = input_getc();
    }
    return size;
  }
  lock_acquire(&fs_lock);
  struct open_file *of = findfile_by_fd(fd);
  if(of==NULL){
    lock_release(&fs_lock);
    return -1;
  }

  int bytes_read=file_read(of->file,buffer,size);
  lock_release(&fs_lock);
  return bytes_read;
}
/*
  writes (size) bytes from buffer to open file fd
  returns number of bytes actually written
*/
int write(int fd, const void *buffer, unsigned size)
{
  if(buffer ==NULL) exit(-1);

  //handle the stdout
  if(fd==1){
    putbuf(buffer,size);
    return size;
  }

  lock_acquire(&fs_lock);
  struct open_file *of = findfile_by_fd(fd);
  if(of ==NULL){
    lock_release(&fs_lock);
    return -1;
  }

  int bytes_written = file_write(of->file,buffer,size);
  lock_release(&fs_lock);
  return bytes_written;
}
/*
  changes the next byte to be read or written in open file fd
*/
void seek(int fd, unsigned position)
{
  lock_acquire(&fs_lock);
  struct open_file *of = findfile_by_fd(fd);
  if(of !=NULL){
    file_seek(of->file,position);
  }
  lock_release(&fs_lock);
}
/*
  returns the position of the next byte to be read or written in open file fd
*/
unsigned tell(int fd)
{
  lock_acquire(&fs_lock);
  struct open_file *of = findfile_by_fd(fd);
  unsigned pos = 0;
  if(of !=NULL){
    pos = file_tell(of->file);
  }
  lock_release(&fs_lock);
  return pos;
}
/*
  closes file descriptor fd
*/
void close(int fd)
{
  lock_acquire(&fs_lock);
  struct open_file *of = findfile_by_fd(fd);
  if(of !=NULL){
    file_close(of->file);
    list_remove(&of->elem);
    free(of);
  }
  lock_release(&fs_lock);
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
  if (ptr == NULL || !is_user_vaddr(ptr) || pagedir_get_page(thread_current()->pagedir,ptr)==NULL)
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
  f->esp = (int *)f->esp + 1;
  exit(status);
}
void sys_exec_wrapper(struct intr_frame *f)
{
  char *cmd_line = get_char_ptr((char ***)&f->esp);
  validate_void_ptr((void *)cmd_line);
  f->esp = (int *)f->esp + 1;
  f->eax = exec(cmd_line);
}
void sys_wait_wrapper(struct intr_frame *f)
{
  pid_t pid = get_int((int **)&f->esp);
  f->esp = (int *)f->esp + 1;
  f->eax = wait(pid);
}

// file dependent
void sys_create_wrapper(struct intr_frame *f)
{
  char *file = get_char_ptr((char ***)&f->esp);
  f->esp = (int *)f->esp + 1;

  unsigned int initial_size = (unsigned int)get_int((int **)&f->esp);
  f->esp = (int *)f->esp + 1;

  validate_void_ptr((void *)file);
  f->eax = create(file, initial_size);
}
void sys_remove_wrapper(struct intr_frame *f)
{
  char *file = get_char_ptr((char***)&f->esp);
  f->esp = (int*)f->esp + 1;
  validate_void_ptr((void *)file);
  // Don't modify f->esp in wrappers!
  f->eax = remove(file);
} //gom3a
void sys_open_wrapper(struct intr_frame *f)
{
  char *file = get_char_ptr((char ***)&f->esp);
  validate_void_ptr((void *)file);

  f->esp = (int *)f->esp + 1;
  f->eax = open(file);
}
void sys_filesize_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp = (int *)f->esp + 1;
  f->eax = filesize(fd);
}
void sys_read_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp = (int *)f->esp + 1;

  void *buffer = get_void_ptr((void ***)&f->esp);
  f->esp = (int *)f->esp + 1;

  unsigned int size = (unsigned int)get_int((int **)&f->esp);
  f->esp = (int *)f->esp + 1;

  validate_buffer(buffer,size);
  f->eax = read(fd, buffer, size);
}
void sys_write_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp = (int *)f->esp + 1;

  void *buffer = get_void_ptr((void ***)&f->esp);
  f->esp = (int *)f->esp + 1;

  unsigned int size = (unsigned int)get_int((int **)&f->esp);
  f->esp = (int *)f->esp + 1;

  validate_buffer(buffer,size);
  f->eax = write(fd, buffer, size);
}
void sys_seek_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp = (int *)f->esp + 1;
  unsigned int position = (unsigned int)get_int((int **)&f->esp);
  f->esp = (int *)f->esp + 1;
  seek(fd, position);
}
void sys_tell_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp = (int *)f->esp + 1;
  f->eax = tell(fd);
}
void sys_close_wrapper(struct intr_frame *f)
{
  int fd = get_int((int **)&f->esp);
  f->esp = (int *)f->esp + 1;
  close(fd);
}
void validate_buffer(void* buffer,unsigned size){
  unsigned i;
  char* ptr= (char*) buffer;
  for(i= 0;i<size;i++){
    validate_void_ptr((void*)(ptr+i));
  }
}