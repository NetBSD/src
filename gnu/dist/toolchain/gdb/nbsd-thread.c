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

#include <pthread.h>
#include <pthread_dbg.h>

#include "symtab.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbthread.h"
#include "bfd.h"
#include "elf-bfd.h"
#include "target.h"
#include "inferior.h"
#include "gdbcmd.h"
#include "gdbcore.h"

#include <machine/reg.h>

/* nbsd_thread_present indicates that new_objfile has spotted
   libpthread and that post_attach() or create_inferior() should fire
   up thread debugging. */
static int nbsd_thread_present = 0;

/* nbsd_thread_active indicates that thread debugging is up and running, and
   in particular that main_ta and main_pid are valid. */
static int nbsd_thread_active = 0;

static int main_pid;		/* Real process ID */

static int cached_thread;

struct target_ops nbsd_thread_ops;
struct target_ops nbsd_core_ops;

struct td_proc_callbacks_t nbsd_thread_callbacks;
struct td_proc_callbacks_t nbsd_core_callbacks;

/* place to store core_ops before we overwrite it */
static struct target_ops orig_core_ops;

extern struct target_ops child_ops;  /* target vector for inftarg.c */
extern struct target_ops core_ops; /* target vector for corelow.c */

extern int child_suppress_run;

/* inferior_pid is a global variable that is used by gdb to identify
   the entity that is under the debugging microscope. In normal
   operation, it would be the PID of the child process. For threaded
   programs, the thread module of GDB keeps a "pid" value for each
   thread and sets inferior_pid to that value when switching
   threads. Therefore, inferior_pid needs to encode some kind of
   thread identifier. It can't be the pthread_t, because that's a
   pointer, and pointers can be larger than ints, so we use the "ID"
   number given to us by libthread_db.

   Unfortunately, inferior_pid is also used implicitly by the various
   ptrace target operations, so when we call one of those we have to
   save the thread-specific inferior_pid, set inferior_pid to
   main_pid, make the call, and then do the restoration. In order to
   guarantee that inferior_pid gets restored (in case of errors), we
   need to call save_inferior_pid before changing it.  At the end of the
   function, we invoke do_cleanups to restore it. */

static struct cleanup * save_inferior_pid PARAMS ((void));
static void restore_inferior_pid PARAMS ((void *pid));

static int find_active_thread PARAMS ((void));
static void nbsd_find_new_threads PARAMS ((void));

static struct cleanup *
save_inferior_pid ()
{
  return make_cleanup (restore_inferior_pid, (void *)(long)inferior_pid);
}

static void
restore_inferior_pid (pid)
     void *pid;
{
  /* XXX terrible type punning, but C doesn't have closures... */
  inferior_pid = (int)(long)pid;
}

static td_proc_t *main_ta;

static const char *syncnames[] = {"unknown",
			   "mutex",
			   "cond var",
			   "spinlock",
			   "joining thread"};

struct string_map
  {
    int num;
    char *str;
  };

static char *
td_err_string (errcode)
     int errcode;
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
nbsd_thread_activate ()
{
  int val;

  val = td_open (&nbsd_thread_callbacks, NULL, &main_ta);
  if (val != 0)
    error ("nbsd_thread_create_inferior: td_open: %s",
	  td_err_string (val));

  main_pid = inferior_pid;
  nbsd_thread_active = 1;
  nbsd_find_new_threads ();
  val = find_active_thread ();
  if (val == -1)
    error ("No active thread found\n");
  inferior_pid = val;
}

static void
nbsd_thread_deactivate ()
{
  inferior_pid = main_pid;
  main_pid = 0;
  cached_thread = 0;
  nbsd_thread_active = 0;
  init_thread_list ();

  td_close (main_ta);
}

static void
nbsd_thread_attach (args, from_tty)
     char *args;
     int from_tty;
{
  child_ops.to_attach (args, from_tty);

  push_target (&nbsd_thread_ops);
}

/* Attach to process PID, then initialize for debugging it
   and wait for the trace-trap that results from attaching.  */

