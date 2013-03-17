#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <stdio.h>
#include <stdint.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"

const int MAX_SIZE = 256;
const int CONSOLEWRITE = 1;

static void syscall_handler (struct intr_frame *);
static bool check_uptr(const void* uptr);
static intptr_t next_value(intptr_t** sp);
static char* next_charptr(intptr_t** sp);
static void* next_ptr(intptr_t** sp);

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
	return NULL;
}

void
syscall_init (void) 
{
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
	intptr_t* kpaddr_sp = (intptr_t*) frame->esp;
	int syscall_num = -1;
	if(check_uptr(kpaddr_sp))
		syscall_num = next_value(&kpaddr_sp);

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
				thread_exit();
			}
			break;
		case SYS_EXEC:               
			{
				//pid_t exec (const char *file);
				printf("Unimplemented Call\n");
				thread_exit();
			}
			break;
		case SYS_WAIT:              
			{
				//int wait (pid_t);
				printf("Unimplemented Call\n");
				thread_exit();
			}
			break;
		case SYS_CREATE:           
			{
				//bool create (const char *file, unsigned initial_size);
				printf("Unimplemented Call\n");
				thread_exit();
			}
			break;
		case SYS_REMOVE:         
			{
				//bool remove (const char *file);
				printf("Unimplemented Call\n");
				thread_exit();
			}
			break;
		case SYS_OPEN:          
			{
				//int open (const char *file);
				printf("Unimplemented Call\n");
				thread_exit();
			}
			break;
		case SYS_FILESIZE:     
			{
				//int filesize (int fd);
				printf("Unimplemented Call\n");
				thread_exit();
			}
			break;
		case SYS_READ:        
			{
				//int read (int fd, void *buffer, unsigned length);
				printf("Unimplemented Call\n");
				thread_exit();
			}
			break;
		case SYS_WRITE:      
			{
				//int write (int fd, const void *buffer, unsigned length);
				intptr_t fd = -1;
				if(check_uptr(kpaddr_sp))
					fd = next_value(&kpaddr_sp);

				char* buffer = next_charptr(&kpaddr_sp);

				intptr_t length = -1;
				if(check_uptr(kpaddr_sp))
					length = next_value(&kpaddr_sp);

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
			}
			break;
		case SYS_SEEK:
			{
				//void seek (int fd, unsigned position);
				printf("Unimplemented Call\n");
				thread_exit();
			}
			break;
		case SYS_TELL:
			{
				//unsigned tell (int fd);
				printf("Unimplemented Call\n");
				thread_exit();
			}
			break;
		case SYS_CLOSE:    
			{
				//void close (int fd);
				printf("Unimplemented Call\n");
				thread_exit();
			}
			break;
		default:
			{
				printf("Unrecognized System Call\n");
				thread_exit();
			}
		break;
	}
}

static intptr_t
next_value(intptr_t** sp)
{
	intptr_t* ptr = *sp;
	intptr_t value = *ptr;
	++ptr;
	*sp = ptr;
	return value;
}

static char*
next_charptr(intptr_t** sp)
{
	char* charptr = (char*) next_value(sp);
	if(check_uptr(charptr))
		return charptr;
	else
		return NULL;
}

static void*
next_ptr(intptr_t** sp)
{
	void* voidptr = (void*) next_value(sp);
	if(check_uptr(voidptr))
		return voidptr;
	else
		return NULL;
}
