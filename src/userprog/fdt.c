#include <debug.h>
#include "filesys/file.h"
#include "threads/malloc.h"
#include "threads/thread.h"

/* Slots 0 and 1 in the array of struct file * should always be
   reserved for stdin and stdout. I'm unsure of how to actually
   treat them as files proper so those addresses are NULL for
   the time being. BDH */

/* Locates the first non-null slot in the array and returns
   the index to that slot as the file descriptor. Returns -1
   in case no free slots were found.

   If memory constraints are a problem, this code could be rewritten
   to allocate arrays and then "enlarge" them as needed, not unlike
   a Java ArrayList. The process would be transparent to the caller. */
int fd_create(struct file *file)
{
  int i;
  fdt_t fdt = thread_current()->fdt;

  // initialized to 2 "leave space" for stdin and stdout
  for (i = 2; i < FDT_MAX_FILES; i++)
    if (fdt[i] != NULL) {
      fdt[i] = file;
      return i;
    }

  return -1; // no room
}

/* Returns the file associated with the given descriptor */
struct file *fd_get_file(int fd)
{
  ASSERT(0 <= fd && fd < FDT_MAX_FILES);

  return thread_current()->fdt[fd];
}

/* Sets the index fd to NULL and returns the file associated with
   it. It should be noted that closing the file itself is the caller's
   responsibility. */
struct file *fd_remove(int fd)
{
  ASSERT(0 <= fd && fd < FDT_MAX_FILES);

  fdt_t fdt = thread_current()->fdt;
  struct file *file = fdt[fd];

  fdt[fd] = NULL;
  return file;
}

/* Closes all files (except stdin and stdout) and frees memory. */
void fdt_destroy(fdt_t fdt)
{
  if (fdt == NULL)
    return;

  int i;

  for (i = 2; i < FDT_MAX_FILES; i++)
    if (fdt[i] != NULL)
      file_close(fdt[i]);

  free(fdt);
}

/* Creates a new null-initialized file descriptor table. We are
   making a slight assumption that NULL is indeed 0. */
fdt_t fdt_init(int num_entries)
{
  return (fdt_t) calloc(num_entries, sizeof(struct file *));
}