static void
nbsd_thread_post_attach (pid)
	int pid;
{
  int val;

  child_ops.to_post_attach (pid);

  if (nbsd_thread_present)
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
nbsd_thread_detach (args, from_tty)
     char *args;
     int from_tty;
{
  struct cleanup *old_chain;
  old_chain = save_inferior_pid ();

  if (nbsd_thread_active)
    nbsd_thread_deactivate ();
  unpush_target (&nbsd_thread_ops);
  child_ops.to_detach (args, from_tty);

  do_cleanups (old_chain);
}

static void
nbsd_thread_resume (pid, step, signo)
     int pid;
     int step;
     enum target_signal signo;
{
  struct cleanup *old_chain;
  old_chain = save_inferior_pid ();

  if (nbsd_thread_active)
    inferior_pid = main_pid;
  pid = inferior_pid;

  child_ops.to_resume (pid, step, signo);

  cached_thread = 0;

  do_cleanups (old_chain);
}


static int
find_active_thread ()
{
  int val;
  td_thread_t *thread;
  td_thread_info_t ti;

  val = td_map_lwps (main_ta);
  if (val != 0)
    {
      warning ("find_active_thread: td_map_lwps: %s\n", td_err_string (val));
      return -1;
    }
  if (cached_thread != 0)
    return cached_thread;

  val = td_map_lwp2thr (main_ta, 0, &thread);
  if (val != 0)
    {
      warning ("find_active_thread: td_map_lwp2thr: %s\n",
	       td_err_string (val));
      return -1;
    }
  val = td_thr_info (thread, &ti);
  if (val != 0)
    {
      warning ("find_active_thread: td_thr_info: %s\n", td_err_string (val));
      return -1;
    }

  val = BUILD_THREAD (ti.thread_id, main_pid);
  cached_thread = val;
  return val;
}


/* Wait for any threads to stop.  We may have to convert PID from a thread id
   to a LWP id, and vice versa on the way out.  */

static int
nbsd_thread_wait (pid, ourstatus)
     int pid;
     struct target_waitstatus *ourstatus;
{
  int rtnval;
  struct cleanup *old_chain;
  old_chain = save_inferior_pid ();

  if (nbsd_thread_active)
    inferior_pid = main_pid;

  if (pid != -1)
    pid = inferior_pid;
  rtnval = child_ops.to_wait (pid, ourstatus);
  if (rtnval == 0 && nbsd_thread_active)
    {
      rtnval = find_active_thread ();
      if (rtnval == -1)
	error ("No active thread!\n");
      if (!in_thread_list (rtnval))
	add_thread (rtnval);
    }
  do_cleanups (old_chain);

  return rtnval;
}

static void
nbsd_thread_fetch_registers (regno)
  int regno;
{
  td_thread_t *thread;
  struct reg gregs;
  struct fpreg fpregs;
  int val;
  struct cleanup *old_chain;

  old_chain = save_inferior_pid ();

  if (nbsd_thread_active && IS_THREAD (inferior_pid))
    {
      if ((val = td_map_id2thr (main_ta, GET_THREAD (inferior_pid), &thread)) != 0)
	error ("nbsd_thread_fetch_registers: td_map_id2thr: %s\n",
	       td_err_string (val));
      if ((val = td_thr_getregs (thread, 0, &gregs)) != 0)
	error ("nbsd_thread_fetch_registers: td_thr_getregs: %s\n",
	       td_err_string (val));
      if ((val = td_thr_getregs (thread, 1, &fpregs)) != 0)
	error ("nbsd_thread_fetch_registers: td_thr_getregs: %s\n",
	       td_err_string (val));
      nbsd_reg_to_internal (&gregs);
      nbsd_fpreg_to_internal (&fpregs);
    }
  else
    {
      if (nbsd_thread_active)
	inferior_pid = BUILD_LWP (GET_LWP (inferior_pid), 
				  GET_PROCESS (inferior_pid));
      if (target_has_execution)
	child_ops.to_fetch_registers (regno);
      else
	orig_core_ops.to_fetch_registers (regno);
    }
  
  do_cleanups (old_chain);
}

static void
nbsd_thread_store_registers (regno)
  int regno;
{
  td_thread_t *thread;
  struct reg gregs;
  struct fpreg fpregs;
  int val;
  struct cleanup *old_chain;

  old_chain = save_inferior_pid ();

  if (nbsd_thread_active && IS_THREAD (inferior_pid))
    {
      val = td_map_id2thr (main_ta, GET_THREAD (inferior_pid), &thread);
      if (val != 0)
	error ("nbsd_thread_store_registers: td_map_id2thr: %s\n",
	      td_err_string (val));

      nbsd_internal_to_reg (&gregs);
      nbsd_internal_to_fpreg (&fpregs);

      val = td_thr_setregs (thread, 0, &gregs);
      if (val != 0)
	error ("nbsd_thread_store_registers: td_thr_setregs: %s\n",
	      td_err_string (val));
      val = td_thr_setregs (thread, 1, &fpregs);
      if (val != 0)
	error ("nbsd_thread_store_registers: td_thr_setregs: %s\n",
	      td_err_string (val));
    }
  else
    {
      if (nbsd_thread_active)
	inferior_pid = BUILD_LWP (GET_LWP (inferior_pid), 
				  GET_PROCESS (inferior_pid));
      if (target_has_execution)
	child_ops.to_store_registers (regno);
      else
	orig_core_ops.to_store_registers (regno);
    }

  do_cleanups (old_chain);
}


/* Get ready to modify the registers array.  On machines which store
   individual registers, this doesn't need to do anything.  On machines
   which store all the registers in one fell swoop, this makes sure
   that registers contains all the registers from the program being
   debugged.  */

static void
nbsd_thread_prepare_to_store ()
{
  struct cleanup *old_chain;
  old_chain = save_inferior_pid ();

  if (nbsd_thread_active)
    inferior_pid = main_pid;

  child_ops.to_prepare_to_store ();

  do_cleanups (old_chain);
}


static int
nbsd_thread_xfer_memory (memaddr, myaddr, len, dowrite, target)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
     int dowrite;
     struct target_ops *target;	/* ignored */
{
  int retval;
  struct cleanup *old_chain;

  old_chain = save_inferior_pid ();

  if (nbsd_thread_active)
    inferior_pid = main_pid;

  if (target_has_execution)
    retval = child_ops.to_xfer_memory (memaddr, myaddr, len, dowrite, target);
  else
    retval = orig_core_ops.to_xfer_memory (memaddr, myaddr, len, dowrite, target);

  do_cleanups (old_chain);

  return retval;
}

/* ARGSUSED */
static int
nbsd_thread_has_exited (pid, wait_status, exit_status)
     int pid;
     int wait_status;
     int *exit_status;
{
  struct cleanup *old_chain;
  int val;

  old_chain = save_inferior_pid ();
  if (nbsd_thread_active)
    inferior_pid = main_pid;
  pid = inferior_pid;

  val = child_ops.to_has_exited (pid, wait_status, exit_status);
  do_cleanups (old_chain);
  return val;
}

/* Clean up after the inferior dies.  */

static void
nbsd_thread_mourn_inferior ()
{
  struct cleanup *old_chain;
  old_chain = save_inferior_pid ();

  if (nbsd_thread_active)
    nbsd_thread_deactivate ();

  unpush_target (&nbsd_thread_ops);
  child_ops.to_mourn_inferior ();

  do_cleanups (old_chain);
}

/* Convert a pid to printable form. */

char *
nbsd_pid_to_str (pid)
     int pid;
{
  static char buf[100];
  td_thread_t *th;
  int retval;
  char name[32];

  if (IS_THREAD (pid))
    {
      if ((td_map_id2thr (main_ta, GET_THREAD(pid), &th) == 0) &&
	  (td_thr_getname (th, name, 32) == 0))
	sprintf (buf, "Thread %d (%s)", GET_THREAD (pid), name);
      else
	sprintf (buf, "Thread %d", GET_THREAD (pid));
    }
  else
    sprintf (buf, "LWP %d", GET_LWP (pid));

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

/* Saved pointer to previous owner of the new_objfile event. */
static void (*target_new_objfile_chain) PARAMS ((struct objfile *));

void
nbsd_thread_new_objfile (objfile)
     struct objfile *objfile;
{
  int val;

  if (!objfile)
    {
      nbsd_thread_present = 0;
      goto quit;
    }

  /* don't do anything if init failed */
  if (!child_suppress_run)
    goto quit;

  /* Don't do anything if we've already fired up the debugging library */
  if (nbsd_thread_present)
    goto quit;

  /* Now, initialize the thread debugging library.  This needs to be
     done after the shared libraries are located because it needs
     information from the user's thread library.  */
  val = td_open (&nbsd_thread_callbacks, NULL, &main_ta);
  if (val == TD_ERR_NOLIB)
    goto quit;
  else if (val != 0)
    {
      warning ("target_new_objfile: td_open: %s", td_err_string (val));
      goto quit;
    }
  td_close (main_ta);
  nbsd_thread_present = 1;

 quit:
  /* Call predecessor on chain, if any. */
  if (target_new_objfile_chain)
    target_new_objfile_chain (objfile);
}


static int
nbsd_thread_alive (pid)
     int pid;
{
  td_thread_t *th;
  td_thread_info_t ti;
  int retval;
  struct cleanup *old_chain;

  old_chain = save_inferior_pid ();

  if (nbsd_thread_active && IS_THREAD (pid))
    {
      retval = 0;
      if (td_map_id2thr (main_ta, GET_THREAD (pid), &th) == 0)
	{
	  /* Thread found */
	  if (td_thr_info (th, &ti) == 0)
	    retval = 1;
	}
    }
  else
    {
      if (nbsd_thread_active)
	inferior_pid = BUILD_LWP (GET_LWP (inferior_pid), 
				  GET_PROCESS (inferior_pid));
      if (target_has_execution)
	retval = child_ops.to_thread_alive (GET_PROCESS (pid));
      else
	retval = orig_core_ops.to_thread_alive (GET_PROCESS (pid));
    }

  do_cleanups (old_chain);
  return retval;
}


/* Worker bee for find_new_threads
   Callback function that gets called once per USER thread (i.e., not
   kernel) thread. */

static int
nbsd_find_new_threads_callback (th, ignored)
     td_thread_t *th;
     void *ignored;
{
  int retval;
  td_thread_info_t ti;
  int pid;

  if ((retval = td_thr_info (th, &ti)) != 0)
      return -1;

  pid = BUILD_THREAD (ti.thread_id, main_pid);
  if (ti.thread_type == TD_TYPE_USER &&
      ti.thread_state != TD_STATE_ZOMBIE &&
      !in_thread_list (pid))
    add_thread (pid);

  return 0;
}

static void
nbsd_find_new_threads ()
{
  int retval;

  if (nbsd_thread_active == 0)
	  return;

  /* don't do anything if init failed to resolve the libthread_db library */
  if (!child_suppress_run)
    return;

  if (inferior_pid == -1)
    {
      printf_filtered ("No process.\n");
      return;
    }
  retval = td_thr_iter (main_ta, nbsd_find_new_threads_callback, (void *) 0);
  if (retval != 0)
    error ("nbsd_find_new_threads: td_thr_iter: %s",
	  td_err_string (retval));
}


/* Mark our target-struct as eligible for stray "run" and "attach" commands.  */

static int
nbsd_thread_can_run ()
{
  return child_suppress_run;
}

static void
nbsd_thread_stop ()
{
  struct cleanup *old_chain;
  old_chain = save_inferior_pid ();

  if (nbsd_thread_active)
    inferior_pid = main_pid;
  child_ops.to_stop ();

  do_cleanups (old_chain);
}

/* Print status information about what we're accessing.  */

static void
nbsd_thread_files_info (ignore)
     struct target_ops *ignore;
{
  struct cleanup *old_chain;
  old_chain = save_inferior_pid ();

  if (nbsd_thread_active)
    inferior_pid = main_pid;

  child_ops.to_files_info (ignore);

  do_cleanups (old_chain);
}

static void
nbsd_thread_kill_inferior ()
{
  struct cleanup *old_chain;
  old_chain = save_inferior_pid ();

  if (nbsd_thread_active)
    inferior_pid = main_pid;

  child_ops.to_kill ();

  do_cleanups (old_chain);
}

static void
nbsd_thread_notice_signals (pid)
     int pid;
{
  struct cleanup *old_chain;
  old_chain = save_inferior_pid ();

  if (nbsd_thread_active)
    inferior_pid = main_pid;

  child_ops.to_notice_signals (pid);

  do_cleanups (old_chain);
}


/* Fork an inferior process, and start debugging it with /proc.  */

static void
nbsd_thread_create_inferior (exec_file, allargs, env)
     char *exec_file;
     char *allargs;
     char **env;
{
  int val;

  child_ops.to_create_inferior (exec_file, allargs, env);

  push_target (&nbsd_thread_ops);

  if (nbsd_thread_present)
    nbsd_thread_activate ();
}


static int
waiter_cb (th, s)
     td_thread_t *th;
     void *s;
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
info_cb (th, s)
     td_thread_t *th;
     void *s;
{
  int ret;
  td_thread_info_t ti, ti2;
  td_sync_t *ts;
  td_sync_info_t tsi;
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
	      if (tsi.sync_type == TD_SYNC_MUTEX &&
		  tsi.sync_data.mutex.owner != 0)
		{
		  td_thr_info (tsi.sync_data.mutex.owner, &ti2);
		  printf_filtered ("owned by %d ", ti2.thread_id);
		}
	    }
	}

      if (ti.thread_hasjoiners)
	{
	  printf_filtered ("(being joined by");
	  td_thr_join_iter (th, waiter_cb, NULL);
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
nbsd_thread_examine_all_cmd (args, from_tty)
     char *args;
     int from_tty;
{
  int val;

  if (nbsd_thread_active)
    {
      val = td_thr_iter (main_ta, info_cb, args);
      if (val != 0)
	error ("nbsd_find_new_threads: td_thr_iter: %s", td_err_string (val));
    }
  else
    printf_filtered ("Thread debugging not active.\n");
}

static void
nbsd_thread_examine_cmd (exp, from_tty)
     char *exp;
     int from_tty;
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

  if ((ret = td_map_pth2thr (main_ta, (pthread_t) addr, &th)) != 0)
    error ("nbsd_thread_examine_command: td_map_pth2thr: %s",
	  td_err_string (ret));

  info_cb (th, NULL);
}


static void
nbsd_thread_sync_cmd (exp, from_tty)
     char *exp;
     int from_tty;
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

  if ((ret = td_map_addr2sync (main_ta, (caddr_t)addr, &ts)) != 0)
    error ("nbsd_thread_sync_cmd: td_map_addr2sync: %s", td_err_string (ret));

  if ((ret = td_sync_info (ts, &tsi)) != 0)
    error ("nbsd_thread_sync_cmd: td_sync_info: %s", td_err_string (ret));

  printf_filtered ("%p: %s ", addr, syncnames[tsi.sync_type]);

  if (tsi.sync_type == TD_SYNC_MUTEX)
    {
      if (!tsi.sync_data.mutex.locked)
	printf_filtered (" unlocked ");
      else
	{
	  td_thr_info (tsi.sync_data.mutex.owner, &ti);
	  printf_filtered (" locked by thread %d", ti.thread_id);
	}
    }
  else if (tsi.sync_type == TD_SYNC_SPIN)
    {
      if (!tsi.sync_data.spin.locked)
	printf_filtered (" unlocked ");
      else
	{
	  td_thr_info (tsi.sync_data.mutex.owner, &ti);
	  printf_filtered (" locked (waiters not tracked)");
	}
    }
  else if (tsi.sync_type == TD_SYNC_JOIN)
    {
      td_thr_info (tsi.sync_data.join.thread, &ti);
      printf_filtered (" %d ", ti.thread_id);
    }

  if (tsi.sync_haswaiters)
    {
      printf_filtered (" waiters:");
      if ((ret = td_sync_waiters_iter (ts, waiter_cb, NULL)) != 0)
	error ("nbsd_thread_sync_cmd: td_sync_info: %s",
	       td_err_string (ret));
    }

  printf_filtered ("\n");
}

