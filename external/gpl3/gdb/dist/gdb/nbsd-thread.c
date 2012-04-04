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
#include <pthread_dbg.h>

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

struct td_proc_callbacks_t nbsd_thread_callbacks;

static ptid_t find_active_thread (void);
static void nbsd_find_new_threads (struct target_ops *);

#define GET_PID(ptid)		ptid_get_pid (ptid)
#define GET_LWP(ptid)		ptid_get_lwp (ptid)

#define IS_LWP(ptid)		(GET_LWP (ptid) != 0)

#define BUILD_LWP(lwp, ptid)	ptid_build (GET_PID(ptid), (lwp) - 1, 0)

static td_proc_t *main_ta;

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

static char *
td_err_string (int errcode)
{
  static struct string_map
    td_err_table[] =
  {
    {TD_ERR_OK, "generic \"call succeeded\""},
    {TD_ERR_ERR, "generic error."},
    {TD_ERR_NOSYM, "symbol not found"},
    {TD_ERR_NOOBJ, "no object can be found to satisfy query"},
    {TD_ERR_BADTHREAD, "thread can not answer request"},
    {TD_ERR_INUSE, "debugging interface already in use for this process"},
    {TD_ERR_NOLIB, "process is not using libpthread"},
    {TD_ERR_NOMEM, "out of memory"},
    {TD_ERR_IO, "process callback error"},
    {TD_ERR_INVAL, "invalid argument"},
  };
  const int td_err_size = sizeof td_err_table / sizeof (struct string_map);
  int i;
  static char buf[90];

  for (i = 0; i < td_err_size; i++)
    if (td_err_table[i].num == errcode)
      return td_err_table[i].str;

  sprintf (buf, "Unknown thread_db error code: %d", errcode);

  return buf;
}

static void
nbsd_thread_activate (void)
{
  nbsd_thread_active = 1;
  main_ptid = inferior_ptid;
  cached_thread = minus_one_ptid;
  nbsd_find_new_threads (NULL);
  inferior_ptid = find_active_thread ();
}

static void
nbsd_thread_deactivate (void)
{
  td_close (main_ta);

  inferior_ptid = main_ptid;
  main_ptid = minus_one_ptid;
  cached_thread = main_ptid;
  nbsd_thread_active = 0;
  nbsd_thread_present = 0;
  init_thread_list ();
}

