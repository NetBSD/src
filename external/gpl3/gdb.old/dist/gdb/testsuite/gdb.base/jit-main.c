/* This test program is part of GDB, the GNU debugger.

   Copyright 2011-2016 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Simulate loading of JIT code.  */

#include <elf.h>
#include <link.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/* ElfW is coming from linux. On other platforms it does not exist.
   Let us define it here. */
#ifndef ElfW
# if (defined  (_LP64) || defined (__LP64__)) 
#   define WORDSIZE 64
# else
#   define WORDSIZE 32
# endif /* _LP64 || __LP64__  */
#define ElfW(type)      _ElfW (Elf, WORDSIZE, type)
#define _ElfW(e,w,t)    _ElfW_1 (e, w, _##t)
#define _ElfW_1(e,w,t)  e##w##t
#endif /* !ElfW  */

typedef enum
{
  JIT_NOACTION = 0,
  JIT_REGISTER_FN,
  JIT_UNREGISTER_FN
} jit_actions_t;

struct jit_code_entry
{
  struct jit_code_entry *next_entry;
  struct jit_code_entry *prev_entry;
  const char *symfile_addr;
  uint64_t symfile_size;
};

struct jit_descriptor
{
  uint32_t version;
  /* This type should be jit_actions_t, but we use uint32_t
     to be explicit about the bitwidth.  */
  uint32_t action_flag;
  struct jit_code_entry *relevant_entry;
  struct jit_code_entry *first_entry;
};

/* GDB puts a breakpoint in this function.  */
void __attribute__((noinline)) __jit_debug_register_code () { }

/* Make sure to specify the version statically, because the
   debugger may check the version before we can set it.  */
struct jit_descriptor __jit_debug_descriptor = { 1, 0, 0, 0 };

static void
usage (const char *const argv0)
{
  fprintf (stderr, "Usage: %s library [count]\n", argv0);
  exit (1);
}

/* Update .p_vaddr and .sh_addr as if the code was JITted to ADDR.  */

static void
update_locations (const void *const addr, int idx)
{
  const ElfW (Ehdr) *const ehdr = (ElfW (Ehdr) *)addr;
  ElfW (Shdr) *const shdr = (ElfW (Shdr) *)((char *)addr + ehdr->e_shoff);
  ElfW (Phdr) *const phdr = (ElfW (Phdr) *)((char *)addr + ehdr->e_phoff);
  int i;

  for (i = 0; i < ehdr->e_phnum; ++i)
    if (phdr[i].p_type == PT_LOAD)
      phdr[i].p_vaddr += (ElfW (Addr))addr;

  for (i = 0; i < ehdr->e_shnum; ++i)
    {
      if (shdr[i].sh_type == SHT_STRTAB)
        {
          /* Note: we update both .strtab and .dynstr.  The latter would
             not be correct if this were a regular shared library (.hash
             would be wrong), but this is a simulation -- the library is
             never exposed to the dynamic loader, so it all ends up ok.  */
          char *const strtab = (char *)((ElfW (Addr))addr + shdr[i].sh_offset);
          char *const strtab_end = strtab + shdr[i].sh_size;
          char *p;

          for (p = strtab; p < strtab_end; p += strlen (p) + 1)
            if (strcmp (p, "jit_function_XXXX") == 0)
              sprintf (p, "jit_function_%04d", idx);
        }

      if (shdr[i].sh_flags & SHF_ALLOC)
        shdr[i].sh_addr += (ElfW (Addr))addr;
    }
}

/* Defined by the .exp file if testing attach.  */
#ifndef ATTACH
#define ATTACH 0
#endif

#ifndef MAIN
#define MAIN main
#endif

/* Used to spin waiting for GDB.  */
volatile int wait_for_gdb = ATTACH;
#define WAIT_FOR_GDB while (wait_for_gdb)

/* The current process's PID.  GDB retrieves this.  */
int mypid;

int
MAIN (int argc, char *argv[])
{
  /* These variables are here so they can easily be set from jit.exp.  */
  const char *libname = NULL;
  int count = 0, i, fd;
  struct stat st;

  alarm (300);

  mypid = getpid ();

  count = count;  /* gdb break here 0  */

  if (argc < 2)
    {
      usage (argv[0]);
      exit (1);
    }

  if (libname == NULL)
    /* Only set if not already set from GDB.  */
    libname = argv[1];

  if (argc > 2 && count == 0)
    /* Only set if not already set from GDB.  */
    count = atoi (argv[2]);

  printf ("%s:%d: libname = %s, count = %d\n", __FILE__, __LINE__,
	  libname, count);

  if ((fd = open (libname, O_RDONLY)) == -1)
    {
      fprintf (stderr, "open (\"%s\", O_RDONLY): %s\n", libname,
	       strerror (errno));
      exit (1);
    }

  if (fstat (fd, &st) != 0)
    {
      fprintf (stderr, "fstat (\"%d\"): %s\n", fd, strerror (errno));
      exit (1);
    }

  for (i = 0; i < count; ++i)
    {
      const void *const addr = mmap (0, st.st_size, PROT_READ|PROT_WRITE,
				     MAP_PRIVATE, fd, 0);
      struct jit_code_entry *const entry = calloc (1, sizeof (*entry));

      if (addr == MAP_FAILED)
	{
	  fprintf (stderr, "mmap: %s\n", strerror (errno));
	  exit (1);
	}

      update_locations (addr, i);

      /* Link entry at the end of the list.  */
      entry->symfile_addr = (const char *)addr;
      entry->symfile_size = st.st_size;
      entry->prev_entry = __jit_debug_descriptor.relevant_entry;
      __jit_debug_descriptor.relevant_entry = entry;

      if (entry->prev_entry != NULL)
	entry->prev_entry->next_entry = entry;
      else
	__jit_debug_descriptor.first_entry = entry;

      /* Notify GDB.  */
      __jit_debug_descriptor.action_flag = JIT_REGISTER_FN;
      __jit_debug_register_code ();
    }

  WAIT_FOR_GDB; i = 0;  /* gdb break here 1 */

  /* Now unregister them all in reverse order.  */
  while (__jit_debug_descriptor.relevant_entry != NULL)
    {
      struct jit_code_entry *const entry =
	__jit_debug_descriptor.relevant_entry;
      struct jit_code_entry *const prev_entry = entry->prev_entry;

      if (prev_entry != NULL)
	{
	  prev_entry->next_entry = NULL;
	  entry->prev_entry = NULL;
	}
      else
	__jit_debug_descriptor.first_entry = NULL;

      /* Notify GDB.  */
      __jit_debug_descriptor.action_flag = JIT_UNREGISTER_FN;
      __jit_debug_register_code ();

      __jit_debug_descriptor.relevant_entry = prev_entry;
      free (entry);
    }

  WAIT_FOR_GDB; return 0;  /* gdb break here 2  */
}