int
tsd_cb (key, destructor)
     pthread_key_t key;
     void (*destructor)(void *);
{
  struct minimal_symbol *ms;
  char *name;

  printf_filtered ("Key %3d   ", key);

  /* XXX What does GDB use to print a function? */
  ms = lookup_minimal_symbol_by_pc ((CORE_ADDR)destructor);

  if (!ms)
    name = "???";
  else
    name = SYMBOL_NAME (ms);

  printf_filtered ("Destructor %p <%s>\n", destructor, name);
}

static void
nbsd_thread_tsd_cmd (exp, from_tty)
     char *exp;
     int from_tty;
{
  td_tsd_iter (main_ta, tsd_cb, NULL);
}

static void
nbsd_add_to_thread_list (abfd, asect, reg_sect_arg)
     bfd *abfd;
     asection *asect;
     PTR reg_sect_arg;
{
  int regval;
  td_thread_t *dummy;

  asection *reg_sect = (asection *) reg_sect_arg;

  if (strncmp (bfd_section_name (abfd, asect), ".reg/", 5) != 0)
    return;

  regval = atoi (bfd_section_name (abfd, asect) + 5);

  td_map_lwp2thr (main_ta, regval >> 16, &dummy);

  add_thread (BUILD_LWP(regval >> 16, main_pid));
}

