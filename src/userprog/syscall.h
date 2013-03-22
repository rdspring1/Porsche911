#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"


extern unsigned waitproc;
struct semaphore pwait_sema;

void syscall_init (void);

#endif /* userprog/syscall.h */