static void
nbsd_thread_attach (struct target_ops *ops, char *args, int from_tty)
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
nbsd_thread_post_attach (int pid)
{
#if 0
  struct target_ops *beneath = find_target_beneath (ops);
  beneath->to_post_attach (pid);
#endif

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
nbsd_thread_detach (struct target_ops *ops, char *args, int from_tty)
{
  struct target_ops *beneath = find_target_beneath (ops);
  nbsd_thread_deactivate ();
  unpush_target (ops);
  /* Ordinarily, gdb caches solib information, but this means that it
     won't call the new_obfile hook on a reattach. Clear the symbol file
     cache so that attach -> detach -> attach works. */
  clear_solib();
  symbol_file_clear(0);
  beneath->to_detach (beneath, args, from_tty);
}

static int nsusp;
static int nsuspalloc;
static td_thread_t **susp;

static int
thread_resume_suspend_cb (td_thread_t *th, void *arg)
{
  int val;
  ptid_t *pt = arg;
  td_thread_info_t ti;

  if (td_thr_info (th, &ti) != 0)
      return -1;

  return 0;
}

static void
nbsd_thread_resume (struct target_ops *ops, ptid_t ptid, int step,
    enum target_signal signo)
{
  struct target_ops *beneath = find_target_beneath (ops);

  /* If a particular thread is specified, then gdb wants to resume or
     step just that thread. If it isn't on a processor, then it needs
     to be put on one, and nothing else can be on the runnable list.
     XXX If GDB asks us to step a LWP rather than a thread, there
     isn't anything we can do but pass it down to the ptrace call;
     given the flexibility of the LWP-to-thread mapping, this might or
     might not accomplish what the user wanted. */
  beneath->to_resume (beneath, ptid, step, signo);

  cached_thread = minus_one_ptid;
}


static void
nbsd_thread_unsuspend(void)
{
  int i, val;

  for (i = 0; i < nsusp; i++)
    {
      val = td_thr_resume(susp[i]);
      if (val != 0)
	error ("nbsd_thread_unsuspend: td_thr_resume(%p): %s", susp[i],
	       td_err_string (val));
    }
  nsusp = 0;
}
  
static ptid_t
find_active_thread (void)
{
  int val;
  td_thread_t *thread;
  td_thread_info_t ti;
  struct ptrace_lwpinfo pl;

  if (!ptid_equal (cached_thread, minus_one_ptid))
    return cached_thread;

  if (target_has_execution)
    {
      pl.pl_lwpid = 0;
      val = ptrace (PT_LWPINFO, GET_PID(inferior_ptid), (void *)&pl, sizeof(pl));
      while ((val != -1) && (pl.pl_lwpid != 0) &&
	     (pl.pl_event != PL_EVENT_SIGNAL))
	val = ptrace (PT_LWPINFO, GET_PID(inferior_ptid), (void *)&pl, sizeof(pl));
      if (pl.pl_lwpid == 0)
	 pl.pl_lwpid = 1;
    }
  else
    {
      return inferior_ptid;
    }

  cached_thread = BUILD_LWP (pl.pl_lwpid, main_ptid);
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

  nbsd_thread_unsuspend();

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
  td_thread_t *thread;
  gregset_t gregs;
#ifdef HAVE_FPREGS
  fpregset_t fpregs;
#endif
  int val;
  struct cleanup *old_chain;

  old_chain = save_inferior_ptid ();

  if (!target_has_execution)
    {
      inferior_ptid = pid_to_ptid ((GET_LWP (inferior_ptid) << 16) | 
				    GET_PID (inferior_ptid));
    }
    beneath->to_fetch_registers (beneath, cache, regno);
  
  do_cleanups (old_chain);
}

static void
nbsd_thread_store_registers (struct target_ops *ops, struct regcache *cache,
    int regno)
{
  struct target_ops *beneath = find_target_beneath (ops);
  td_thread_t *thread;
  gregset_t gregs;
#ifdef HAVE_FPREGS
  fpregset_t fpregs;
#endif
  int val;

  beneath->to_store_registers (beneath, cache, regno);
}



static LONGEST
nbsd_thread_xfer_partial (struct target_ops *ops, enum target_object object,
			  const char *annex, gdb_byte *readbuf,
			  const gdb_byte *writebuf,  ULONGEST offset,
			  LONGEST len)
{
  struct target_ops *beneath = find_target_beneath (ops);
  LONGEST val;

  val = beneath->to_xfer_partial (beneath, object, annex, readbuf, writebuf,
				  offset, len);
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

static void
nbsd_core_files_info (struct target_ops *ignore)
{
  struct target_ops *beneath = find_target_beneath (ignore);
  beneath->to_files_info (beneath);
}

/* Convert a ptid to printable form. */

char *
nbsd_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  static char buf[100];
  td_thread_t *th;
  char name[32];

  if ((GET_LWP(ptid) == 0) && 
      (nbsd_thread_active == 0))
    sprintf (buf, "process %d", GET_PID (ptid));
  else
    sprintf (buf, "LWP %ld", GET_LWP (ptid));

  return buf;
}


static void
nbsd_add_to_thread_list (bfd *abfd, asection *asect, PTR reg_sect_arg)
{
  int regval;
  td_thread_t *dummy;

  if (strncmp (bfd_section_name (abfd, asect), ".reg/", 5) != 0)
    return;

  regval = atoi (bfd_section_name (abfd, asect) + 5);

  add_thread (BUILD_LWP(regval, main_ptid));
}

/* This routine is called whenever a new symbol table is read in, or when all
   symbol tables are removed.  libthread_db can only be initialized when it
   finds the right variables in libthread.so.  Since it's a shared library,
   those variables don't show up until the library gets mapped and the symbol
   table is read in.  */

/* This new_objfile event is now managed by a chained function pointer.
 * It is the callee's responsability to call the next client on the chain.
 */

void
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


  /* Now, initialize the thread debugging library.  This needs to be
     done after the shared libraries are located because it needs
     information from the user's thread library.  */
  val = td_open (&nbsd_thread_callbacks, &main_arg, &main_ta);
  if (val == TD_ERR_NOLIB)
    goto quit;
  else if (val != 0)
    {
      warning ("nbsd_thread_new_objfile: td_open: %s", td_err_string (val));
      goto quit;
    }

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
  td_thread_t *th;
  td_thread_info_t ti;
  int val;

  if (nbsd_thread_active)
    {
      if (IS_LWP (ptid))
	{
	  struct ptrace_lwpinfo pl;
	  pl.pl_lwpid = GET_LWP (ptid);
	  val = ptrace (PT_LWPINFO, GET_PID (ptid), (void *)&pl, sizeof(pl));
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

static int
nbsd_core_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  struct target_ops *beneath = find_target_beneath (ops);
  return beneath->to_thread_alive (beneath, ptid);
}


static void
nbsd_find_new_threads (struct target_ops *ops)
{
  int retval;
  ptid_t ptid;
#ifdef notyet
  td_thread_t *thread;
#endif

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
      retval = ptrace (PT_LWPINFO, GET_PID(inferior_ptid), (void *)&pl, sizeof(pl));
      while ((retval != -1) && pl.pl_lwpid != 0)
	{
	  ptid = BUILD_LWP (pl.pl_lwpid, main_ptid);
#ifdef notyet
	  td_map_lwp2thr (main_ta, pl.pl_lwpid, &thread);
#endif
	  if (!in_thread_list (ptid))
	    add_thread (ptid);
	  retval = ptrace (PT_LWPINFO, GET_PID(inferior_ptid), (void *)&pl, sizeof(pl));
	}
    }
}


/* Fork an inferior process, and start debugging it.  */

static void
nbsd_thread_create_inferior (struct target_ops *ops, char *exec_file,
			     char *allargs, char **env, int from_tty)
{
  struct target_ops *beneath = find_target_beneath (ops);
  nbsd_thread_core = 0;

  if (nbsd_thread_present && !nbsd_thread_active)
    push_target(&nbsd_thread_ops);

  beneath->to_create_inferior (beneath, exec_file, allargs, env, from_tty);

  if (nbsd_thread_present && !nbsd_thread_active)
    nbsd_thread_activate();
}


static int
waiter_cb (td_thread_t *th, void *s)
{
  int ret;
  td_thread_info_t ti;

  if ((ret = td_thr_info (th, &ti)) == 0)
    {
      wrap_here (NULL);
      printf_filtered (" %d", ti.thread_id);
    }

  return 0;
}

/* Worker bee for thread state command.  This is a callback function that
   gets called once for each user thread (ie. not kernel thread) in the
   inferior.  Print anything interesting that we can think of.  */

static int
info_cb (td_thread_t *th, void *s)
{
  int ret;
  td_thread_info_t ti, ti2;
#ifdef notyet
  td_sync_t *ts;
  td_sync_info_t tsi;
#endif
  char name[32];

  if ((ret = td_thr_info (th, &ti)) == 0)
    {
      if (ti.thread_type != TD_TYPE_USER)
	return 0;
      td_thr_getname(th, name, 32);
      printf_filtered ("%p: thread %4d ", ti.thread_addr, ti.thread_id);
      if (name[0] != '\0')
	      printf_filtered("(%s) ", name);

      switch (ti.thread_state)
	{
	default:
	case TD_STATE_UNKNOWN:
	  printf_filtered ("<unknown state> ");
	  break;
	case TD_STATE_RUNNING:
	  printf_filtered ("running  ");
	  break;
	case TD_STATE_RUNNABLE:
	  printf_filtered ("active   ");
	  break;
	case TD_STATE_BLOCKED:
	  printf_filtered ("in kernel");
	  break;
	case TD_STATE_SLEEPING:
	  printf_filtered ("sleeping ");
	  break;
	case TD_STATE_ZOMBIE:
	  printf_filtered ("zombie   ");
	  break;
	}

      if (ti.thread_state == TD_STATE_SLEEPING)
	{
#ifdef notyet
	  td_thr_sleepinfo (th, &ts);
	  td_sync_info (ts, &tsi);
	  if (tsi.sync_type == TD_SYNC_JOIN)
	    {
	      td_thr_info (tsi.sync_data.join.thread, &ti2);
	      printf ("joining thread %d ", ti2.thread_id);
	    }
	  else
	    {
	      printf_filtered ("on %s at %p ",
			       syncnames[tsi.sync_type],
			       (void *)tsi.sync_addr);
	    }
#endif
	}

#ifdef notyet
      if (ti.thread_hasjoiners)
	{
	  printf_filtered ("(being joined by");
	  td_thr_join_iter (th, waiter_cb, NULL);
	  printf_filtered (")");
	}
#endif
      printf_filtered ("\n");
    }
  else
    warning ("info nbsd-thread: failed to get info for thread.");

  return 0;
}


/* List some state about each user thread in the inferior.  */

static void
nbsd_thread_examine_all_cmd (char *args, int from_tty)
{
  int val;

  if (nbsd_thread_active)
    {
      val = td_thr_iter (main_ta, info_cb, args);
      if (val != 0)
	error ("nbsd_thread_examine_all_cmd: td_thr_iter: %s",
	       td_err_string (val));
    }
  else
    printf_filtered ("Thread debugging not active.\n");
}

static void
nbsd_thread_examine_cmd (char *exp, int from_tty)
{
  CORE_ADDR addr;
  td_thread_t *th;
  int ret;

  if (!nbsd_thread_active)
    {
      warning ("Thread debugging not active.\n");
      return;
    }

  if (exp != NULL && *exp != '\000')
    {
      addr = parse_and_eval_address (exp);
      if (from_tty)
	*exp = 0;
    }
  else
    return;

  if ((ret = td_map_pth2thr (main_ta, (pthread_t)(uintptr_t)addr, &th)) != 0)
    error ("nbsd_thread_examine_command: td_map_pth2thr: %s",
	   td_err_string (ret));
  
  info_cb (th, NULL);
}


static void
nbsd_thread_sync_cmd (char *exp, int from_tty)
{
  CORE_ADDR addr;
#ifdef notyet
  td_sync_t *ts;
  td_sync_info_t tsi;
#endif
  td_thread_info_t ti;
  int ret;

  if (!nbsd_thread_active)
    {
      warning ("Thread debugging not active.\n");
      return;
    }

  if (exp != NULL && *exp != '\000')
    {
      addr = parse_and_eval_address (exp);
      if (from_tty)
	*exp = 0;
    }
  else
    return;

#ifdef notyet
  if ((ret = td_map_addr2sync (main_ta, (caddr_t)addr, &ts)) != 0)
    error ("nbsd_thread_sync_cmd: td_map_addr2sync: %s", td_err_string (ret));

  if ((ret = td_sync_info (ts, &tsi)) != 0)
    error ("nbsd_thread_sync_cmd: td_sync_info: %s", td_err_string (ret));

  printf_filtered ("%p: %s", (void *)addr, syncnames[tsi.sync_type]);

  if (tsi.sync_type == TD_SYNC_MUTEX)
    {
      if (!tsi.sync_data.mutex.locked)
	printf_filtered (" unlocked");
      else
	{
	  td_thr_info (tsi.sync_data.mutex.owner, &ti);
	  printf_filtered (" locked by thread %d", ti.thread_id);
	}
    }
  else if (tsi.sync_type == TD_SYNC_SPIN)
    {
      if (!tsi.sync_data.spin.locked)
	printf_filtered (" unlocked");
      else
	printf_filtered (" locked (waiters not tracked)");
    }
  else if (tsi.sync_type == TD_SYNC_JOIN)
    {
      td_thr_info (tsi.sync_data.join.thread, &ti);
      printf_filtered (" %d", ti.thread_id);
    }
  else if (tsi.sync_type == TD_SYNC_RWLOCK)
    {
      if (!tsi.sync_data.rwlock.locked)
	printf_filtered (" unlocked");
      else
	{
	  printf_filtered (" locked");
	  if (tsi.sync_data.rwlock.readlocks > 0)
	    printf_filtered (" by %d reader%s", 
			     tsi.sync_data.rwlock.readlocks,
			     (tsi.sync_data.rwlock.readlocks > 1) ? "s" : "");
	  else
	    {
	      td_thr_info (tsi.sync_data.rwlock.writeowner, &ti);
	      printf_filtered (" by writer %d", ti.thread_id);
	    }
	}
    }
  else
    printf_filtered("Unknown sync object type %d", tsi.sync_type);

  if (tsi.sync_haswaiters)
    {
      printf_filtered (" waiters:");
      if ((ret = td_sync_waiters_iter (ts, waiter_cb, NULL)) != 0)
	error ("nbsd_thread_sync_cmd: td_sync_info: %s",
	       td_err_string (ret));
    }

  printf_filtered ("\n");
#endif
}

int
tsd_cb (pthread_key_t key, void (*destructor)(void *), void *ignore)
{
  struct minimal_symbol *ms;
  char *name;

  printf_filtered ("Key %3d   ", key);

  /* XXX What does GDB use to print a function? */
  ms = lookup_minimal_symbol_by_pc ((CORE_ADDR)(uintptr_t)destructor);

  if (!ms)
    name = "???";
  else
    name = SYMBOL_PRINT_NAME (ms);

  printf_filtered ("Destructor %p <%s>\n", destructor, name);

  return 0;
}

static void
nbsd_thread_tsd_cmd (char *exp, int from_tty)
{
  td_tsd_iter (main_ta, tsd_cb, NULL);
}

/*
 * Process operation callbacks
 */
static int
nbsd_thread_proc_read (void *arg, caddr_t addr, void *buf, size_t size)
{
  int val;

  val = target_read_memory ((CORE_ADDR)(uintptr_t)addr, buf, size);

  if (val == 0)
    return 0;
  else
    return TD_ERR_IO;
}


static int
nbsd_thread_proc_write (void *arg, caddr_t addr, void *buf, size_t size)
{
  int val;

  val = target_write_memory ((CORE_ADDR)(uintptr_t)addr, buf, size);

  if (val == 0)
    return 0;
  else
    return TD_ERR_IO;
}

static int
nbsd_thread_proc_lookup (void *arg, const char *sym, caddr_t *addr)
{
  struct minimal_symbol *ms;

  ms = lookup_minimal_symbol (sym, NULL, NULL);

  if (!ms)
    return TD_ERR_NOSYM;

  *addr = (caddr_t)(uintptr_t)SYMBOL_VALUE_ADDRESS (ms);

  return 0;

}

static int
nbsd_thread_proc_regsize (void *arg, int regset, size_t *size)
{
  switch (regset)
    {
    case 0:
      *size = sizeof (gregset_t);
      break;
    case 1:
#ifdef HAVE_FPREGS
      *size = sizeof (fpregset_t);
#else
      *size = 0;
#endif
      break;
    default:
      return TD_ERR_INVAL;
    }

  return 0;
}

static int
nbsd_thread_proc_getregs (void *arg, int regset, int lwp, void *buf)
{
  struct nbsd_thread_proc_arg *a = arg; 
  struct regcache *cache = a->cache;
  struct target_ops *ops = a->ops;
  struct cleanup *old_chain;
  struct target_ops *beneath = find_target_beneath (ops);
  int ret;

  old_chain = save_inferior_ptid ();

  if (target_has_execution)
    {
      /* Fetching registers from a live process requires that
	 inferior_ptid is a LWP value rather than a thread value. */
      inferior_ptid = BUILD_LWP (lwp, main_ptid);
      beneath->to_fetch_registers (beneath, cache, -1);
    }
  else
    {
      /* Fetching registers from a core process requires that
	 the PID value of inferior_ptid have the funky value that
	 the kernel drops rather than the real PID. Gross. */
      inferior_ptid = pid_to_ptid ((lwp << 16) | GET_PID (main_ptid));
      beneath->to_fetch_registers (ops, cache, -1);
    }

  ret = 0;
  switch (regset)
    {
    case 0:
      fill_gregset (cache, (gregset_t *)buf, -1);
      break;
    case 1:
#ifdef HAVE_FPREGS
      fill_fpregset (cache, (fpregset_t *)buf, -1);
#endif
      break;
    default: /* XXX need to handle other reg sets: SSE, AltiVec, etc. */
      ret = TD_ERR_INVAL;
    }

  do_cleanups (old_chain);

  return ret;
}

static int
nbsd_thread_proc_setregs (void *arg, int regset, int lwp, void *buf)
{
  struct nbsd_thread_proc_arg *a = arg; 
  struct regcache *cache = a->cache;
  struct target_ops *ops = a->ops;
  struct cleanup *old_chain;
  struct target_ops *beneath = find_target_beneath (ops);
  int ret;

  ret = 0;
  old_chain = save_inferior_ptid ();

  switch (regset)
    {
    case 0:
      supply_gregset(cache, (gregset_t *)buf);
      break;
    case 1:
#ifdef HAVE_FPREGS
      supply_fpregset(cache, (fpregset_t *)buf);
#endif
      break;
    default: /* XXX need to handle other reg sets: SSE, AltiVec, etc. */
      ret = TD_ERR_INVAL;
    }

  /* Storing registers requires that inferior_ptid is a LWP value
     rather than a thread value. */
  inferior_ptid = BUILD_LWP (lwp, main_ptid);
  beneath->to_store_registers (beneath, cache, -1);
  do_cleanups (old_chain);

  return ret;
}

static void
init_nbsd_proc_callbacks (void)
{
  nbsd_thread_callbacks.proc_read = nbsd_thread_proc_read;
  nbsd_thread_callbacks.proc_write = nbsd_thread_proc_write;
  nbsd_thread_callbacks.proc_lookup = nbsd_thread_proc_lookup;
  nbsd_thread_callbacks.proc_regsize = nbsd_thread_proc_regsize;
  nbsd_thread_callbacks.proc_getregs = nbsd_thread_proc_getregs;
  nbsd_thread_callbacks.proc_setregs = nbsd_thread_proc_setregs;
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
  nbsd_thread_ops.to_terminal_init = terminal_init_inferior;
  nbsd_thread_ops.to_terminal_inferior = terminal_inferior;
  nbsd_thread_ops.to_terminal_ours_for_output = terminal_ours_for_output;
  nbsd_thread_ops.to_terminal_ours = terminal_ours;
  nbsd_thread_ops.to_terminal_info = child_terminal_info;
  nbsd_thread_ops.to_create_inferior = nbsd_thread_create_inferior;
  nbsd_thread_ops.to_mourn_inferior = nbsd_thread_mourn_inferior;
  nbsd_thread_ops.to_thread_alive = nbsd_thread_alive;
  nbsd_thread_ops.to_pid_to_str = nbsd_pid_to_str;
  nbsd_thread_ops.to_find_new_threads = nbsd_find_new_threads;
  nbsd_thread_ops.to_stratum = thread_stratum;
  nbsd_thread_ops.to_has_thread_control = tc_none;
  nbsd_thread_ops.to_magic = OPS_MAGIC;
}

void
_initialize_nbsd_thread (void)
{
  static struct cmd_list_element *thread_examine_list = NULL;

  init_nbsd_thread_ops ();
  init_nbsd_proc_callbacks ();

  add_target (&nbsd_thread_ops);
  add_cmd ("tsd", class_run, nbsd_thread_tsd_cmd,
	   "Show the thread-specific data keys and destructors for the process.\n",
	   &thread_cmd_list);

  add_cmd ("sync", class_run, nbsd_thread_sync_cmd,
	   "Show the synchronization object at the given address.\n",
	   &thread_cmd_list);

  add_prefix_cmd ("examine", class_run, nbsd_thread_examine_cmd,
		  "Show the thread at the given address.\n",
		  &thread_examine_list, "examine ", 1, &thread_cmd_list);

  add_cmd ("all", class_run, nbsd_thread_examine_all_cmd,
	   "Show all threads.",
	   &thread_examine_list);

  /* Hook into new_objfile notification.  */ 
  observer_attach_new_objfile (nbsd_thread_new_objfile);
}
