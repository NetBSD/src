/* Thread debugging back-end code for NetBSD, for GDB.

   Copyright 2005 Free Software Foundation, Inc.
   Written by Nathan Williams <nathanw@wasabisystems.com>

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
#include <dlfcn.h>

#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "observer.h"
#include "solib.h"
#include "solist.h"
#include "gdbthread.h"
#include "bfd.h"
#include "elf-bfd.h"
#include "target.h"
#include "inferior.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "regset.h"
#include "regcache.h"

#include "gdb_assert.h"
#include "gdb_obstack.h"

#include <machine/reg.h>

#ifndef HAVE_GREGSET_T
typedef struct reg gregset_t;
#endif

#ifndef HAVE_FPREGSET_T
typedef struct fpreg fpregset_t;
#endif

#include "gregset.h"

static int debug_nbsd_thread;

/* nbsd_thread_present indicates that new_objfile has spotted
   libpthread and that post_attach() or create_inferior() should fire
   up thread debugging if it isn't already active. */
static int nbsd_thread_present = 0;

/* nbsd_thread_active indicates that thread debugging is up and running, and
   in particular that main_ta and main_ptid are valid. */
static int nbsd_thread_active = 0;

static ptid_t main_ptid;		/* Real process ID */

static ptid_t cached_thread;

struct target_ops nbsd_thread_ops;
struct target_ops nbsd_core_ops; 	/* XXXNH Used? */

struct td_proc_callbacks_t nbsd_thread_callbacks;
struct td_proc_callbacks_t nbsd_core_callbacks;

/* Save the target_ops for things this module can't handle. */
static struct target_ops target_beneath;

/* Saved pointer to previous owner of
   deprecated_target_new_objfile_hook.  */
static void (*target_new_objfile_chain)(struct objfile *);

static ptid_t find_active_thread PARAMS ((void));
static void nbsd_find_new_threads PARAMS ((void));

#define GET_PID(ptid)		ptid_get_pid (ptid)
#define GET_LWP(ptid)		ptid_get_lwp (ptid)
#define GET_THREAD(ptid)	ptid_get_tid (ptid)

#define IS_LWP_P(ptid)		(GET_LWP (ptid) != 0)
#define IS_THREAD_P(ptid)		(GET_THREAD (ptid) != 0)

#define BUILD_LWP(lwp, ptid)	ptid_build (GET_PID(ptid), lwp, 0)
#define BUILD_THREAD(tid, ptid)	ptid_build (GET_PID(ptid), 0, tid)

static td_proc_t *main_ta;

/* Pointers to routines from libpthread_dbg resolved by dlopen().  */
static int (*p_td_open)(struct td_proc_callbacks_t *, void *arg, td_proc_t **);
static int (*p_td_close)(td_proc_t *);
static int (*p_td_thr_iter)(td_proc_t *, int (*)(td_thread_t *, void *), void *);
static int (*p_td_thr_info)(td_thread_t *, td_thread_info_t *);
static int (*p_td_thr_getname)(td_thread_t *, char *, int);
static int (*p_td_thr_getregs)(td_thread_t *, int, void *);
static int (*p_td_thr_setregs)(td_thread_t *, int, void *);
static int (*p_td_thr_join_iter)(td_thread_t *, int (*)(td_thread_t *, void *), void *);
static int (*p_td_thr_sleepinfo)(td_thread_t *, td_sync_t **);
static int (*p_td_sync_info)(td_sync_t *, td_sync_info_t *);
static int (*p_td_sync_waiters_iter)(td_sync_t *, int (*)(td_thread_t *, void *), void *);
static int (*p_td_map_addr2sync)(td_proc_t *, caddr_t addr, td_sync_t **);
static int (*p_td_map_pth2thr)(td_proc_t *, pthread_t, td_thread_t **);
static int (*p_td_map_id2thr)(td_proc_t *, int, td_thread_t **);
static int (*p_td_map_lwp2thr)(td_proc_t *, int, td_thread_t **);
static int (*p_td_map_lwps)(td_proc_t *);
static int (*p_td_tsd_iter)(td_proc_t *, int (*)(pthread_key_t, void (*)(void *), void *), 
    void *);
static int (*p_td_thr_tsd)(td_thread_t *, pthread_key_t, void **);
static int (*p_td_thr_suspend)(td_thread_t *);
static int (*p_td_thr_resume)(td_thread_t *);

static const char * const syncnames[] = {"unknown",
			   "mutex",
			   "cond var",
			   "spinlock",
			   "joining thread",
			   "read-write lock"};

