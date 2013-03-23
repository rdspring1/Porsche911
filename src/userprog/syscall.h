#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"
#include "threads/synch.h"

extern struct list waitproc_list;

struct waitproc
{
	struct semaphore sema;
	tid_t id;
	struct list_elem elem; 
};

void syscall_init (void);

typedef void exit_action_func (struct exitstatus * es, void *aux);
void exit_foreach(exit_action_func * es, void * aux);

#endif /* userprog/syscall.h */
