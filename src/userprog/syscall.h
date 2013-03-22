#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"
#include "threads/synch.h"

extern unsigned waitproc;
extern struct semaphore pwait_sema;

void syscall_init (void);

typedef void exit_action_func (struct exitstatus * es, void *aux);
void exit_foreach(exit_action_func * es, void * aux);

#endif /* userprog/syscall.h */
