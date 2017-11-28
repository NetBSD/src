/* Thread debugging back-end code for NetBSD, for GDB.
   Copyright 2002
   Wasabi Systems, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */


#include "defs.h"

#include <sys/types.h>
#include <sys/ptrace.h>

#include <pthread.h>

#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "solib.h"
#include "gdbthread.h"
#include "bfd.h"
#include "elf-bfd.h"
#include "target.h"
#include "inferior.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "observer.h"

#include <machine/reg.h>
#ifdef __sh3__
struct fpreg { };
#elif defined(__vax__)
#else
#define HAVE_FPREGS
#endif

#ifndef HAVE_GREGSET_T
typedef struct reg gregset_t;
#endif

#ifndef HAVE_FPREGSET_T
typedef struct fpreg fpregset_t;
#endif

#include "gregset.h"

/* nbsd_thread_present indicates that new_objfile has spotted
   libpthread and that post_attach() or create_inferior() should fire
   up thread debugging if it isn't already active. */
static int nbsd_thread_present = 0;

/* nbsd_thread_active indicates that thread debugging is up and running, and
   in particular that main_ta and main_ptid are valid. */
static int nbsd_thread_active = 0;

/* nbsd_thread_core indicates that we're working on a corefile, not a
   live process. */ 
static int nbsd_thread_core = 0;

static ptid_t main_ptid;		/* Real process ID */

static ptid_t cached_thread;

struct target_ops nbsd_thread_ops;

static ptid_t find_active_thread (void);
static void nbsd_update_thread_list (struct target_ops *);


struct nbsd_thread_proc_arg {
  struct target_ops *ops;
  struct regcache *cache;
} main_arg;

static const char *syncnames[] = {
  "unknown", "mutex", "cond var", "spinlock", "thread"
};

struct string_map
  {
    int num;
    char *str;
  };

static void
nbsd_thread_activate (void)
{
  nbsd_thread_active = 1;
  main_ptid = inferior_ptid;
  cached_thread = minus_one_ptid;
  thread_change_ptid(inferior_ptid,
      ptid_build (ptid_get_pid (inferior_ptid), 1, 0));
  nbsd_update_thread_list (NULL);
  inferior_ptid = find_active_thread ();
}

static void
nbsd_thread_deactivate (void)
{
  inferior_ptid = main_ptid;
  main_ptid = minus_one_ptid;
  cached_thread = main_ptid;
  nbsd_thread_active = 0;
  nbsd_thread_present = 0;
  init_thread_list ();
}

static void
nbsd_thread_attach (struct target_ops *ops, const char *args, int from_tty)
{
  struct target_ops *beneath = find_target_beneath (ops);
  nbsd_thread_core = 0;

  if (nbsd_thread_present && !nbsd_thread_active)
    push_target(&nbsd_thread_ops);

  beneath->to_attach (beneath, args, from_tty);

  /* seems like a good place to activate, but isn't. Let it happen in
     nbsd_thread_post_attach(), after a wait has occurred. */
}

static void
nbsd_thread_post_attach (struct target_ops *ops, int pid)
{
  if (nbsd_thread_present && !nbsd_thread_active)
    nbsd_thread_activate ();
}


/* Take a program previously attached to and detaches it.
   The program resumes execution and will no longer stop
   on signals, etc.  We'd better not have left any breakpoints
   in the program or it'll die when it hits one.  For this
   to work, it may be necessary for the process to have been
   previously attached.  It *might* work if the program was
   started via the normal ptrace (PTRACE_TRACEME).  */

static void
nbsd_thread_detach (struct target_ops *ops, const char *args, int from_tty)
{
  struct target_ops *beneath = find_target_beneath (ops);
  unpush_target (ops);
  /* Ordinarily, gdb caches solib information, but this means that it
     won't call the new_obfile hook on a reattach. Clear the symbol file
     cache so that attach -> detach -> attach works. */
  clear_solib();
  symbol_file_clear(0);
  beneath->to_detach (beneath, args, from_tty);
  nbsd_thread_deactivate ();
}

static int nsusp;
static int nsuspalloc;

static void
nbsd_thread_resume (struct target_ops *ops, ptid_t ptid, int step,
    enum gdb_signal signo)
{
  struct target_ops *beneath = find_target_beneath (ops);

  /* If a particular thread is specified, then gdb wants to resume or
     step just that thread. If it isn't on a processor, then it needs
     to be put on one, and nothing else can be on the runnable list.
     XXX If GDB asks us to step a LWP rather than a thread, there
     isn't anything we can do but pass it down to the ptrace call;
     given the flexibility of the LWP-to-thread mapping, this might or
     might not accomplish what the user wanted. */
  if (ptid_get_pid(ptid) == -1)
    ptid = inferior_ptid;
  beneath->to_resume (beneath, ptid, step, signo);

  cached_thread = minus_one_ptid;
}


