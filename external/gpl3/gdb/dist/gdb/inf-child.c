/* Base/prototype target for default child (native) targets.

   Copyright (C) 1988-2017 Free Software Foundation, Inc.

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

/* This file provides a common base class/target that all native
   target implementations extend, by calling inf_child_target to get a
   new prototype target and then overriding target methods as
   necessary.  */

#include "defs.h"
#include "regcache.h"
#include "memattr.h"
#include "symtab.h"
#include "target.h"
#include "inferior.h"
#include <sys/stat.h>
#include "inf-child.h"
#include "fileio.h"
#include "agent.h"
#include "gdb_wait.h"
#include "filestuff.h"

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

/* A pointer to what is returned by inf_child_target.  Used by
   inf_child_open to push the most-derived target in reaction to
   "target native".  */
static struct target_ops *inf_child_ops = NULL;

/* Helper function for child_wait and the derivatives of child_wait.
   HOSTSTATUS is the waitstatus from wait() or the equivalent; store our
   translation of that in OURSTATUS.  */
void
store_waitstatus (struct target_waitstatus *ourstatus, int hoststatus)
{
  if (WIFEXITED (hoststatus))
    {
      ourstatus->kind = TARGET_WAITKIND_EXITED;
      ourstatus->value.integer = WEXITSTATUS (hoststatus);
    }
  else if (!WIFSTOPPED (hoststatus))
    {
      ourstatus->kind = TARGET_WAITKIND_SIGNALLED;
      ourstatus->value.sig = gdb_signal_from_host (WTERMSIG (hoststatus));
    }
  else
    {
      ourstatus->kind = TARGET_WAITKIND_STOPPED;
      ourstatus->value.sig = gdb_signal_from_host (WSTOPSIG (hoststatus));
    }
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

static void
inf_child_fetch_inferior_registers (struct target_ops *ops,
				    struct regcache *regcache, int regnum)
{
  if (regnum == -1)
    {
      for (regnum = 0;
	   regnum < gdbarch_num_regs (get_regcache_arch (regcache));
	   regnum++)
	regcache_raw_supply (regcache, regnum, NULL);
    }
  else
    regcache_raw_supply (regcache, regnum, NULL);
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers (including the floating point registers).  */

static void
inf_child_store_inferior_registers (struct target_ops *ops,
				    struct regcache *regcache, int regnum)
{
}

static void
inf_child_post_attach (struct target_ops *self, int pid)
{
  /* This target doesn't require a meaningful "post attach" operation
     by a debugger.  */
}

/* Get ready to modify the registers array.  On machines which store
   individual registers, this doesn't need to do anything.  On
   machines which store all the registers in one fell swoop, this
   makes sure that registers contains all the registers from the
   program being debugged.  */

static void
inf_child_prepare_to_store (struct target_ops *self,
			    struct regcache *regcache)
{
}

/* True if the user did "target native".  In that case, we won't
   unpush the child target automatically when the last inferior is
   gone.  */
static int inf_child_explicitly_opened;

/* See inf-child.h.  */

void
inf_child_open_target (struct target_ops *target, const char *arg,
		       int from_tty)
{
  target_preopen (from_tty);
  push_target (target);
  inf_child_explicitly_opened = 1;
  if (from_tty)
    printf_filtered ("Done.  Use the \"run\" command to start a process.\n");
}

static void
inf_child_open (const char *arg, int from_tty)
{
  inf_child_open_target (inf_child_ops, arg, from_tty);
}

/* Implement the to_disconnect target_ops method.  */

static void
inf_child_disconnect (struct target_ops *target, const char *args, int from_tty)
{
  if (args != NULL)
    error (_("Argument given to \"disconnect\"."));

  /* This offers to detach/kill current inferiors, and then pops all
     targets.  */
  target_preopen (from_tty);
}

/* Implement the to_close target_ops method.  */

static void
inf_child_close (struct target_ops *target)
{
  /* In case we were forcibly closed.  */
  inf_child_explicitly_opened = 0;
}

void
inf_child_mourn_inferior (struct target_ops *ops)
{
  generic_mourn_inferior ();
  inf_child_maybe_unpush_target (ops);
}

/* See inf-child.h.  */

void
inf_child_maybe_unpush_target (struct target_ops *ops)
{
  if (!inf_child_explicitly_opened && !have_inferiors ())
    unpush_target (ops);
}

static void
inf_child_post_startup_inferior (struct target_ops *self, ptid_t ptid)
{
  /* This target doesn't require a meaningful "post startup inferior"
     operation by a debugger.  */
}

static int
inf_child_follow_fork (struct target_ops *ops, int follow_child,
		       int detach_fork)
{
  /* This target doesn't support following fork or vfork events.  */
  return 0;
}

static int
inf_child_can_run (struct target_ops *self)
{
  return 1;
}

static char *
inf_child_pid_to_exec_file (struct target_ops *self, int pid)
{
  /* This target doesn't support translation of a process ID to the
     filename of the executable file.  */
  return NULL;
}

/* Implementation of to_fileio_open.  */

static int
inf_child_fileio_open (struct target_ops *self,
		       struct inferior *inf, const char *filename,
		       int flags, int mode, int warn_if_slow,
		       int *target_errno)
{
  int nat_flags;
  mode_t nat_mode;
  int fd;

  if (fileio_to_host_openflags (flags, &nat_flags) == -1
      || fileio_to_host_mode (mode, &nat_mode) == -1)
    {
      *target_errno = FILEIO_EINVAL;
      return -1;
    }

  fd = gdb_open_cloexec (filename, nat_flags, nat_mode);
  if (fd == -1)
    *target_errno = host_to_fileio_error (errno);

  return fd;
}

/* Implementation of to_fileio_pwrite.  */

static int
inf_child_fileio_pwrite (struct target_ops *self,
			 int fd, const gdb_byte *write_buf, int len,
			 ULONGEST offset, int *target_errno)
{
  int ret;

#ifdef HAVE_PWRITE
  ret = pwrite (fd, write_buf, len, (long) offset);
#else
  ret = -1;
#endif
  /* If we have no pwrite or it failed for this file, use lseek/write.  */
  if (ret == -1)
    {
      ret = lseek (fd, (long) offset, SEEK_SET);
      if (ret != -1)
	ret = write (fd, write_buf, len);
    }

  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of to_fileio_pread.  */

static int
inf_child_fileio_pread (struct target_ops *self,
			int fd, gdb_byte *read_buf, int len,
			ULONGEST offset, int *target_errno)
{
  int ret;

#ifdef HAVE_PREAD
  ret = pread (fd, read_buf, len, (long) offset);
#else
  ret = -1;
#endif
  /* If we have no pread or it failed for this file, use lseek/read.  */
  if (ret == -1)
    {
      ret = lseek (fd, (long) offset, SEEK_SET);
      if (ret != -1)
	ret = read (fd, read_buf, len);
    }

  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of to_fileio_fstat.  */

static int
inf_child_fileio_fstat (struct target_ops *self, int fd,
			struct stat *sb, int *target_errno)
{
  int ret;

  ret = fstat (fd, sb);
  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of to_fileio_close.  */

static int
inf_child_fileio_close (struct target_ops *self, int fd, int *target_errno)
{
  int ret;

  ret = close (fd);
  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of to_fileio_unlink.  */

static int
inf_child_fileio_unlink (struct target_ops *self,
			 struct inferior *inf, const char *filename,
			 int *target_errno)
{
  int ret;

  ret = unlink (filename);
  if (ret == -1)
    *target_errno = host_to_fileio_error (errno);

  return ret;
}

/* Implementation of to_fileio_readlink.  */

static char *
inf_child_fileio_readlink (struct target_ops *self,
			   struct inferior *inf, const char *filename,
			   int *target_errno)
{
  /* We support readlink only on systems that also provide a compile-time
     maximum path length (PATH_MAX), at least for now.  */
#if defined (PATH_MAX)
  char buf[PATH_MAX];
  int len;
  char *ret;

  len = readlink (filename, buf, sizeof buf);
  if (len < 0)
    {
      *target_errno = host_to_fileio_error (errno);
      return NULL;
    }

  ret = (char *) xmalloc (len + 1);
  memcpy (ret, buf, len);
  ret[len] = '\0';
  return ret;
#else
  *target_errno = FILEIO_ENOSYS;
  return NULL;
#endif
}

static int
inf_child_use_agent (struct target_ops *self, int use)
{
  if (agent_loaded_p ())
    {
      use_agent = use;
      return 1;
    }
  else
    return 0;
}

static int
inf_child_can_use_agent (struct target_ops *self)
{
  return agent_loaded_p ();
}

/* Default implementation of the to_can_async_p and
   to_supports_non_stop methods.  */

static int
return_zero (struct target_ops *ignore)
{
  return 0;
}

struct target_ops *
inf_child_target (void)
{
  struct target_ops *t = XCNEW (struct target_ops);

  t->to_shortname = "native";
  t->to_longname = "Native process";
  t->to_doc = "Native process (started by the \"run\" command).";
  t->to_open = inf_child_open;
  t->to_close = inf_child_close;
  t->to_disconnect = inf_child_disconnect;
  t->to_post_attach = inf_child_post_attach;
  t->to_fetch_registers = inf_child_fetch_inferior_registers;
  t->to_store_registers = inf_child_store_inferior_registers;
  t->to_prepare_to_store = inf_child_prepare_to_store;
  t->to_insert_breakpoint = memory_insert_breakpoint;
  t->to_remove_breakpoint = memory_remove_breakpoint;
  t->to_terminal_init = child_terminal_init;
  t->to_terminal_inferior = child_terminal_inferior;
  t->to_terminal_ours_for_output = child_terminal_ours_for_output;
  t->to_terminal_ours = child_terminal_ours;
  t->to_terminal_info = child_terminal_info;
  t->to_post_startup_inferior = inf_child_post_startup_inferior;
  t->to_follow_fork = inf_child_follow_fork;
  t->to_can_run = inf_child_can_run;
  /* We must default these because they must be implemented by any
     target that can run.  */
  t->to_can_async_p = return_zero;
  t->to_supports_non_stop = return_zero;
  t->to_pid_to_exec_file = inf_child_pid_to_exec_file;
  t->to_stratum = process_stratum;
  t->to_has_all_memory = default_child_has_all_memory;
  t->to_has_memory = default_child_has_memory;
  t->to_has_stack = default_child_has_stack;
  t->to_has_registers = default_child_has_registers;
  t->to_has_execution = default_child_has_execution;
  t->to_fileio_open = inf_child_fileio_open;
  t->to_fileio_pwrite = inf_child_fileio_pwrite;
  t->to_fileio_pread = inf_child_fileio_pread;
  t->to_fileio_fstat = inf_child_fileio_fstat;
  t->to_fileio_close = inf_child_fileio_close;
  t->to_fileio_unlink = inf_child_fileio_unlink;
  t->to_fileio_readlink = inf_child_fileio_readlink;
  t->to_magic = OPS_MAGIC;
  t->to_use_agent = inf_child_use_agent;
  t->to_can_use_agent = inf_child_can_use_agent;

  /* Store a pointer so we can push the most-derived target from
     inf_child_open.  */
  inf_child_ops = t;

  return t;
}
