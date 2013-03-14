#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);

/* Get Kernel Physical Address from
   User Virtual Address; otherwise
   return null pointer*/
static uint8_t *
convert_uvaddr_kpaddr (const uint8_t *uaddr)
{
  	if(uaddr != null)
	{
  		if(is_user_vaddr((void*) uaddr))
		{
         void* kvaddr = pagedir_get_page(active_pd(), uaddr);
		   if(kvadd != null)
		   {
			   return (uint8_t *) vtop(kvaddr);
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
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}