static ptid_t
find_active_thread (void)
{
  int val;
  struct ptrace_lwpinfo pl;

  if (!ptid_equal (cached_thread, minus_one_ptid))
    return cached_thread;

  if (target_has_execution)
    {
      pl.pl_lwpid = 0;
      val = ptrace (PT_LWPINFO, ptid_get_pid(inferior_ptid), (void *)&pl, sizeof(pl));
      while ((val != -1) && (pl.pl_lwpid != 0) &&
	     (pl.pl_event != PL_EVENT_SIGNAL)) {
	val = ptrace (PT_LWPINFO, ptid_get_pid(inferior_ptid), (void *)&pl, sizeof(pl));
    }
      if (pl.pl_lwpid == 0)
	/* found no "active" thread, stay with current */
	pl.pl_lwpid = ptid_get_lwp (inferior_ptid);
    }
  else
    {
      return inferior_ptid;
    }

  cached_thread = ptid_build (ptid_get_pid (main_ptid), pl.pl_lwpid, 0);
  return cached_thread;
}


/* Wait for any threads to stop.  We may have to convert PID from a thread id
   to a LWP id, and vice versa on the way out.  */

static ptid_t
nbsd_thread_wait (struct target_ops *ops, ptid_t ptid,
  struct target_waitstatus *ourstatus, int options)
{
  ptid_t rtnval;
  struct target_ops *beneath = find_target_beneath (ops);

  rtnval = beneath->to_wait (beneath, ptid, ourstatus, options);

  if (nbsd_thread_active && (ourstatus->kind != TARGET_WAITKIND_EXITED))
    {
      rtnval = find_active_thread ();
      if (ptid_equal (rtnval, minus_one_ptid))
	error ("No active thread!\n");
      if (!in_thread_list (rtnval))
	add_thread (rtnval);
    }

  return rtnval;
}

static void
nbsd_thread_fetch_registers (struct target_ops *ops, struct regcache *cache,
    int regno)
{
  struct target_ops *beneath = find_target_beneath (ops);
  gregset_t gregs;
#ifdef HAVE_FPREGS
  fpregset_t fpregs;
#endif
  int val;
  struct cleanup *old_chain;

  old_chain = save_inferior_ptid ();

  if (!target_has_execution)
    {
      inferior_ptid = pid_to_ptid ((ptid_get_lwp (inferior_ptid) << 16) | 
				    ptid_get_pid (inferior_ptid));
    }
    beneath->to_fetch_registers (beneath, cache, regno);
  
  do_cleanups (old_chain);
}

static void
nbsd_thread_store_registers (struct target_ops *ops, struct regcache *cache,
    int regno)
{
  struct target_ops *beneath = find_target_beneath (ops);
  gregset_t gregs;
#ifdef HAVE_FPREGS
  fpregset_t fpregs;
#endif
  int val;

  beneath->to_store_registers (beneath, cache, regno);
}



static enum target_xfer_status
nbsd_thread_xfer_partial (struct target_ops *ops, enum target_object object,
			  const char *annex, gdb_byte *readbuf,
			  const gdb_byte *writebuf,  ULONGEST offset,
			  ULONGEST len, ULONGEST *xfered_len)
{
  struct target_ops *beneath = find_target_beneath (ops);
  enum target_xfer_status val;

  val = beneath->to_xfer_partial (beneath, object, annex, readbuf, writebuf,
				  offset, len, xfered_len);
  return val;
}

/* Clean up after the inferior dies.  */

static void
nbsd_thread_mourn_inferior (struct target_ops *ops)
{
  struct target_ops *beneath = find_target_beneath (ops);

  if (nbsd_thread_active)
    nbsd_thread_deactivate ();

  unpush_target (ops);
  beneath->to_mourn_inferior (beneath);
}


static void
nbsd_thread_files_info (struct target_ops *ignore)
{
  struct target_ops *beneath = find_target_beneath (ignore);
  beneath->to_files_info (beneath);
}

#if 0
static void
nbsd_core_files_info (struct target_ops *ignore)
{
  struct target_ops *beneath = find_target_beneath (ignore);
  beneath->to_files_info (beneath);
}
#endif

/* Convert a ptid to printable form. */

static const char *
nbsd_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  static char buf[100];
  char name[32];

  if ((ptid_get_lwp(ptid) == 0) && 
      (nbsd_thread_active == 0))
    sprintf (buf, "process %d", ptid_get_pid (ptid));
  else
    sprintf (buf, "LWP %ld", ptid_get_lwp (ptid));

  return buf;
}

/* This routine is called whenever a new symbol table is read in, or when all
   symbol tables are removed.  libthread_db can only be initialized when it
   finds the right variables in libthread.so.  Since it's a shared library,
   those variables don't show up until the library gets mapped and the symbol
   table is read in.  */

/* This new_objfile event is now managed by a chained function pointer.
 * It is the callee's responsability to call the next client on the chain.
 */

static void
nbsd_thread_new_objfile (struct objfile *objfile)
{
  int val;

  if (!objfile)
    {
      nbsd_thread_active = 0;
      goto quit;
    }

  /* Don't do anything if we've already fired up the debugging library */
  if (nbsd_thread_active)
    goto quit;


  nbsd_thread_present = 1;

  if ((nbsd_thread_core == 0) && 
      !ptid_equal (inferior_ptid, null_ptid))
    {
      push_target (&nbsd_thread_ops);
      nbsd_thread_activate();
    }

quit:
  return;
}

