#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"

static struct condition pwait_cond;
static struct lock pwait_lock;
void syscall_init (void);

#endif /* userprog/syscall.h */