struct string_map
  {
    int num;
    char *str;
  };

static char *
td_err_string (int errcode)
{
  static const struct string_map
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
show_debug_nbsd_thread (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Debugging of NetBSD thread module is %s.\n"),
		    value);
}









static void
nbsd_thread_activate (void)
{
  nbsd_thread_active = 1;
  main_ptid = inferior_ptid;
  cached_thread = minus_one_ptid;
  nbsd_find_new_threads ();
#if 0
  /* This doesn't work on newly-started processes; let it be handled in 
     wait() */
  inferior_ptid = find_active_thread ();
#endif
}

static void
nbsd_thread_deactivate (void)
{
  p_td_close (main_ta);

  inferior_ptid = main_ptid;
  main_ptid = minus_one_ptid;
  cached_thread = main_ptid;
  nbsd_thread_active = 0;
  nbsd_thread_present = 0;
  init_thread_list ();
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

  if (p_td_thr_info (th, &ti) != 0)
      return -1;

  if ((ti.thread_id != GET_THREAD (*pt)) &&
      (ti.thread_type == TD_TYPE_USER) &&
      (ti.thread_state != TD_STATE_SUSPENDED) &&
      (ti.thread_state != TD_STATE_ZOMBIE))
    {
      val = p_td_thr_suspend(th);
      if (val != 0)
	error ("thread_resume_suspend_cb: td_thr_suspend(%p): %s", th,
	       td_err_string (val));
	
      if (nsusp == nsuspalloc)
	{
	  if (nsuspalloc == 0)
	    {
	      nsuspalloc = 32;
	      susp = malloc (nsuspalloc * sizeof(td_thread_t *));
	      if (susp == NULL)
		error ("thread_resume_suspend_cb: out of memory\n");
	    }
	  else
	    {
	      static td_thread_t **newsusp;
	      nsuspalloc *= 2;
	      newsusp = realloc (susp, nsuspalloc * sizeof(td_thread_t *));
	      if (newsusp == NULL)
		error ("thread_resume_suspend_cb: out of memory\n");
	      susp = newsusp;
	    }
	}
      susp[nsusp] = th;
      nsusp++;
    }
  
  return 0;
}

static void
nbsd_thread_resume (ptid_t ptid, int step, enum target_signal signo)
{

  /* If a particular ptid is specified, then gdb wants to resume or
     step just that thread. If it isn't on a processor, then it needs
     to be put on one, and nothing else can be on the runnable
     list. */
  if ((GET_PID (ptid) != -1) && IS_THREAD_P (ptid))
    {
      int i, val;

      val = p_td_thr_iter (main_ta, thread_resume_suspend_cb, &ptid);
      if (val != 0)
	error ("nbsd_thread_resume td_thr_iter: %s", td_err_string (val));

      target_beneath.to_resume (ptid, step, signo);

      /* can't un-suspend just yet, child may not be stopped */
    }
  else
    target_beneath.to_resume (ptid, step, signo);

  cached_thread = minus_one_ptid;
}


static void
nbsd_thread_unsuspend(void)
{
  int i, val;

  for (i = 0; i < nsusp; i++)
    {
      td_thread_info_t ti;
      p_td_thr_info(susp[i], &ti);
      val = p_td_thr_resume(susp[i]);
      if (val != 0)
	error ("nbsd_thread_resume: td_thr_suspend(%p): %s", susp[i],
	       td_err_string (val));
    }
  nsusp = 0;
}
  
static ptid_t
find_active_thread (void)
{
  int val, lwp;
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
    }

#ifdef MYCROFT_HACK
  cached_thread = BUILD_LWP (pl.pl_lwpid, main_ptid);
#else

  if (p_td_map_lwp2thr (main_ta, pl.pl_lwpid, &thread) == 0)
    if (p_td_thr_info (thread, &ti) == 0)
      return (cached_thread = BUILD_THREAD (ti.thread_id, inferior_ptid));

  cached_thread = BUILD_THREAD (ti.thread_id, main_ptid);
#endif

  return cached_thread;
}


/* FIXME: This function is only there because otherwise GDB tries to
   invoke deprecate_xfer_memory.  */

static LONGEST
nbsd_thread_xfer_partial (struct target_ops *ops, enum target_object object,
			  const char *annex, gdb_byte *readbuf,
			  const gdb_byte *writebuf, ULONGEST offset,
			  LONGEST len)
{
  gdb_assert (ops->beneath->to_xfer_partial);
  return ops->beneath->to_xfer_partial (ops->beneath, object, annex, readbuf,
					writebuf, offset, len);
}