static int
nbsd_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  struct target_ops *beneath = find_target_beneath (ops);
  int val;

  if (nbsd_thread_active)
    {
      if (ptid_lwp_p (ptid))
	{
	  struct ptrace_lwpinfo pl;
	  pl.pl_lwpid = ptid_get_lwp (ptid);
	  val = ptrace (PT_LWPINFO, ptid_get_pid (ptid), (void *)&pl, sizeof(pl));
	  if (val == -1)
	    val = 0;
	  else
	    val = 1;
	}
      else
	val = beneath->to_thread_alive (beneath, ptid);
    }
  else
    val = beneath->to_thread_alive (beneath, ptid);

  return val;
}

#ifdef notdef
static int
nbsd_core_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  struct target_ops *beneath = find_target_beneath (ops);
  return beneath->to_thread_alive (beneath, ptid);
}
#endif


static void
nbsd_update_thread_list (struct target_ops *ops)
{
  int retval;
  ptid_t ptid;

  if (nbsd_thread_active == 0)
	  return;

  if (ptid_equal (inferior_ptid, minus_one_ptid))
    {
      printf_filtered ("No process.\n");
      return;
    }

  if (target_has_execution)
    {
      struct ptrace_lwpinfo pl;
      pl.pl_lwpid = 0;
      retval = ptrace (PT_LWPINFO, ptid_get_pid(inferior_ptid), (void *)&pl, sizeof(pl));
      while ((retval != -1) && pl.pl_lwpid != 0)
	{
	  ptid = ptid_build (ptid_get_pid (main_ptid), pl.pl_lwpid, 0);
	  if (!in_thread_list (ptid))
	    add_thread (ptid);
	  retval = ptrace (PT_LWPINFO, ptid_get_pid(inferior_ptid), (void *)&pl, sizeof(pl));
	}
    }
}


/* Fork an inferior process, and start debugging it.  */

static void
nbsd_thread_create_inferior (struct target_ops *ops, 
			     const char *exec_file, const std::string &allargs,
			     char **env, int from_tty)
{
  struct target_ops *beneath = find_target_beneath (ops);
  nbsd_thread_core = 0;

  if (nbsd_thread_present && !nbsd_thread_active)
    push_target(&nbsd_thread_ops);

  beneath->to_create_inferior (beneath, exec_file, allargs, env, from_tty);

  if (nbsd_thread_present && !nbsd_thread_active)
    nbsd_thread_activate();
}

static void
init_nbsd_thread_ops (void)
{
  nbsd_thread_ops.to_shortname = "netbsd-threads";
  nbsd_thread_ops.to_longname = "NetBSD pthread.";
  nbsd_thread_ops.to_doc = "NetBSD pthread support.";
  nbsd_thread_ops.to_attach = nbsd_thread_attach;
  nbsd_thread_ops.to_post_attach = nbsd_thread_post_attach;
  nbsd_thread_ops.to_detach = nbsd_thread_detach;
  nbsd_thread_ops.to_resume = nbsd_thread_resume;
  nbsd_thread_ops.to_wait = nbsd_thread_wait;
  nbsd_thread_ops.to_fetch_registers = nbsd_thread_fetch_registers;
  nbsd_thread_ops.to_store_registers = nbsd_thread_store_registers;
  nbsd_thread_ops.to_xfer_partial = nbsd_thread_xfer_partial;
  nbsd_thread_ops.to_files_info = nbsd_thread_files_info;
  nbsd_thread_ops.to_insert_breakpoint = memory_insert_breakpoint;
  nbsd_thread_ops.to_remove_breakpoint = memory_remove_breakpoint;
  nbsd_thread_ops.to_terminal_init = child_terminal_init;
  nbsd_thread_ops.to_terminal_inferior = child_terminal_inferior;
  nbsd_thread_ops.to_terminal_ours_for_output = child_terminal_ours_for_output;
  nbsd_thread_ops.to_terminal_ours = child_terminal_ours;
  nbsd_thread_ops.to_terminal_info = child_terminal_info;
  nbsd_thread_ops.to_create_inferior = nbsd_thread_create_inferior;
  nbsd_thread_ops.to_mourn_inferior = nbsd_thread_mourn_inferior;
  nbsd_thread_ops.to_thread_alive = nbsd_thread_alive;
  nbsd_thread_ops.to_pid_to_str = nbsd_pid_to_str;
  nbsd_thread_ops.to_update_thread_list = nbsd_update_thread_list;
  nbsd_thread_ops.to_stratum = thread_stratum;
  nbsd_thread_ops.to_has_thread_control = tc_none;
  nbsd_thread_ops.to_magic = OPS_MAGIC;
}

void _initialize_nbsd_thread (void);
void
_initialize_nbsd_thread (void)
{
  init_nbsd_thread_ops ();

  add_target (&nbsd_thread_ops);

  /* Hook into new_objfile notification.  */ 
  observer_attach_new_objfile (nbsd_thread_new_objfile);
}
