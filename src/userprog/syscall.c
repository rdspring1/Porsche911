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

const unsigned MAX_SIZE = 256;
const unsigned CONSOLEWRITE = 1;
const unsigned CONSOLEREAD = 0;

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
static void sysexec(struct intr_frame* frame, const char* file);
static void syscreate(struct intr_frame* frame, const char* file, unsigned size);
static void sysremove(struct intr_frame* frame, const char* file);

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
		case SYS_OPEN:          
			{
				//int open (const char *file);
				printf("Unimplemented Call\n");
				exitcmd();
			}
			break;
		case SYS_FILESIZE:     
			{
				//int filesize (int fd);
				printf("Unimplemented Call\n");
				exitcmd();
			}
			break;
		case SYS_READ:        
			{
				//int read (int fd, void *buffer, unsigned length);
				printf("Unimplemented Call\n");
				exitcmd();
			}
			break;
		case SYS_WRITE:      
			{
				//int write (int fd, const void *buffer, unsigned length);
				uintptr_t fd = 0;
				if(check_uptr(kpaddr_sp))
					fd = next_value(&kpaddr_sp);
				else
					exitcmd();

				char* buffer = next_charptr(&kpaddr_sp);
				unsigned len = strlen(buffer);
				if(check_buffer(buffer, len) && buffer == NULL)
					exitcmd();

				uintptr_t length = 0;
				if(check_uptr(kpaddr_sp))
					length = next_value(&kpaddr_sp);
				else
					exitcmd();

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
				exitcmd();
			}
			break;
		case SYS_TELL:
			{
				//unsigned tell (int fd);
				printf("Unimplemented Call\n");
				exitcmd();
			}
			break;
		case SYS_CLOSE:    
			{
				//void close (int fd);
				printf("Unimplemented Call\n");
				exitcmd();
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
	thread_exit();
}

static void
sysexec(struct intr_frame* frame, const char* file)
{
	lock_acquire(&exec_lock);
	tid_t newpid = process_execute(file);
	frame->eax = newpid;
	lock_release(&exec_lock);
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
sysremove(struct intr_frame* frame, const char* file)
{
	lock_acquire(&fileremove_lock);
	bool result = filesys_remove(file);
	frame->eax = result;
	lock_release(&fileremove_lock);
}