/* Wait for any threads to stop.  We may have to convert PID from a thread id
   to a LWP id, and vice versa on the way out.  */

static ptid_t
nbsd_thread_wait (ptid_t ptid, struct target_waitstatus *ourstatus)
{
  ptid_t rtnval;

  rtnval = target_beneath.to_wait (ptid, ourstatus);

  nbsd_thread_unsuspend();

  if (nbsd_thread_active && (ourstatus->kind != TARGET_WAITKIND_EXITED))
    {
      rtnval = find_active_thread ();
      if (!ptid_equal (rtnval, main_ptid) && 
	  !in_thread_list (rtnval))
	add_thread (rtnval);
    }

  return rtnval;
}

static void
nbsd_thread_fetch_registers (int regno)
{
  td_thread_t *thread;
  gregset_t gregs;
  fpregset_t fpregs;
  const struct regset *rs, *frs;
  int val;
  struct cleanup *old_chain;

  old_chain = save_inferior_ptid ();

  if (nbsd_thread_active && IS_THREAD_P (inferior_ptid))
    {
      if ((val = p_td_map_id2thr (main_ta, GET_THREAD (inferior_ptid), &thread)) != 0)
	error ("nbsd_thread_fetch_registers: td_map_id2thr: %s\n",
	       td_err_string (val));
      if ((val = p_td_thr_getregs (thread, 0, &gregs)) != 0)
	error ("nbsd_thread_fetch_registers: td_thr_getregs: %s\n",
	       td_err_string (val));
      rs = gdbarch_regset_from_core_section(current_gdbarch, ".reg", 
					    sizeof(gregset_t));
      rs->supply_regset(rs, current_regcache, regno, &gregs, sizeof(gregs));
      if ((val = p_td_thr_getregs (thread, 1, &fpregs)) == 0) {
	frs = gdbarch_regset_from_core_section(current_gdbarch, ".reg2",
					       sizeof(fpregset_t));
	frs->supply_regset(frs, current_regcache, regno, &fpregs,
			   sizeof(fpregs));
      }
    }
  else
    {
      if (target_has_execution)
	target_beneath.to_fetch_registers (regno);
      else
	{
	  inferior_ptid = pid_to_ptid ((GET_LWP (inferior_ptid) << 16) | 
				        GET_PID (inferior_ptid));
	  target_beneath.to_fetch_registers (regno);
	}
    }
  
  do_cleanups (old_chain);
}

static void
nbsd_thread_store_registers (int regno)
{
  td_thread_t *thread;
  gregset_t gregs;
  fpregset_t fpregs;
  const struct regset *rs, *frs;
  int val;

  if (nbsd_thread_active && IS_THREAD_P (inferior_ptid))
    {
      val = p_td_map_id2thr (main_ta, GET_THREAD (inferior_ptid), &thread);
      if (val != 0)
	error ("nbsd_thread_store_registers: td_map_id2thr: %s\n",
	      td_err_string (val));

      rs = gdbarch_regset_from_core_section(current_gdbarch, ".reg",
					    sizeof (gregset_t));
      frs = gdbarch_regset_from_core_section(current_gdbarch, ".reg2",
				   sizeof (fpregset_t));
      rs->collect_regset(rs, current_regcache, -1, &gregs, sizeof(gregs));
      frs->collect_regset(frs, current_regcache, -1, &fpregs,
			  sizeof (fpregs));

      val = p_td_thr_setregs (thread, 0, &gregs);
      if (val != 0)
	error ("nbsd_thread_store_registers: td_thr_setregs: %s\n",
	      td_err_string (val));
      val = p_td_thr_setregs (thread, 1, &fpregs);
      if (val != 0)
	error ("nbsd_thread_store_registers: td_thr_setregs: %s\n",
	      td_err_string (val));
    }
  else
    target_beneath.to_store_registers (regno);

}


/* Clean up after the inferior dies.  */

static void
nbsd_thread_mourn_inferior (void)
{
  struct target_ops *t;

  if (nbsd_thread_active)
    nbsd_thread_deactivate ();

  t = &target_beneath;
  while (t != NULL)
    {
      if (t->to_mourn_inferior != NULL)
        {
          if (debug_nbsd_thread)
            {
              fprintf_unfiltered (gdb_stdlog,
                 "%s: calling to_mourn_inferior from '%s',\n", __func__, t->to_shortname);
            }
          t->to_mourn_inferior ();
          break;
        }
      t = find_target_beneath (t);
    }
  unpush_target (&nbsd_thread_ops);

#if 0
  find_target_beneath (nbsd_thread_ops_hack)->to_mourn_inferior ();
#endif
}

