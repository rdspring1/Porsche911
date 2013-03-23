#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "lib/string.h"

// Extern
struct list exit_list;
struct list waitproc_list;

const unsigned MAX_SIZE = 256;
const unsigned CONSOLEWRITE = 1;
const unsigned CONSOLEREAD = 0;

static void syscall_handler (struct intr_frame* frame);
static void exitcmd(int status);

// User Memory Check
static bool check_uptr(const void* uptr);
static bool check_buffer(const char* uptr, unsigned length);
static uintptr_t next_value(uintptr_t** sp);
static char* next_charptr(uintptr_t** sp);
static void* next_ptr(uintptr_t** sp);

// Semaphores
static struct semaphore exec_sema;
static struct semaphore filecreate_sema;
static struct semaphore fileremove_sema;

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
	list_init (&waitproc_list);
	list_init (&exit_list);

	// Initialize Private Locks
	sema_init(&exec_sema, 1);
	sema_init(&filecreate_sema, 1);
	sema_init(&fileremove_sema, 1);

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
		exitcmd(-1);

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
				uintptr_t status = -1;
				if(check_uptr(kpaddr_sp))
					status = next_value(&kpaddr_sp);
				exitcmd(status);
			}
			break;
		case SYS_EXEC:  //pid_t exec (const char *file);
			{
				const char* file = next_charptr(&kpaddr_sp);
				unsigned len = strlen(file);
				if(!check_buffer(file, len) && file == NULL)
					exitcmd(-1);
				else
					sysexec(frame, file);
			}
			break;
		case SYS_WAIT:  //int wait (pid_t);
			{
				uintptr_t childid = -1;
				if(check_uptr(kpaddr_sp))
					childid = next_value(&kpaddr_sp);
				else
					exitcmd(childid);
			
				int retval = process_wait((tid_t) childid);
				frame->eax = retval;
			}
			break;
		case SYS_CREATE:	//bool create (const char *file, unsigned initial_size);
			{
				const char* file =  next_charptr(&kpaddr_sp);
				unsigned len = strlen(file);
				if(file == NULL || !check_buffer(file, len))
					exitcmd(-1);

				uintptr_t size = 0;
				if(check_uptr(kpaddr_sp))
					size = next_value(&kpaddr_sp);
				else
					exitcmd(-1);

				syscreate(frame, file, size);
			}
			break;
		case SYS_REMOVE:	//bool remove (const char *file);
			{
				const char* file =  next_charptr(&kpaddr_sp);
				unsigned len = strlen(file);
				if(!check_buffer(file, len) && file == NULL)
					exitcmd(-1);

				sysremove(frame, file);
			}
			break;
		case SYS_OPEN:          
			{
				//int open (const char *file);
				printf("Unimplemented Call\n");
				exitcmd(-1);
			}
			break;
		case SYS_FILESIZE:     
			{
				//int filesize (int fd);
				printf("Unimplemented Call\n");
				exitcmd(-1);
			}
			break;
		case SYS_READ:        
			{
				//int read (int fd, void *buffer, unsigned length);
				printf("Unimplemented Call\n");
				exitcmd(-1);
			}
			break;
		case SYS_WRITE:      
			{
				//int write (int fd, const void *buffer, unsigned length);
				uintptr_t fd = 0;
				if(check_uptr(kpaddr_sp))
					fd = next_value(&kpaddr_sp);
				else
					exitcmd(-1);

				char* buffer = next_charptr(&kpaddr_sp);
				unsigned len = strlen(buffer);
				if(check_buffer(buffer, len) && buffer == NULL)
					exitcmd(-1);

				uintptr_t length = 0;
				if(check_uptr(kpaddr_sp))
					length = next_value(&kpaddr_sp);
				else
					exitcmd(-1);

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
				exitcmd(-1);
			}
			break;
		case SYS_TELL:
			{
				//unsigned tell (int fd);
				printf("Unimplemented Call\n");
				exitcmd(-1);
			}
			break;
		case SYS_CLOSE:    
			{
				//void close (int fd);
				printf("Unimplemented Call\n");
				exitcmd(-1);
			}
			break;
		default:
			{
				printf("Unrecognized System Call\n");
				exitcmd(-1);
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
exitcmd(int status)
{
		// Print Process Termination Message
		// File Name	
		char* name = thread_current()->name;
		char* token, *save_ptr;
		token = strtok_r(name, " ", &save_ptr);
		putbuf (token, strlen(token));

		char* str1 = ": exit(";
		putbuf (str1, strlen(str1));

		// ExitStatus
		char strstatus[32];
		snprintf(strstatus, 32, "%d", status);
		putbuf (strstatus, strlen(strstatus));

		char* str2 = ")\n";
		putbuf (str2, strlen(str2));

		// Save exit status
		struct exitstatus * es = (struct exitstatus *) malloc(sizeof(struct exitstatus));
		if(es != NULL)
		{
			es->avail = true;
			es->status = status;
			es->childid = thread_current()->tid;
			list_push_back(&exit_list, &es->elem);

			struct list_elem * e;
			for (e = list_begin (&waitproc_list); e != list_end (&waitproc_list); e = list_next (e))
			{
				struct waitproc * item = list_entry (e, struct waitproc, elem);
				sema_up(&item->sema);	
			}
		}
		thread_exit();
}

static void
sysexec(struct intr_frame* frame, const char* file)
{
	while(!sema_try_down(&exec_sema));

	tid_t newpid = process_execute(file);
	frame->eax = newpid;
	if(newpid != TID_ERROR)
		addChildProc(newpid);
	
	sema_up(&exec_sema);
}

static void
syscreate(struct intr_frame* frame, const char* file, unsigned size)
{
	while(!sema_try_down(&filecreate_sema));

	bool result = filesys_create(file, size);
	frame->eax = result;

	sema_up(&filecreate_sema);
}

static void 
sysremove(struct intr_frame* frame, const char* file)
{
	while(!sema_try_down(&fileremove_sema));

	bool result = filesys_remove(file);
	frame->eax = result;

	sema_up(&filecreate_sema);
}

void exit_foreach(exit_action_func * func, void* aux)
{
	struct list_elem * e;
	for (e = list_begin (&exit_list); e != list_end (&exit_list); e = list_next (e))
	{
		struct exitstatus * es = list_entry (e, struct exitstatus, elem);
		func (es, aux);
	}
}
