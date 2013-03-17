#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <stdint.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"

const size_t MAX_SIZE = 256;
const int CONSOLEWRITE = 1;
static void syscall_handler (struct intr_frame *);

/* Get Kernel Physical Address from User Virtual Address; 
   otherwise return null pointer*/
static void *
convert_uvaddr_kpaddr (uint32_t* pagedir, const void *uaddr)
{
  	if(uaddr != null)
	{
  		if(is_user_vaddr(uaddr))
		{
         void* kvaddr = pagedir_get_page(pagedir, uaddr);
		    if(kvadd != null)
		    {
			   return vtop(kvaddr);
		    }
        }
  	} 
  	return null;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f frame) 
{
	// -------- System Call Handler Overview -------- 
	// Get system call number
	// switch statement using system call number
	// collect arguments for system call function if necessary
	// call system call function
	// set frame->eax to return value if necessary
	// ----------------------------------------------
	struct thread* userthread = pg_round_down (frame->esp);
	uint32_t** kpaddr_sp = &((uint32_t*) convert_uvaddr_kpaddr(userthread->pagedir, frame->esp));
	uint32_t syscall_num = next_int(kpaddr_sp);
	switch(syscall_num)
	{
		case SYS_HALT:                   /* Halt the operating system. */
			// Terminates Pintos
			shutdown_power_off();
		break;
		case SYS_EXIT:                   /* Terminate this process. */
			frame->error_code = next_int(kpaddr_sp);
			thread_exit();
		break;
		case SYS_EXEC:                   /* Start another process. */
			//pid_t exec (const char *file);
		break;
		case SYS_WAIT:                   /* Wait for a child process to die. */
			//int wait (pid_t);
		break;
		case SYS_CREATE:                 /* Create a file. */
			//bool create (const char *file, unsigned initial_size);
		break;
		case SYS_REMOVE:                 /* Delete a file. */
			//bool remove (const char *file);
		break;
		case SYS_OPEN:                   /* Open a file. */
			//int open (const char *file);
		break;
		case SYS_FILESIZE:               /* Obtain a file's size. */
			//int filesize (int fd);
		break;
		case SYS_READ:                   /* Read from a file. */
			//int read (int fd, void *buffer, unsigned length);
		break;
		case SYS_WRITE:                  /* Write to a file. */
			//int write (int fd, const void *buffer, unsigned length);
			uint32_t fd = next_int(kpaddr_sp);
			char *buffer = next_charptr(kpaddr_sp);
			uint32_t length = next_int(kpaddr_sp);
			if(fd == CONSOLEWRITE) // Write to Console
			{
				while(length > 0)
				{
					if(length > MAX_SIZE)
					{
						putbuf (buffer, MAX_SIZE);
						buffer += MAX_SIZE;
						length -= MAX_SIZE;
					}
					else
					{
						putbuf (buffer, length);
						length = 0;
					}
				}
			}
		break;
		case SYS_SEEK:                   /* Change position in a file. */
			//void seek (int fd, unsigned position);
		break;
		case SYS_TELL:                   /* Report current position in a file. */
			//unsigned tell (int fd);
		break;
		case SYS_CLOSE:                  /* Close a file. */
			//void close (int fd);
		break;
	}
}

static uint32_t
next_value(uint32_t** sp)
{
	uint32_t* ptr = *sp;
	uint32_t value = *ptr;
	++ptr;
	*sp = ptr;
	return value;
}

static char*
next_charptr(uint32_t** sp)
{
	return (char *) next_value(sp);
}

static void*
next_ptr(uint32_t** sp)
{
	return (void *) next_value(sp);
}