/* Convert a ptid to printable form. */

char *
nbsd_thread_pid_to_str (ptid_t ptid)
{
  static char buf[100];
  td_thread_t *th;
  int retval;
  char name[32];

  if ((GET_THREAD(ptid) == 0) &&
      (GET_LWP(ptid) == 0) && 
      (nbsd_thread_active == 0))
    sprintf (buf, "process %d", GET_PID (ptid));
  else if (IS_THREAD_P (ptid))
    {
      if ((p_td_map_id2thr (main_ta, GET_THREAD (ptid), &th) == 0) &&
	  (p_td_thr_getname (th, name, 32) == 0) &&
	  (name[0] != '\0'))
	sprintf (buf, "Thread %ld (%s)", GET_THREAD (ptid), name);
      else
	sprintf (buf, "Thread %ld", GET_THREAD (ptid));
    }
  else
    sprintf (buf, "LWP %ld", GET_LWP (ptid));

  return buf;
}


static void
nbsd_thread_inferior_created (struct target_ops *objfile, int from_tty)
{
  int val;

  if (debug_nbsd_thread)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "%s: called\n", __func__);
    }

  /* Initialize the thread debugging library.  This needs to be
     done after the shared libraries are located because it needs
     information from the user's thread library.  */
  val = p_td_open (&nbsd_thread_callbacks, NULL, &main_ta);
  if (val == TD_ERR_NOLIB)
    return;
  else if (val != 0)
    {
      warning ("nbsd_thread_inferior_created: td_open: %s",
	       td_err_string (val));
      return;
    }

  nbsd_thread_present = 1;

  if (!ptid_equal (inferior_ptid, null_ptid))
    {
      target_beneath = current_target;
      push_target (&nbsd_thread_ops);
      nbsd_thread_activate();
    }
}

/* This routine is called whenever a new symbol table is read in, or when all
   symbol tables are removed.  libthread_db can only be initialized when it
   finds the right variables in libthread.so.  Since it's a shared library,
   those variables don't show up until the library gets mapped and the symbol
   table is read in.  */

/* This new_objfile event is now managed by a chained function pointer.
 * It is the callee's responsability to call the next client on the chain.
 */

static const char *nbsd_thread_solib_name =  "/usr/lib/libpthread.so";

static void
nbsd_thread_solib_loaded (struct so_list *so)
{
  int val;

  if (debug_nbsd_thread)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "%s: called\n", __func__);
    }

  if (strncmp (so->so_original_name, nbsd_thread_solib_name,
	       strlen(nbsd_thread_solib_name)) != 0)
    return;

  /* Don't do anything if we've already fired up the debugging library */
  if (nbsd_thread_active)
    return;

  solib_read_symbols(so, so->from_tty);

  /* Now, initialize the thread debugging library.  This needs to be
     done after the shared libraries are located because it needs
     information from the user's thread library.  */
  val = p_td_open (&nbsd_thread_callbacks, NULL, &main_ta);
  if (val == TD_ERR_NOLIB)
    return;
  else if (val != 0)
    {
      warning ("target_new_objfile: td_open: %s", td_err_string (val));
      return;
    }

  nbsd_thread_present = 1;

  if (!ptid_equal (inferior_ptid, null_ptid))
    {
      target_beneath = current_target;
      push_target (&nbsd_thread_ops);
      nbsd_thread_activate();
    }

}

static void
nbsd_thread_solib_unloaded (struct so_list *so)
{
  if (debug_nbsd_thread)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "%s: called\n", __func__);
    }

  if (strncmp (so->so_original_name, nbsd_thread_solib_name,
	       strlen(nbsd_thread_solib_name)) != 0)
    return;
  
  nbsd_thread_deactivate();
  unpush_target (&nbsd_thread_ops);

}

static int
nbsd_thread_alive (ptid_t ptid)
{
  td_thread_t *th;
  td_thread_info_t ti;
  int val;

  if (nbsd_thread_active)
    {
      if (IS_THREAD_P (ptid))
	{
	  val = 0;
	  if (p_td_map_id2thr (main_ta, GET_THREAD (ptid), &th) == 0)
	    {
	      /* Thread found */
	      if ((p_td_thr_info (th, &ti) == 0) &&
		  (ti.thread_state != TD_STATE_ZOMBIE) &&
		  (ti.thread_type != TD_TYPE_SYSTEM))
		val = 1;
	    }
	}
      else if (IS_LWP_P (ptid))
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
	val = target_beneath.to_thread_alive (ptid);
    }
  else
    val = target_beneath.to_thread_alive (ptid);

  return val;
}