static void
nbsd_core_open (filename, from_tty)
     char *filename;
     int from_tty;
{
  int val;

  orig_core_ops.to_open (filename, from_tty);

  if (nbsd_thread_present)
    {
      val = td_open (&nbsd_thread_callbacks, NULL, &main_ta);
      if (val == 0)
	{
	  main_pid = elf_tdata (core_bfd)->core_pid;
	  nbsd_thread_active = 1;
	  bfd_map_over_sections (core_bfd, nbsd_add_to_thread_list,
				 bfd_get_section_by_name (core_bfd, ".reg"));
	  nbsd_find_new_threads ();
	}
      else
	error ("target_new_objfile: td_open: %s", td_err_string (val));
    }
}

static void
nbsd_core_close (quitting)
     int quitting;
{
  /* XXX Setting these here is a gross hack. It needs to be set to
   * XXX the "current thread ID" when a core file is loaded, but there's
   * XXX no hook that happens then. However, core_open() in corelow is
   * XXX pretty likely to call this.
   */
  inferior_pid = 0;

  orig_core_ops.to_close (quitting);
}

static void
nbsd_core_detach (args, from_tty)
     char *args;
     int from_tty;
{
  orig_core_ops.to_detach (args, from_tty);
}

static void
nbsd_core_files_info (t)
     struct target_ops *t;
{
  orig_core_ops.to_files_info (t);
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
      *size = sizeof (struct reg);
      break;
    case 1:
      *size = sizeof (struct fpreg);
      break;
    default:
      return TD_ERR_INVAL;
    }

  return 0;
}

