/* Native-dependent code for FreeBSD.

   Copyright (C) 2002-2014 Free Software Foundation, Inc.

   This file is part of GDB.

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

#include "defs.h"
#include "gdbcore.h"
#include "inferior.h"
#include "regcache.h"
#include "regset.h"
#include "gdbthread.h"

#include "gdb_assert.h"
#include <string.h>
#include <sys/types.h>
#include <sys/procfs.h>
#include <sys/sysctl.h>

#include "elf-bfd.h"
#include "fbsd-nat.h"

/* Return the name of a file that can be opened to get the symbols for
   the child process identified by PID.  */

char *
fbsd_pid_to_exec_file (int pid)
{
  size_t len = PATH_MAX;
  char *buf = xcalloc (len, sizeof (char));
  char *path;

#ifdef KERN_PROC_PATHNAME
  int mib[4];

  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = pid;
  if (sysctl (mib, 4, buf, &len, NULL, 0) == 0)
    return buf;
#endif

  path = xstrprintf ("/proc/%d/file", pid);
  if (readlink (path, buf, PATH_MAX - 1) == -1)
    {
      xfree (buf);
      buf = NULL;
    }

  xfree (path);
  return buf;
}

static int
fbsd_read_mapping (FILE *mapfile, unsigned long *start, unsigned long *end,
		   char *protection)
{
  /* FreeBSD 5.1-RELEASE uses a 256-byte buffer.  */
  char buf[256];
  int resident, privateresident;
  unsigned long obj;
  int ret = EOF;

  /* As of FreeBSD 5.0-RELEASE, the layout is described in
     /usr/src/sys/fs/procfs/procfs_map.c.  Somewhere in 5.1-CURRENT a
     new column was added to the procfs map.  Therefore we can't use
     fscanf since we need to support older releases too.  */
  if (fgets (buf, sizeof buf, mapfile) != NULL)
    ret = sscanf (buf, "%lx %lx %d %d %lx %s", start, end,
		  &resident, &privateresident, &obj, protection);

  return (ret != 0 && ret != EOF);
}

/* Iterate over all the memory regions in the current inferior,
   calling FUNC for each memory region.  OBFD is passed as the last
   argument to FUNC.  */

int
fbsd_find_memory_regions (find_memory_region_ftype func, void *obfd)
{
  pid_t pid = ptid_get_pid (inferior_ptid);
  char *mapfilename;
  FILE *mapfile;
  unsigned long start, end, size;
  char protection[4];
  int read, write, exec;
  struct cleanup *cleanup;

  mapfilename = xstrprintf ("/proc/%ld/map", (long) pid);
  cleanup = make_cleanup (xfree, mapfilename);
  mapfile = fopen (mapfilename, "r");
  if (mapfile == NULL)
    error (_("Couldn't open %s."), mapfilename);
  make_cleanup_fclose (mapfile);

  if (info_verbose)
    fprintf_filtered (gdb_stdout, 
		      "Reading memory regions from %s\n", mapfilename);

  /* Now iterate until end-of-file.  */
  while (fbsd_read_mapping (mapfile, &start, &end, &protection[0]))
    {
      size = end - start;

      read = (strchr (protection, 'r') != 0);
      write = (strchr (protection, 'w') != 0);
      exec = (strchr (protection, 'x') != 0);

      if (info_verbose)
	{
	  fprintf_filtered (gdb_stdout, 
			    "Save segment, %ld bytes at %s (%c%c%c)\n",
			    size, paddress (target_gdbarch (), start),
			    read ? 'r' : '-',
			    write ? 'w' : '-',
			    exec ? 'x' : '-');
	}

      /* Invoke the callback function to create the corefile segment.
	 Pass MODIFIED as true, we do not know the real modification state.  */
      func (start, size, read, write, exec, 1, obfd);
    }

  do_cleanups (cleanup);
  return 0;
}

static int
find_signalled_thread (struct thread_info *info, void *data)
{
  if (info->suspend.stop_signal != GDB_SIGNAL_0
      && ptid_get_pid (info->ptid) == ptid_get_pid (inferior_ptid))
    return 1;

  return 0;
}

static enum gdb_signal
find_stop_signal (void)
{
  struct thread_info *info =
    iterate_over_threads (find_signalled_thread, NULL);

  if (info)
    return info->suspend.stop_signal;
  else
    return GDB_SIGNAL_0;
}

/* Create appropriate note sections for a corefile, returning them in
   allocated memory.  */

char *
fbsd_make_corefile_notes (bfd *obfd, int *note_size)
{
  const struct regcache *regcache = get_current_regcache ();
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  gregset_t gregs;
  fpregset_t fpregs;
  char *note_data = NULL;
  Elf_Internal_Ehdr *i_ehdrp;
  const struct regset *regset;
  size_t size;

  /* Put a "FreeBSD" label in the ELF header.  */
  i_ehdrp = elf_elfheader (obfd);
  i_ehdrp->e_ident[EI_OSABI] = ELFOSABI_FREEBSD;

  gdb_assert (gdbarch_regset_from_core_section_p (gdbarch));

  size = sizeof gregs;
  regset = gdbarch_regset_from_core_section (gdbarch, ".reg", size);
  gdb_assert (regset && regset->collect_regset);
  regset->collect_regset (regset, regcache, -1, &gregs, size);

  note_data = elfcore_write_prstatus (obfd, note_data, note_size,
				      ptid_get_pid (inferior_ptid),
				      find_stop_signal (), &gregs);

  size = sizeof fpregs;
  regset = gdbarch_regset_from_core_section (gdbarch, ".reg2", size);
  gdb_assert (regset && regset->collect_regset);
  regset->collect_regset (regset, regcache, -1, &fpregs, size);

  note_data = elfcore_write_prfpreg (obfd, note_data, note_size,
				     &fpregs, sizeof (fpregs));

  if (get_exec_file (0))
    {
      const char *fname = lbasename (get_exec_file (0));
      char *psargs = xstrdup (fname);

      if (get_inferior_args ())
	psargs = reconcat (psargs, psargs, " ", get_inferior_args (),
			   (char *) NULL);

      note_data = elfcore_write_prpsinfo (obfd, note_data, note_size,
					  fname, psargs);
    }

  make_cleanup (xfree, note_data);
  return note_data;
}