/* Worker bee for find_new_threads
   Callback function that gets called once per USER thread (i.e., not
   kernel) thread. */

static int
nbsd_find_new_threads_callback (td_thread_t *th, void *ignored)
{
  td_thread_info_t ti;
  ptid_t ptid;

  if (p_td_thr_info (th, &ti) != 0)
      return -1;

  ptid = BUILD_THREAD (ti.thread_id, main_ptid);
  if (ti.thread_type == TD_TYPE_USER &&
      ti.thread_state != TD_STATE_BLOCKED &&
      ti.thread_state != TD_STATE_ZOMBIE &&
      !in_thread_list (ptid))
    add_thread (ptid);

  return 0;
}

static void
nbsd_find_new_threads (void)
{ 
 int retval;
  ptid_t ptid;
  td_thread_t *thread;

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
	  p_td_map_lwp2thr (main_ta, pl.pl_lwpid, &thread);
#if 0
	  ptid = BUILD_LWP (pl.pl_lwpid, main_ptid);
	  if (!in_thread_list (ptid))
	    add_thread (ptid);
#endif
	  retval = ptrace (PT_LWPINFO, GET_PID(inferior_ptid), (void *)&pl, sizeof(pl));
	}
    }

  retval = p_td_thr_iter (main_ta, nbsd_find_new_threads_callback, (void *) 0);
  if (retval != 0)
    error ("nbsd_find_new_threads: td_thr_iter: %s",
	   td_err_string (retval));
}

#if 0
/* Fork an inferior process, and start debugging it with /proc.  */

static void
nbsd_thread_create_inferior (char *exec_file, char *allargs, char **env,
			     int from_tty)
{
  if (nbsd_thread_present && !nbsd_thread_active)
    push_target(&nbsd_thread_ops);

  find_target_beneath (nbsd_thread_ops_hack)->to_create_inferior (exec_file, allargs, env, from_tty);

  if (nbsd_thread_present && !nbsd_thread_active)
    nbsd_thread_activate();
}
#endif

static int
waiter_cb (td_thread_t *th, void *s)
{
  int ret;
  td_thread_info_t ti;

  if ((ret = p_td_thr_info (th, &ti)) == 0)
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
  td_sync_t *ts;
  td_sync_info_t tsi;
  char name[32];

  if ((ret = p_td_thr_info (th, &ti)) == 0)
    {
      if (ti.thread_type != TD_TYPE_USER)
	return 0;
      p_td_thr_getname(th, name, 32);
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
	  p_td_thr_sleepinfo (th, &ts);
	  p_td_sync_info (ts, &tsi);
	  if (tsi.sync_type == TD_SYNC_JOIN)
	    {
	      p_td_thr_info (tsi.sync_data.join.thread, &ti2);
	      printf ("joining thread %d ", ti2.thread_id);
	    }
	  else
	    {
	      printf_filtered ("on %s at %p ",
			       syncnames[tsi.sync_type],
			       (void *)tsi.sync_addr);
	    }
	}

      if (ti.thread_hasjoiners)
	{
	  printf_filtered ("(being joined by");
	  p_td_thr_join_iter (th, waiter_cb, NULL);
	  printf_filtered (")");
	}
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
      val = p_td_thr_iter (main_ta, info_cb, args);
      if (val != 0)
	error ("nbsd_find_new_threads: td_thr_iter: %s", td_err_string (val));
    }
  else
    printf_filtered ("Thread debugging not active.\n");
}

static void
nbsd_thread_examine_cmd (char *exp, int from_tty)
{
  struct expression *expr;
  struct value *val;
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

  if ((ret = p_td_map_pth2thr (main_ta, (pthread_t) addr, &th)) != 0)
    error ("nbsd_thread_examine_command: td_map_pth2thr: %s",
	   td_err_string (ret));
  
  info_cb (th, NULL);
}