static int
nbsd_thread_proc_getregs (void *arg, int regset, int lwp, void *buf)
{
  struct reg gregs;
  struct fpreg fpregs;
  struct cleanup *old_chain;
  int ret;

  old_chain = save_inferior_pid ();
  inferior_pid = main_pid;

  /* Fetching registers requires that inferior_pid have the
     funky LWP/process mix that the kernel drops. */
     inferior_pid = BUILD_LWP (lwp, inferior_pid);
  if (target_has_execution)
    child_ops.to_fetch_registers (-1);
  else
    orig_core_ops.to_fetch_registers (-1);

  nbsd_internal_to_reg (&gregs);
  nbsd_internal_to_fpreg (&fpregs);

  ret = 0;
  switch (regset)
    {
    case 0:
      memcpy (buf, &gregs, sizeof (struct reg));
      break;
    case 1:
      memcpy (buf, &fpregs, sizeof (struct fpreg));
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
  struct reg gregs;
  struct fpreg fpregs;
  struct cleanup *old_chain;
  int ret;

  ret = 0;
  old_chain = save_inferior_pid ();
  inferior_pid = main_pid;

  switch (regset)
    {
    case 0:
      memcpy (&gregs, buf, sizeof (struct reg));
      break;
    case 1:
      memcpy (&fpregs, buf, sizeof (struct fpreg));
      break;
    default: /* XXX need to handle other reg sets: SSE, AltiVec, etc. */
      ret = TD_ERR_INVAL;
    }

  /* Storing registers requires that inferior_pid have the
     funky LWP/process mix that the kernel drops. */
  inferior_pid = BUILD_LWP (lwp, inferior_pid);
  nbsd_reg_to_internal (&gregs);
  nbsd_fpreg_to_internal (&fpregs);
  child_ops.to_store_registers (-1);

  do_cleanups (old_chain);

  return ret;
}


static int
ignore (addr, contents)
     CORE_ADDR addr;
     char *contents;
{
  return 0;
}

static void
init_nbsd_proc_callbacks ()
{
  nbsd_thread_callbacks.proc_read = nbsd_thread_proc_read;
  nbsd_thread_callbacks.proc_write = nbsd_thread_proc_write;
  nbsd_thread_callbacks.proc_lookup = nbsd_thread_proc_lookup;
  nbsd_thread_callbacks.proc_regsize = nbsd_thread_proc_regsize;
  nbsd_thread_callbacks.proc_getregs = nbsd_thread_proc_getregs;
  nbsd_thread_callbacks.proc_setregs = nbsd_thread_proc_setregs;
}

static void
init_nbsd_thread_ops ()
{
  nbsd_thread_ops.to_shortname = "netbsd-threads";
  nbsd_thread_ops.to_longname = "NetBSD pthread.";
  nbsd_thread_ops.to_doc = "NetBSD pthread support.";
  nbsd_thread_ops.to_open = 0;
  nbsd_thread_ops.to_close = 0;
  nbsd_thread_ops.to_attach = nbsd_thread_attach;
  nbsd_thread_ops.to_post_attach = nbsd_thread_post_attach;
  nbsd_thread_ops.to_detach = nbsd_thread_detach;
  nbsd_thread_ops.to_resume = nbsd_thread_resume;
  nbsd_thread_ops.to_wait = nbsd_thread_wait;
  nbsd_thread_ops.to_fetch_registers = nbsd_thread_fetch_registers;
  nbsd_thread_ops.to_store_registers = nbsd_thread_store_registers;
  nbsd_thread_ops.to_prepare_to_store = nbsd_thread_prepare_to_store;
  nbsd_thread_ops.to_xfer_memory = nbsd_thread_xfer_memory;
  nbsd_thread_ops.to_files_info = nbsd_thread_files_info;
  nbsd_thread_ops.to_insert_breakpoint = memory_insert_breakpoint;
  nbsd_thread_ops.to_remove_breakpoint = memory_remove_breakpoint;
  nbsd_thread_ops.to_terminal_init = terminal_init_inferior;
  nbsd_thread_ops.to_terminal_inferior = terminal_inferior;
  nbsd_thread_ops.to_terminal_ours_for_output = terminal_ours_for_output;
  nbsd_thread_ops.to_terminal_ours = terminal_ours;
  nbsd_thread_ops.to_terminal_info = child_terminal_info;
  nbsd_thread_ops.to_kill = nbsd_thread_kill_inferior;
  nbsd_thread_ops.to_load = 0;
  nbsd_thread_ops.to_lookup_symbol = 0;
  nbsd_thread_ops.to_create_inferior = nbsd_thread_create_inferior;
  nbsd_thread_ops.to_has_exited = nbsd_thread_has_exited;
  nbsd_thread_ops.to_mourn_inferior = nbsd_thread_mourn_inferior;
  nbsd_thread_ops.to_can_run = nbsd_thread_can_run;
  nbsd_thread_ops.to_notice_signals = nbsd_thread_notice_signals;
  nbsd_thread_ops.to_thread_alive = nbsd_thread_alive;
  nbsd_thread_ops.to_pid_to_str = nbsd_pid_to_str;
  nbsd_thread_ops.to_find_new_threads = nbsd_find_new_threads;
  nbsd_thread_ops.to_stop = nbsd_thread_stop;
  nbsd_thread_ops.to_stratum = process_stratum;
  nbsd_thread_ops.to_has_all_memory = 1;
  nbsd_thread_ops.to_has_memory = 1;
  nbsd_thread_ops.to_has_stack = 1;
  nbsd_thread_ops.to_has_registers = 1;
  nbsd_thread_ops.to_has_execution = 1;
  nbsd_thread_ops.to_has_thread_control = tc_none;
  nbsd_thread_ops.to_sections = 0;
  nbsd_thread_ops.to_sections_end = 0;
  nbsd_thread_ops.to_magic = OPS_MAGIC;
}

static void
init_nbsd_core_ops ()
{
  nbsd_core_ops.to_shortname = "netbsd-core";
  nbsd_core_ops.to_longname = "NetBSD core pthread.";
  nbsd_core_ops.to_doc = "NetBSD pthread support for core files.";
  nbsd_core_ops.to_open = nbsd_core_open;
  nbsd_core_ops.to_close = nbsd_core_close;
  nbsd_core_ops.to_attach = nbsd_thread_attach;
  nbsd_core_ops.to_post_attach = nbsd_thread_post_attach;
  nbsd_core_ops.to_detach = nbsd_core_detach;
  /* nbsd_core_ops.to_resume  = 0; */
  /* nbsd_core_ops.to_wait  = 0;  */
  nbsd_core_ops.to_fetch_registers = nbsd_thread_fetch_registers;
  /* nbsd_core_ops.to_store_registers  = 0; */
  /* nbsd_core_ops.to_prepare_to_store  = 0; */
  nbsd_core_ops.to_xfer_memory = nbsd_thread_xfer_memory;
  nbsd_core_ops.to_files_info = nbsd_core_files_info;
  nbsd_core_ops.to_insert_breakpoint = ignore;
  nbsd_core_ops.to_remove_breakpoint = ignore;
  /* nbsd_core_ops.to_terminal_init  = 0; */
  /* nbsd_core_ops.to_terminal_inferior  = 0; */
  /* nbsd_core_ops.to_terminal_ours_for_output  = 0; */
  /* nbsd_core_ops.to_terminal_ours  = 0; */
  /* nbsd_core_ops.to_terminal_info  = 0; */
  /* nbsd_core_ops.to_kill  = 0; */
  /* nbsd_core_ops.to_load  = 0; */
  /* nbsd_core_ops.to_lookup_symbol  = 0; */
  nbsd_core_ops.to_create_inferior = nbsd_thread_create_inferior;
  nbsd_core_ops.to_stratum = core_stratum;
  nbsd_core_ops.to_has_all_memory = 0;
  nbsd_core_ops.to_has_memory = 1;
  nbsd_core_ops.to_has_stack = 1;
  nbsd_core_ops.to_has_registers = 1;
  nbsd_core_ops.to_has_execution = 0;
  nbsd_core_ops.to_has_thread_control = tc_none;
  nbsd_core_ops.to_thread_alive = nbsd_thread_alive;
  nbsd_core_ops.to_pid_to_str = nbsd_pid_to_str;
  nbsd_core_ops.to_find_new_threads = nbsd_find_new_threads;
  nbsd_core_ops.to_sections = 0;
  nbsd_core_ops.to_sections_end = 0;
  nbsd_core_ops.to_magic = OPS_MAGIC;
}

/* we suppress the call to add_target of core_ops in corelow because
   if there are two targets in the stratum core_stratum, find_core_target
   won't know which one to return.  see corelow.c for an additonal
   comment on coreops_suppress_target. */
int coreops_suppress_target = 1;

void
_initialize_nbsd_thread ()
{
  static struct cmd_list_element *thread_examine_list = NULL;

  init_nbsd_thread_ops ();
  init_nbsd_core_ops ();
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


  memcpy (&orig_core_ops, &core_ops, sizeof (struct target_ops));
  memcpy (&core_ops, &nbsd_core_ops, sizeof (struct target_ops));
  add_target (&core_ops);

  child_suppress_run = 1;

  /* Hook into new_objfile notification. */
  target_new_objfile_chain = target_new_objfile_hook;
  target_new_objfile_hook  = nbsd_thread_new_objfile;
}
