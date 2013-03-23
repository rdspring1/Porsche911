#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <stdio.h>
#include <stdint.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "lib/string.h"
#include "userprog/fdt.h"
#include "filesys/file.h"
#include "devices/input.h"

#define user_return(val) frame->eax = val; return
#define MAX_SIZE 256

static void syscall_handler (struct intr_frame* frame);
static void exitcmd(void);

// User Memory Check
static bool check_uptr(const void* uptr);
static bool check_buffer(const char* uptr, unsigned length);
static uintptr_t next_value(uintptr_t** sp);
static char* next_charptr(uintptr_t** sp);
static void* next_ptr(uintptr_t** sp);

// Locks
static struct lock exec_lock;
static struct lock filecreate_lock;
static struct lock fileremove_lock;

// Syscall Functions
static void sysclose(int fd);
static void syscreate(struct intr_frame* frame, const char* file, unsigned size);
static void sysexec(struct intr_frame* frame, const char* file);
static void sysfilesize(struct intr_frame *frame, int fd);
static void sysopen(struct intr_frame *frame, const char *file);
static void sysread(struct intr_frame *frame, int fd, void *buffer, unsigned size);
static void sysremove(struct intr_frame* frame, const char* file);
static void sysseek(int fd, unsigned position);
static void systell(struct intr_frame *frame, int fd);
static void syswrite(struct intr_frame *frame, int fd, const void *buffer, unsigned size);

/* Determine whether user process pointer is valid;
   Otherwise, return false*/ 
static bool
check_uptr (const void* uptr)
{
	if(uptr != NULL)
	{
		if(is_user_vaddr(uptr))
		{
			void* kptr = pagedir_get_page(thread_current()->pagedir, uptr);
			if(kptr != NULL)
			{
				return true;
			}
		}
	} 
	return false;
}

static bool
check_buffer (const char* uptr, unsigned length)
{
	unsigned i;
	for(i = 0; i < length; ++i)
	{
		if(!check_uptr(uptr))
			return false;
		++uptr;
	}
	return true;
}

void
syscall_init (void) 
{
	// Initialize Locks
	lock_init(&exec_lock);
	lock_init(&filecreate_lock);
	lock_init(&fileremove_lock);

	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame* frame) 
{
	// -------- System Call Handler Overview -------- 
	// Get system call number
	// switch statement using system call number
	// collect arguments for system call function if necessary
	// call system call function
	// set frame->eax to return value if necessary
	// ----------------------------------------------
	uintptr_t* kpaddr_sp = (uintptr_t*) frame->esp;
	int syscall_num = -1;
	if(check_uptr(kpaddr_sp))
		syscall_num = next_value(&kpaddr_sp);
	else
		exitcmd();

	switch(syscall_num)
	  {
	  case SYS_HALT:                   
	    {
	      // Terminates Pintos
	      shutdown_power_off();
	    }
	    break;
	  case SYS_EXIT:                 
	    {
	      frame->error_code = next_value(&kpaddr_sp);
	      exitcmd();
	    }
	    break;
	  case SYS_EXEC:  //pid_t exec (const char *file);
	    {
	      const char* file = next_charptr(&kpaddr_sp);
	      unsigned len = strlen(file);
	      if(!check_buffer(file, len) && file == NULL)
		exitcmd();
	      else
		sysexec(frame, file);
	    }
	    break;
	  case SYS_WAIT:  //int wait (pid_t);
	    {
	      printf("Unimplemented Call\n");
	      exitcmd();
	    }
	    break;
	  case SYS_CREATE:	//bool create (const char *file, unsigned initial_size);
	    {
	      const char* file =  next_charptr(&kpaddr_sp);
	      unsigned len = strlen(file);
	      if(!check_buffer(file, len) && file == NULL)
		exitcmd();

	      uintptr_t size = 0;
	      if(check_uptr(kpaddr_sp))
		size = next_value(&kpaddr_sp);
	      else
		exitcmd();

	      syscreate(frame, file, size);
	    }
	    break;
	  case SYS_REMOVE:	//bool remove (const char *file);
	    {
	      const char* file =  next_charptr(&kpaddr_sp);
	      unsigned len = strlen(file);
	      if(!check_buffer(file, len) && file == NULL)
		exitcmd();

	      sysremove(frame, file);
	    }
	    break;
	  case SYS_OPEN:  // int open (const char *file);          
	    {
	      const char *file = next_charptr(&kpaddr_sp);
	      // WARN: no check on file. Experiment.
	      sysopen(frame, file);
	    }
	    break;
	  case SYS_FILESIZE: // int filesize (int fd);
	    {
	      int fd = 0;
	      if (check_uptr(kpaddr_sp))
		fd = (int) next_value(&kpaddr_sp);
	      else
		exitcmd();

	      sysfilesize(frame, fd);
	    }
	    break;
	  case SYS_READ:        
	    {
	      //int read (int fd, void *buffer, unsigned length);
	      int fd = 0;
	      if (check_uptr(kpaddr_sp))
		fd = (int) next_value(&kpaddr_sp);
	      else
		exitcmd();
	      
	      char *buffer = next_charptr(&kpaddr_sp);
	      unsigned len = strlen(buffer);
	      if (check_buffer(buffer, len) && buffer == NULL)
		exitcmd();
	      
	      unsigned length = 0;
	      if (check_uptr(kpaddr_sp))
		length = (unsigned) next_value(&kpaddr_sp);
	      else
		exitcmd();

	      sysread(frame, fd, buffer, length);
	    }
	    break;
	  case SYS_WRITE:      
	    {
	      //int write (int fd, const void *buffer, unsigned length);
	      int fd = 0;
	      if(check_uptr(kpaddr_sp))
		fd = (int) next_value(&kpaddr_sp);
	      else
		exitcmd();

	      char *buffer = next_charptr(&kpaddr_sp);
	      unsigned len = strlen(buffer);
	      if(check_buffer(buffer, len) && buffer == NULL)
		exitcmd();

	      unsigned length = 0;
	      if(check_uptr(kpaddr_sp))
		length = (unsigned) next_value(&kpaddr_sp);
	      else
		exitcmd();

	      syswrite(frame, fd, buffer, length);
	    }
	    break;
	  case SYS_SEEK: //void seek (int fd, unsigned position);
	    {
	      int fd = 0;
	      if (check_uptr(kpaddr_sp))
		fd = (int) next_value(&kpaddr_sp);
	      else
		exitcmd();

	      unsigned position = 0;
	      if (check_uptr(kpaddr_sp))
		position = (unsigned) next_value(&kpaddr_sp);
	      else
		exitcmd();

	      sysseek(fd, position);
	    }
	    break;
	  case SYS_TELL: // unsigned tell (int fd);
	    {
	      int fd = 0;
	      if (check_uptr(kpaddr_sp))
		fd = (int) next_value(&kpaddr_sp);
	      else
		exitcmd();

	      systell(frame, fd);
	    }
	    break;
	  case SYS_CLOSE: //void close (int fd);
	    {
	      int fd = 0;
	      if (check_uptr(kpaddr_sp))
		fd = (int) next_value(&kpaddr_sp);
	      else
		exitcmd();
	      
	      sysclose(fd);
	    }
	    break;
	  default:
	    {
	      printf("Unrecognized System Call\n");
	      exitcmd();
	    }
	    break;
	  }
}