static void
nbsd_thread_sync_cmd (char *exp, int from_tty)
{
  struct expression *expr;
  struct value *val;
  CORE_ADDR addr;
  td_sync_t *ts;
  td_sync_info_t tsi;
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

  if ((ret = p_td_map_addr2sync (main_ta, (caddr_t)addr, &ts)) != 0)
    error ("nbsd_thread_sync_cmd: td_map_addr2sync: %s", td_err_string (ret));

  if ((ret = p_td_sync_info (ts, &tsi)) != 0)
    error ("nbsd_thread_sync_cmd: td_sync_info: %s", td_err_string (ret));

  printf_filtered ("%p: %s ", (void *)addr, syncnames[tsi.sync_type]);

  if (tsi.sync_type == TD_SYNC_MUTEX)
    {
      if (!tsi.sync_data.mutex.locked)
	printf_filtered (" unlocked ");
      else
	{
	  p_td_thr_info (tsi.sync_data.mutex.owner, &ti);
	  printf_filtered (" locked by thread %d", ti.thread_id);
	}
    }
  else if (tsi.sync_type == TD_SYNC_SPIN)
    {
      if (!tsi.sync_data.spin.locked)
	printf_filtered (" unlocked ");
      else
	printf_filtered (" locked (waiters not tracked)");
    }
  else if (tsi.sync_type == TD_SYNC_JOIN)
    {
      p_td_thr_info (tsi.sync_data.join.thread, &ti);
      printf_filtered (" %d ", ti.thread_id);
    }
  else if (tsi.sync_type == TD_SYNC_RWLOCK)
    {
      if (!tsi.sync_data.rwlock.locked)
	printf_filtered (" unlocked ");
      else
	{
	  printf_filtered (" locked ");
	  if (tsi.sync_data.rwlock.readlocks > 0)
	    printf_filtered ("by %d reader%s ", 
			     tsi.sync_data.rwlock.readlocks,
			     (tsi.sync_data.rwlock.readlocks > 1) ? "s" : "");
	  else
	    {
	      p_td_thr_info (tsi.sync_data.rwlock.writeowner, &ti);
	      printf_filtered (" by writer %d ", ti.thread_id);
	    }
	}
    }

  if (tsi.sync_haswaiters)
    {
      printf_filtered (" waiters:");
      if ((ret = p_td_sync_waiters_iter (ts, waiter_cb, NULL)) != 0)
	error ("nbsd_thread_sync_cmd: td_sync_info: %s",
	       td_err_string (ret));
    }

  printf_filtered ("\n");
}

int
tsd_cb (pthread_key_t key, void (*destructor)(void *), void *ignore)
{
  struct minimal_symbol *ms;
  char *name;

  printf_filtered ("Key %3d   ", key);

  printf_filtered ("Destructor");
  print_address((CORE_ADDR)destructor, gdb_stdout);
  printf_filtered ("\n");
  return 0;
}

static void
nbsd_thread_tsd_cmd (char *exp, int from_tty)
{
  p_td_tsd_iter (main_ta, tsd_cb, NULL);
}

static void
nbsd_add_to_thread_list (bfd *abfd, asection *asect, PTR reg_sect_arg)
{
  int regval;
  td_thread_t *dummy;

  if (strncmp (bfd_section_name (abfd, asect), ".reg/", 5) != 0)
    return;

  regval = atoi (bfd_section_name (abfd, asect) + 5);

  p_td_map_lwp2thr (main_ta, regval >> 16, &dummy);

  add_thread (BUILD_LWP(regval >> 16, main_ptid));
}

static void
nbsd_thread_core_open (char *filename, int from_tty)
{
  int val;

  target_beneath.to_open (filename, from_tty);

  if (nbsd_thread_present)
    {
      val = p_td_open (&nbsd_thread_callbacks, NULL, &main_ta);
      if (val == 0)
	{
	  main_ptid = pid_to_ptid (elf_tdata (core_bfd)->core_pid);
	  nbsd_thread_active = 1;
	  init_thread_list ();
	  bfd_map_over_sections (core_bfd, nbsd_add_to_thread_list, NULL);
	  nbsd_find_new_threads ();
	}
      else
	error ("nbsd_core_open: td_open: %s", td_err_string (val));
    }
}

static void
nbsd_thread_core_close (int quitting)
{
  struct target_ops *t;

  if (debug_nbsd_thread)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "%s: called\n", __func__);
    }

  /* XXX Setting these here is a gross hack. It needs to be set to
   * XXX the "current thread ID" when a core file is loaded, but there's
   * XXX no hook that happens then. However, core_open() in corelow is
   * XXX pretty likely to call this.
   *
   * XXXNH is this still true??? I don't think so.
   */
  inferior_ptid = minus_one_ptid;

  t = &target_beneath;
  while (t != NULL)
    {
      if (t->to_close != NULL)
        {
          if (debug_nbsd_thread)
            {
              fprintf_unfiltered (gdb_stdlog,
                 "%s: not calling to_close from '%s',\n", __func__, t->to_shortname);
            }
          //t->to_close (quitting);
          break;
        }
      t = find_target_beneath (t);
    }
}

/*
 * Process operation callbacks
 */
static int
nbsd_thread_proc_read (void *arg, caddr_t addr, void *buf, size_t size)
{
  int val;

  val = target_read_memory ((CORE_ADDR)addr, buf, size);

  if (val == 0)
    return 0;
  else
    return TD_ERR_IO;
}


static int
nbsd_thread_proc_write (void *arg, caddr_t addr, void *buf, size_t size)
{
  int val;

  val = target_write_memory ((CORE_ADDR)addr, buf, size);

  if (val == 0)
    return 0;
  else
    return TD_ERR_IO;
}

static int
nbsd_thread_proc_lookup (void *arg, char *sym, caddr_t *addr)
{
  struct minimal_symbol *ms;

  ms = lookup_minimal_symbol (sym, NULL, NULL);

  if (!ms)
    return TD_ERR_NOSYM;

  *addr = (caddr_t) SYMBOL_VALUE_ADDRESS (ms);

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
      *size = sizeof (fpregset_t);
      break;
    default:
      return TD_ERR_INVAL;
    }

  return 0;
}

static int
nbsd_thread_proc_getregs (void *arg, int regset, int lwp, void *buf)
{
  struct cleanup *old_chain;
  const struct regset *rs;
  int ret;

  old_chain = save_inferior_ptid ();

  if (target_has_execution)
    {
      /* Fetching registers from a live process requires that
	 inferior_ptid is a LWP value rather than a thread value. */
      inferior_ptid = BUILD_LWP (lwp, main_ptid);
    }
  else
    {
      /* Fetching registers from a core process requires that
	 the PID value of inferior_ptid have the funky value that
	 the kernel drops rather than the real PID. Gross. */
      inferior_ptid = pid_to_ptid ((lwp << 16) | GET_PID (main_ptid));
    }
  target_beneath.to_fetch_registers (-1);

  ret = 0;
  switch (regset)
    {
    case 0:
      rs = gdbarch_regset_from_core_section(current_gdbarch, ".reg", 
	  sizeof (gregset_t));
      rs->collect_regset(rs, current_regcache, -1, buf, sizeof (gregset_t));
      break;
    case 1:
      rs = gdbarch_regset_from_core_section(current_gdbarch, ".reg2", 
	  sizeof (fpregset_t));
      rs->collect_regset(rs, current_regcache, -1, buf, sizeof (fpregset_t));
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
  struct cleanup *old_chain;
  const struct regset *rs;
  int ret;

  ret = 0;
  old_chain = save_inferior_ptid ();

  switch (regset)
    {
    case 0:
      rs = gdbarch_regset_from_core_section(current_gdbarch, ".reg",
					    sizeof (gregset_t));
      if (rs == NULL)
	ret = TD_ERR_ERR;
      else
	rs->supply_regset(rs, current_regcache, -1, buf, sizeof(gregset_t));
      break;
    case 1:
      rs = gdbarch_regset_from_core_section(current_gdbarch, ".reg2",
					    sizeof (fpregset_t));
      if (rs == NULL)
	ret = TD_ERR_ERR;
      else
	rs->supply_regset(rs, current_regcache, -1, buf, sizeof(fpregset_t));
      break;
    default: /* XXX need to handle other reg sets: SSE, AltiVec, etc. */
      ret = TD_ERR_INVAL;
    }

  /* Storing registers requires that inferior_ptid is a LWP value
     rather than a thread value. */
  inferior_ptid = BUILD_LWP (lwp, main_ptid);
  target_beneath.to_store_registers (-1);
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
  struct target_ops *t = &nbsd_thread_ops;

  t->to_shortname = "netbsd-threads";
  t->to_longname = "NetBSD threads";
  t->to_doc = "NetBSD threads support";

  /* XXX linux doesn't use these */
  t->to_open = nbsd_thread_core_open;
  t->to_close = nbsd_thread_core_close;

  /* linux does use these */
  t->to_fetch_registers = nbsd_thread_fetch_registers;
  t->to_store_registers = nbsd_thread_store_registers;
  t->to_xfer_partial = nbsd_thread_xfer_partial;
  t->to_find_new_threads = nbsd_find_new_threads;
  t->to_pid_to_str = nbsd_thread_pid_to_str;
  t->to_thread_alive = nbsd_thread_alive;
  t->to_has_thread_control = tc_none;
  t->to_mourn_inferior = nbsd_thread_mourn_inferior;
#if 0
  t->to_create_inferior = nbsd_thread_create_inferior;	/* XXX NH new? */
#endif
  t->to_wait = nbsd_thread_wait;
  t->to_resume = nbsd_thread_resume;

#if 0
  t->to_extra_thread_info = bsd_uthread_extra_thread_info;
#endif
  t->to_stratum = thread_stratum;
  t->to_magic = OPS_MAGIC;
/*
  thread_db_ops.to_attach = thread_db_attach;
  thread_db_ops.to_detach = thread_db_detach;
  thread_db_ops.to_kill = thread_db_kill;
  thread_db_ops.to_create_inferior = thread_db_create_inferior;
  thread_db_ops.to_post_startup_inferior = thread_db_post_startup_inferior;
  thread_db_ops.to_has_thread_control = tc_schedlock;
  thread_db_ops.to_get_thread_local_address
    = thread_db_get_thread_local_address;
*/
}

void
_initialize_nbsd_thread (void)
{
  static struct cmd_list_element *thread_examine_list = NULL;
  void *dlhandle;

  init_nbsd_thread_ops();

  dlhandle = dlopen ("libpthread_dbg.so", RTLD_NOW);
  if (!dlhandle)
    goto die;

#define resolve(X) \
  if (!(p_##X = dlsym (dlhandle, #X))) \
    goto die;

  resolve(td_open);
  resolve(td_close);
  resolve(td_thr_iter);
  resolve(td_thr_info);
  resolve(td_thr_getname);
  resolve(td_thr_getregs);
  resolve(td_thr_setregs);
  resolve(td_thr_join_iter);
  resolve(td_thr_sleepinfo);
  resolve(td_sync_info);
  resolve(td_sync_waiters_iter);
  resolve(td_map_addr2sync);
  resolve(td_map_pth2thr);
  resolve(td_map_id2thr);
  resolve(td_map_lwp2thr);
  resolve(td_map_lwps);
  resolve(td_tsd_iter);
  resolve(td_thr_tsd);
  resolve(td_thr_suspend);
  resolve(td_thr_resume);

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

  observer_attach_solib_loaded (nbsd_thread_solib_loaded);
  observer_attach_solib_unloaded (nbsd_thread_solib_unloaded);
  observer_attach_inferior_created (nbsd_thread_inferior_created);

  add_setshow_zinteger_cmd ("nbsd-thread", no_class, &debug_nbsd_thread, _("\
Set debugging of NetBSD thread module."), _("\
Show debugging of NetBSD thread module."), _("\
Enables printf debugging output."),
			    NULL,
			    show_debug_nbsd_thread,
			    &setdebuglist, &showdebuglist);


  return;

 die:
  fprintf_unfiltered (gdb_stderr, "\
[GDB will not be able to debug user-mode threads: %s]\n", dlerror ());

  if (dlhandle)
    dlclose (dlhandle);

  return;
}


#if 0

/* deprecated_target_new_objfile_hook callback.

   If OBJFILE is non-null, check whether libpthread_dbg.so was
   just loaded, and if so, prepare for user-mode thread debugging.

   If OBJFILE is null, libpthread_dbg.so has gone away, so stop
   debugging user-mode threads.

   This function often gets called with nbsd_thread_active == 0.  */

static void
nbsd_thread_new_objfile (struct objfile *objfile)
{
  int val;

  warning ("nbsd_thread_newobjfile: objfile: %p",
    objfile);

  if (!objfile)
    {
      nbsd_thread_active = 0;
      goto quit;
    }

  /* Don't initialize twice.  */
  if (nbsd_thread_active)
    goto quit;

  /* Initialize base_ops.  */
  base_ops = current_target;

  /* Initialize the thread debugging library.  This needs to be
     done after the shared libraries are located because it needs
     information from the user's thread library.  */
  val = p_td_open (&nbsd_thread_callbacks, NULL, &main_ta);
  if (val == TD_ERR_NOLIB) {
  warning ("nbsd_thread_newobjfile: TD_ERR_NOLIB: %p",
    objfile);

    return; }
  else if (val != 0)
    {
      warning ("nbsd_thread_inferior_created: td_open: %s",
	       td_err_string (val));
      return;
    }

  nbsd_thread_present = 1;

  if (!ptid_equal (inferior_ptid, null_ptid))
    {
      push_target (&nbsd_thread_ops);
      nbsd_thread_activate();
    }

quit:
  /* Call predecessor on chain, if any.  */
  if (target_new_objfile_chain)
    target_new_objfile_chain (objfile);
}

#endif