static uintptr_t
next_value(uintptr_t** sp)
{
	uintptr_t* ptr = *sp;
	uintptr_t value = *ptr;
	++ptr;
	*sp = ptr;
	return value;
}

static char*
next_charptr(uintptr_t** sp)
{
	char* charptr = (char*) next_value(sp);
	if(check_uptr(charptr))
		return charptr;
	else
		return NULL;
}

static void*
next_ptr(uintptr_t** sp)
{
	void* voidptr = (void*) next_value(sp);
	if(check_uptr(voidptr))
		return voidptr;
	else
		return NULL;
}

static void
exitcmd()
{
  // Print Process Termination Message
  process_exit();
  thread_exit();
}

static void
sysclose(int fd)
{
	struct file *file = fd_remove(fd);

	if (file != NULL)
		file_close(file);
}

static void
syscreate(struct intr_frame* frame, const char* file, unsigned size)
{
	lock_acquire(&filecreate_lock);
	bool result = filesys_create(file, size);
	frame->eax = result;
	lock_release(&filecreate_lock);
}

static void
sysexec(struct intr_frame* frame, const char* file)
{
	lock_acquire(&exec_lock);
	tid_t newpid = process_execute(file);
	frame->eax = newpid;
	if(newpid != TID_ERROR)
	{
	  struct childproc child;
	  child.childid = newpid;
	  list_push_back(&thread_current()->child_list, &child.elem);
	}
	lock_release(&exec_lock);
}

static void
sysfilesize(struct intr_frame *frame, int fd)
{
  struct file *file = fd_get_file(fd);
  
  if (file == NULL) {
    user_return(-1);
  }
  else {
    user_return( file_length(file) );
  }
}

static void
sysopen(struct intr_frame *frame, const char *file)
{
  struct file *f = filesys_open(file);

  if (f == NULL) {
    user_return(-1);
  }
  else {
    user_return( fd_create(f) );
  }
}

static void
sysread(struct intr_frame *frame, int fd, void *buffer, unsigned size)
{
  // special case
  if (fd == STDIN_FILENO) {
    char *char_buffer = (char *) buffer;

    while (size-- > 0) {
      *char_buffer++ = input_getc();
    }

    user_return(size);
  }

  struct file *file = fd_get_file(fd);

  if (file == NULL) {
    user_return(-1);
  }
  else {
    user_return( file_read(file, buffer, size) );
  }
}

static void 
sysremove(struct intr_frame* frame, const char* file)
{
	lock_acquire(&fileremove_lock);
	bool result = filesys_remove(file);
	frame->eax = result;
	lock_release(&fileremove_lock);
}

static void
sysseek(int fd, unsigned position)
{
	struct file *file = fd_get_file(fd);

	if (file == NULL)
		return;
	else
		file_seek(file, position);
}

static void
systell(struct intr_frame *frame, int fd)
{
	struct file *file = fd_get_file(fd);

	if (file == NULL) {
	  user_return(-1);
	}
	else {
	  user_return( (unsigned) file_tell(file) );
	}
}

static void
syswrite(struct intr_frame *frame, int fd, const void *buffer, unsigned size)
{
  // special case
  if (fd == STDOUT_FILENO) {
    /* Old implementation. Can we get by without MAX_SIZE?
    while(length > 0) {

      if(length > MAX_SIZE) {
	putbuf (buffer, MAX_SIZE);
	buffer += MAX_SIZE;
	length -= MAX_SIZE;
      }
      else {
	putbuf (buffer, length);
	length = 0;
      }
    }
    */

    putbuf(buffer, size);
  }

  struct file *file = fd_get_file(fd);
  
  if (file == NULL) {
    user_return(-1);
  }
  else {
    user_return( file_write(file, buffer, size) );
  }
}
