/* libthread_db assisted debugging support, generic parts.

   Copyright (C) 1999-2014 Free Software Foundation, Inc.

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

#include "gdb_assert.h"
#include <dlfcn.h>
#include "gdb_proc_service.h"
#include "gdb_thread_db.h"
#include "gdb_vecs.h"
#include "bfd.h"
#include "command.h"
#include "exceptions.h"
#include "gdbcmd.h"
#include "gdbthread.h"
#include "inferior.h"
#include "symfile.h"
#include "objfiles.h"
#include "target.h"
#include "regcache.h"
#include "solib.h"
#include "solib-svr4.h"
#include "gdbcore.h"
#include "observer.h"
#include "linux-nat.h"
#include "linux-procfs.h"
#include "linux-osdata.h"
#include "auto-load.h"
#include "cli/cli-utils.h"

#include <signal.h>
#include <ctype.h>

/* GNU/Linux libthread_db support.

   libthread_db is a library, provided along with libpthread.so, which
   exposes the internals of the thread library to a debugger.  It
   allows GDB to find existing threads, new threads as they are
   created, thread IDs (usually, the result of pthread_self), and
   thread-local variables.

   The libthread_db interface originates on Solaris, where it is
   both more powerful and more complicated.  This implementation
   only works for LinuxThreads and NPTL, the two glibc threading
   libraries.  It assumes that each thread is permanently assigned
   to a single light-weight process (LWP).

   libthread_db-specific information is stored in the "private" field
   of struct thread_info.  When the field is NULL we do not yet have
   information about the new thread; this could be temporary (created,
   but the thread library's data structures do not reflect it yet)
   or permanent (created using clone instead of pthread_create).

   Process IDs managed by linux-thread-db.c match those used by
   linux-nat.c: a common PID for all processes, an LWP ID for each
   thread, and no TID.  We save the TID in private.  Keeping it out
   of the ptid_t prevents thread IDs changing when libpthread is
   loaded or unloaded.  */

static char *libthread_db_search_path;

/* Set to non-zero if thread_db auto-loading is enabled
   by the "set auto-load libthread-db" command.  */
static int auto_load_thread_db = 1;

/* "show" command for the auto_load_thread_db configuration variable.  */

static void
show_auto_load_thread_db (struct ui_file *file, int from_tty,
			  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Auto-loading of inferior specific libthread_db "
			    "is %s.\n"),
		    value);
}

static void
set_libthread_db_search_path (char *ignored, int from_tty,
			      struct cmd_list_element *c)
{
  if (*libthread_db_search_path == '\0')
    {
      xfree (libthread_db_search_path);
      libthread_db_search_path = xstrdup (LIBTHREAD_DB_SEARCH_PATH);
    }
}

/* If non-zero, print details of libthread_db processing.  */

static unsigned int libthread_db_debug;

static void
show_libthread_db_debug (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("libthread-db debugging is %s.\n"), value);
}

/* If we're running on GNU/Linux, we must explicitly attach to any new
   threads.  */

/* This module's target vector.  */
static struct target_ops thread_db_ops;

/* Non-zero if we have determined the signals used by the threads
   library.  */
static int thread_signals;
static sigset_t thread_stop_set;
static sigset_t thread_print_set;

struct thread_db_info
{
  struct thread_db_info *next;

  /* Process id this object refers to.  */
  int pid;

  /* Handle from dlopen for libthread_db.so.  */
  void *handle;

  /* Absolute pathname from gdb_realpath to disk file used for dlopen-ing
     HANDLE.  It may be NULL for system library.  */
  char *filename;

  /* Structure that identifies the child process for the
     <proc_service.h> interface.  */
  struct ps_prochandle proc_handle;

  /* Connection to the libthread_db library.  */
  td_thragent_t *thread_agent;

  /* True if we need to apply the workaround for glibc/BZ5983.  When
     we catch a PTRACE_O_TRACEFORK, and go query the child's thread
     list, nptl_db returns the parent's threads in addition to the new
     (single) child thread.  If this flag is set, we do extra work to
     be able to ignore such stale entries.  */
  int need_stale_parent_threads_check;

  /* Location of the thread creation event breakpoint.  The code at
     this location in the child process will be called by the pthread
     library whenever a new thread is created.  By setting a special
     breakpoint at this location, GDB can detect when a new thread is
     created.  We obtain this location via the td_ta_event_addr
     call.  */
  CORE_ADDR td_create_bp_addr;

  /* Location of the thread death event breakpoint.  */
  CORE_ADDR td_death_bp_addr;

  /* Pointers to the libthread_db functions.  */

  td_err_e (*td_init_p) (void);

  td_err_e (*td_ta_new_p) (struct ps_prochandle * ps,
				td_thragent_t **ta);
  td_err_e (*td_ta_map_id2thr_p) (const td_thragent_t *ta, thread_t pt,
				  td_thrhandle_t *__th);
  td_err_e (*td_ta_map_lwp2thr_p) (const td_thragent_t *ta,
				   lwpid_t lwpid, td_thrhandle_t *th);
  td_err_e (*td_ta_thr_iter_p) (const td_thragent_t *ta,
				td_thr_iter_f *callback, void *cbdata_p,
				td_thr_state_e state, int ti_pri,
				sigset_t *ti_sigmask_p,
				unsigned int ti_user_flags);
  td_err_e (*td_ta_event_addr_p) (const td_thragent_t *ta,
				  td_event_e event, td_notify_t *ptr);
  td_err_e (*td_ta_set_event_p) (const td_thragent_t *ta,
				 td_thr_events_t *event);
  td_err_e (*td_ta_clear_event_p) (const td_thragent_t *ta,
				   td_thr_events_t *event);
  td_err_e (*td_ta_event_getmsg_p) (const td_thragent_t *ta,
				    td_event_msg_t *msg);

  td_err_e (*td_thr_validate_p) (const td_thrhandle_t *th);
  td_err_e (*td_thr_get_info_p) (const td_thrhandle_t *th,
				 td_thrinfo_t *infop);
  td_err_e (*td_thr_event_enable_p) (const td_thrhandle_t *th,
				     int event);

  td_err_e (*td_thr_tls_get_addr_p) (const td_thrhandle_t *th,
				     psaddr_t map_address,
				     size_t offset, psaddr_t *address);
};

/* List of known processes using thread_db, and the required
   bookkeeping.  */
struct thread_db_info *thread_db_list;

static void thread_db_find_new_threads_1 (ptid_t ptid);
static void thread_db_find_new_threads_2 (ptid_t ptid, int until_no_new);

/* Add the current inferior to the list of processes using libpthread.
   Return a pointer to the newly allocated object that was added to
   THREAD_DB_LIST.  HANDLE is the handle returned by dlopen'ing
   LIBTHREAD_DB_SO.  */

static struct thread_db_info *
add_thread_db_info (void *handle)
{
  struct thread_db_info *info;

  info = xcalloc (1, sizeof (*info));
  info->pid = ptid_get_pid (inferior_ptid);
  info->handle = handle;

  /* The workaround works by reading from /proc/pid/status, so it is
     disabled for core files.  */
  if (target_has_execution)
    info->need_stale_parent_threads_check = 1;

  info->next = thread_db_list;
  thread_db_list = info;

  return info;
}

/* Return the thread_db_info object representing the bookkeeping
   related to process PID, if any; NULL otherwise.  */

static struct thread_db_info *
get_thread_db_info (int pid)
{
  struct thread_db_info *info;

  for (info = thread_db_list; info; info = info->next)
    if (pid == info->pid)
      return info;

  return NULL;
}

/* When PID has exited or has been detached, we no longer want to keep
   track of it as using libpthread.  Call this function to discard
   thread_db related info related to PID.  Note that this closes
   LIBTHREAD_DB_SO's dlopen'ed handle.  */

static void
delete_thread_db_info (int pid)
{
  struct thread_db_info *info, *info_prev;

  info_prev = NULL;

  for (info = thread_db_list; info; info_prev = info, info = info->next)
    if (pid == info->pid)
      break;

  if (info == NULL)
    return;

  if (info->handle != NULL)
    dlclose (info->handle);

  xfree (info->filename);

  if (info_prev)
    info_prev->next = info->next;
  else
    thread_db_list = info->next;

  xfree (info);
}

/* Prototypes for local functions.  */
static int attach_thread (ptid_t ptid, const td_thrhandle_t *th_p,
			  const td_thrinfo_t *ti_p);
static void detach_thread (ptid_t ptid);


/* Use "struct private_thread_info" to cache thread state.  This is
   a substantial optimization.  */

struct private_thread_info
{
  /* Flag set when we see a TD_DEATH event for this thread.  */
  unsigned int dying:1;

  /* Cached thread state.  */
  td_thrhandle_t th;
  thread_t tid;
};


static char *
thread_db_err_str (td_err_e err)
{
  static char buf[64];

  switch (err)
    {
    case TD_OK:
      return "generic 'call succeeded'";
    case TD_ERR:
      return "generic error";
    case TD_NOTHR:
      return "no thread to satisfy query";
    case TD_NOSV:
      return "no sync handle to satisfy query";
    case TD_NOLWP:
      return "no LWP to satisfy query";
    case TD_BADPH:
      return "invalid process handle";
    case TD_BADTH:
      return "invalid thread handle";
    case TD_BADSH:
      return "invalid synchronization handle";
    case TD_BADTA:
      return "invalid thread agent";
    case TD_BADKEY:
      return "invalid key";
    case TD_NOMSG:
      return "no event message for getmsg";
    case TD_NOFPREGS:
      return "FPU register set not available";
    case TD_NOLIBTHREAD:
      return "application not linked with libthread";
    case TD_NOEVENT:
      return "requested event is not supported";
    case TD_NOCAPAB:
      return "capability not available";
    case TD_DBERR:
      return "debugger service failed";
    case TD_NOAPLIC:
      return "operation not applicable to";
    case TD_NOTSD:
      return "no thread-specific data for this thread";
    case TD_MALLOC:
      return "malloc failed";
    case TD_PARTIALREG:
      return "only part of register set was written/read";
    case TD_NOXREGS:
      return "X register set not available for this thread";
#ifdef THREAD_DB_HAS_TD_NOTALLOC
    case TD_NOTALLOC:
      return "thread has not yet allocated TLS for given module";
#endif
#ifdef THREAD_DB_HAS_TD_VERSION
    case TD_VERSION:
      return "versions of libpthread and libthread_db do not match";
#endif
#ifdef THREAD_DB_HAS_TD_NOTLS
    case TD_NOTLS:
      return "there is no TLS segment in the given module";
#endif
    default:
      snprintf (buf, sizeof (buf), "unknown thread_db error '%d'", err);
      return buf;
    }
}

/* Return 1 if any threads have been registered.  There may be none if
   the threading library is not fully initialized yet.  */

static int
have_threads_callback (struct thread_info *thread, void *args)
{
  int pid = * (int *) args;

  if (ptid_get_pid (thread->ptid) != pid)
    return 0;

  return thread->private != NULL;
}

static int
have_threads (ptid_t ptid)
{
  int pid = ptid_get_pid (ptid);

  return iterate_over_threads (have_threads_callback, &pid) != NULL;
}

struct thread_get_info_inout
{
  struct thread_info *thread_info;
  struct thread_db_info *thread_db_info;
};

/* A callback function for td_ta_thr_iter, which we use to map all
   threads to LWPs.

   THP is a handle to the current thread; if INFOP is not NULL, the
   struct thread_info associated with this thread is returned in
   *INFOP.

   If the thread is a zombie, TD_THR_ZOMBIE is returned.  Otherwise,
   zero is returned to indicate success.  */

static int
thread_get_info_callback (const td_thrhandle_t *thp, void *argp)
{
  td_thrinfo_t ti;
  td_err_e err;
  ptid_t thread_ptid;
  struct thread_get_info_inout *inout;
  struct thread_db_info *info;

  inout = argp;
  info = inout->thread_db_info;

  err = info->td_thr_get_info_p (thp, &ti);
  if (err != TD_OK)
    error (_("thread_get_info_callback: cannot get thread info: %s"),
	   thread_db_err_str (err));

  /* Fill the cache.  */
  thread_ptid = ptid_build (info->pid, ti.ti_lid, 0);
  inout->thread_info = find_thread_ptid (thread_ptid);

  if (inout->thread_info == NULL)
    {
      /* New thread.  Attach to it now (why wait?).  */
      if (!have_threads (thread_ptid))
 	thread_db_find_new_threads_1 (thread_ptid);
      else
	attach_thread (thread_ptid, thp, &ti);
      inout->thread_info = find_thread_ptid (thread_ptid);
      gdb_assert (inout->thread_info != NULL);
    }

  return 0;
}

/* Fetch the user-level thread id of PTID.  */

static void
thread_from_lwp (ptid_t ptid)
{
  td_thrhandle_t th;
  td_err_e err;
  struct thread_db_info *info;
  struct thread_get_info_inout io = {0};

  /* Just in case td_ta_map_lwp2thr doesn't initialize it completely.  */
  th.th_unique = 0;

  /* This ptid comes from linux-nat.c, which should always fill in the
     LWP.  */
  gdb_assert (ptid_get_lwp (ptid) != 0);

  info = get_thread_db_info (ptid_get_pid (ptid));

  /* Access an lwp we know is stopped.  */
  info->proc_handle.ptid = ptid;
  err = info->td_ta_map_lwp2thr_p (info->thread_agent, ptid_get_lwp (ptid),
				   &th);
  if (err != TD_OK)
    error (_("Cannot find user-level thread for LWP %ld: %s"),
	   ptid_get_lwp (ptid), thread_db_err_str (err));

  /* Long-winded way of fetching the thread info.  */
  io.thread_db_info = info;
  io.thread_info = NULL;
  thread_get_info_callback (&th, &io);
}


/* Attach to lwp PTID, doing whatever else is required to have this
   LWP under the debugger's control --- e.g., enabling event
   reporting.  Returns true on success.  */
int
thread_db_attach_lwp (ptid_t ptid)
{
  td_thrhandle_t th;
  td_thrinfo_t ti;
  td_err_e err;
  struct thread_db_info *info;

  info = get_thread_db_info (ptid_get_pid (ptid));

  if (info == NULL)
    return 0;

  /* This ptid comes from linux-nat.c, which should always fill in the
     LWP.  */
  gdb_assert (ptid_get_lwp (ptid) != 0);

  /* Access an lwp we know is stopped.  */
  info->proc_handle.ptid = ptid;

  /* If we have only looked at the first thread before libpthread was
     initialized, we may not know its thread ID yet.  Make sure we do
     before we add another thread to the list.  */
  if (!have_threads (ptid))
    thread_db_find_new_threads_1 (ptid);

  err = info->td_ta_map_lwp2thr_p (info->thread_agent, ptid_get_lwp (ptid),
				   &th);
  if (err != TD_OK)
    /* Cannot find user-level thread.  */
    return 0;

  err = info->td_thr_get_info_p (&th, &ti);
  if (err != TD_OK)
    {
      warning (_("Cannot get thread info: %s"), thread_db_err_str (err));
      return 0;
    }

  attach_thread (ptid, &th, &ti);
  return 1;
}

static void *
verbose_dlsym (void *handle, const char *name)
{
  void *sym = dlsym (handle, name);
  if (sym == NULL)
    warning (_("Symbol \"%s\" not found in libthread_db: %s"),
	     name, dlerror ());
  return sym;
}

static td_err_e
enable_thread_event (int event, CORE_ADDR *bp)
{
  td_notify_t notify;
  td_err_e err;
  struct thread_db_info *info;

  info = get_thread_db_info (ptid_get_pid (inferior_ptid));

  /* Access an lwp we know is stopped.  */
  info->proc_handle.ptid = inferior_ptid;

  /* Get the breakpoint address for thread EVENT.  */
  err = info->td_ta_event_addr_p (info->thread_agent, event, &notify);
  if (err != TD_OK)
    return err;

  /* Set up the breakpoint.  */
  gdb_assert (exec_bfd);
  (*bp) = (gdbarch_convert_from_func_ptr_addr
	   (target_gdbarch (),
	    /* Do proper sign extension for the target.  */
	    (bfd_get_sign_extend_vma (exec_bfd) > 0
	     ? (CORE_ADDR) (intptr_t) notify.u.bptaddr
	     : (CORE_ADDR) (uintptr_t) notify.u.bptaddr),
	    &current_target));
  create_thread_event_breakpoint (target_gdbarch (), *bp);

  return TD_OK;
}

/* Verify inferior's '\0'-terminated symbol VER_SYMBOL starts with "%d.%d" and
   return 1 if this version is lower (and not equal) to
   VER_MAJOR_MIN.VER_MINOR_MIN.  Return 0 in all other cases.  */

static int
inferior_has_bug (const char *ver_symbol, int ver_major_min, int ver_minor_min)
{
  struct minimal_symbol *version_msym;
  CORE_ADDR version_addr;
  char *version;
  int err, got, retval = 0;

  version_msym = lookup_minimal_symbol (ver_symbol, NULL, NULL);
  if (version_msym == NULL)
    return 0;

  version_addr = SYMBOL_VALUE_ADDRESS (version_msym);
  got = target_read_string (version_addr, &version, 32, &err);
  if (err == 0 && memchr (version, 0, got) == &version[got -1])
    {
      int major, minor;

      retval = (sscanf (version, "%d.%d", &major, &minor) == 2
		&& (major < ver_major_min
		    || (major == ver_major_min && minor < ver_minor_min)));
    }
  xfree (version);

  return retval;
}

static void
enable_thread_event_reporting (void)
{
  td_thr_events_t events;
  td_err_e err;
  struct thread_db_info *info;

  info = get_thread_db_info (ptid_get_pid (inferior_ptid));

  /* We cannot use the thread event reporting facility if these
     functions aren't available.  */
  if (info->td_ta_event_addr_p == NULL
      || info->td_ta_set_event_p == NULL
      || info->td_ta_event_getmsg_p == NULL
      || info->td_thr_event_enable_p == NULL)
    return;

  /* Set the process wide mask saying which events we're interested in.  */
  td_event_emptyset (&events);
  td_event_addset (&events, TD_CREATE);

  /* There is a bug fixed between linuxthreads 2.1.3 and 2.2 by
       commit 2e4581e4fba917f1779cd0a010a45698586c190a
       * manager.c (pthread_exited): Correctly report event as TD_REAP
       instead of TD_DEATH.  Fix comments.
     where event reporting facility is broken for TD_DEATH events,
     so don't enable it if we have glibc but a lower version.  */
  if (!inferior_has_bug ("__linuxthreads_version", 2, 2))
    td_event_addset (&events, TD_DEATH);

  err = info->td_ta_set_event_p (info->thread_agent, &events);
  if (err != TD_OK)
    {
      warning (_("Unable to set global thread event mask: %s"),
	       thread_db_err_str (err));
      return;
    }

  /* Delete previous thread event breakpoints, if any.  */
  remove_thread_event_breakpoints ();
  info->td_create_bp_addr = 0;
  info->td_death_bp_addr = 0;

  /* Set up the thread creation event.  */
  err = enable_thread_event (TD_CREATE, &info->td_create_bp_addr);
  if (err != TD_OK)
    {
      warning (_("Unable to get location for thread creation breakpoint: %s"),
	       thread_db_err_str (err));
      return;
    }

  /* Set up the thread death event.  */
  err = enable_thread_event (TD_DEATH, &info->td_death_bp_addr);
  if (err != TD_OK)
    {
      warning (_("Unable to get location for thread death breakpoint: %s"),
	       thread_db_err_str (err));
      return;
    }
}

/* Similar as thread_db_find_new_threads_1, but try to silently ignore errors
   if appropriate.

   Return 1 if the caller should abort libthread_db initialization.  Return 0
   otherwise.  */

static int
thread_db_find_new_threads_silently (ptid_t ptid)
{
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ERROR)
    {
      thread_db_find_new_threads_2 (ptid, 1);
    }

  if (except.reason < 0)
    {
      if (libthread_db_debug)
	exception_fprintf (gdb_stderr, except,
			   "Warning: thread_db_find_new_threads_silently: ");

      /* There is a bug fixed between nptl 2.6.1 and 2.7 by
	   commit 7d9d8bd18906fdd17364f372b160d7ab896ce909
	 where calls to td_thr_get_info fail with TD_ERR for statically linked
	 executables if td_thr_get_info is called before glibc has initialized
	 itself.
	 
	 If the nptl bug is NOT present in the inferior and still thread_db
	 reports an error return 1.  It means the inferior has corrupted thread
	 list and GDB should fall back only to LWPs.

	 If the nptl bug is present in the inferior return 0 to silently ignore
	 such errors, and let gdb enumerate threads again later.  In such case
	 GDB cannot properly display LWPs if the inferior thread list is
	 corrupted.  For core files it does not apply, no 'later enumeration'
	 is possible.  */

      if (!target_has_execution || !inferior_has_bug ("nptl_version", 2, 7))
	{
	  exception_fprintf (gdb_stderr, except,
			     _("Warning: couldn't activate thread debugging "
			       "using libthread_db: "));
	  return 1;
	}
    }
  return 0;
}

/* Lookup a library in which given symbol resides.
   Note: this is looking in GDB process, not in the inferior.
   Returns library name, or NULL.  */

static const char *
dladdr_to_soname (const void *addr)
{
  Dl_info info;

  if (dladdr (addr, &info) != 0)
    return info.dli_fname;
  return NULL;
}

/* Attempt to initialize dlopen()ed libthread_db, described by INFO.
   Return 1 on success.
   Failure could happen if libthread_db does not have symbols we expect,
   or when it refuses to work with the current inferior (e.g. due to
   version mismatch between libthread_db and libpthread).  */

static int
try_thread_db_load_1 (struct thread_db_info *info)
{
  td_err_e err;

  /* Initialize pointers to the dynamic library functions we will use.
     Essential functions first.  */

  info->td_init_p = verbose_dlsym (info->handle, "td_init");
  if (info->td_init_p == NULL)
    return 0;

  err = info->td_init_p ();
  if (err != TD_OK)
    {
      warning (_("Cannot initialize libthread_db: %s"),
	       thread_db_err_str (err));
      return 0;
    }

  info->td_ta_new_p = verbose_dlsym (info->handle, "td_ta_new");
  if (info->td_ta_new_p == NULL)
    return 0;

  /* Initialize the structure that identifies the child process.  */
  info->proc_handle.ptid = inferior_ptid;

  /* Now attempt to open a connection to the thread library.  */
  err = info->td_ta_new_p (&info->proc_handle, &info->thread_agent);
  if (err != TD_OK)
    {
      if (libthread_db_debug)
	printf_unfiltered (_("td_ta_new failed: %s\n"),
			   thread_db_err_str (err));
      else
        switch (err)
          {
            case TD_NOLIBTHREAD:
#ifdef THREAD_DB_HAS_TD_VERSION
            case TD_VERSION:
#endif
              /* The errors above are not unexpected and silently ignored:
                 they just mean we haven't found correct version of
                 libthread_db yet.  */
              break;
            default:
              warning (_("td_ta_new failed: %s"), thread_db_err_str (err));
          }
      return 0;
    }

  info->td_ta_map_id2thr_p = verbose_dlsym (info->handle, "td_ta_map_id2thr");
  if (info->td_ta_map_id2thr_p == NULL)
    return 0;

  info->td_ta_map_lwp2thr_p = verbose_dlsym (info->handle,
					     "td_ta_map_lwp2thr");
  if (info->td_ta_map_lwp2thr_p == NULL)
    return 0;

  info->td_ta_thr_iter_p = verbose_dlsym (info->handle, "td_ta_thr_iter");
  if (info->td_ta_thr_iter_p == NULL)
    return 0;

  info->td_thr_validate_p = verbose_dlsym (info->handle, "td_thr_validate");
  if (info->td_thr_validate_p == NULL)
    return 0;

  info->td_thr_get_info_p = verbose_dlsym (info->handle, "td_thr_get_info");
  if (info->td_thr_get_info_p == NULL)
    return 0;

  /* These are not essential.  */
  info->td_ta_event_addr_p = dlsym (info->handle, "td_ta_event_addr");
  info->td_ta_set_event_p = dlsym (info->handle, "td_ta_set_event");
  info->td_ta_clear_event_p = dlsym (info->handle, "td_ta_clear_event");
  info->td_ta_event_getmsg_p = dlsym (info->handle, "td_ta_event_getmsg");
  info->td_thr_event_enable_p = dlsym (info->handle, "td_thr_event_enable");
  info->td_thr_tls_get_addr_p = dlsym (info->handle, "td_thr_tls_get_addr");

  if (thread_db_find_new_threads_silently (inferior_ptid) != 0)
    {
      /* Even if libthread_db initializes, if the thread list is
         corrupted, we'd not manage to list any threads.  Better reject this
         thread_db, and fall back to at least listing LWPs.  */
      return 0;
    }

  printf_unfiltered (_("[Thread debugging using libthread_db enabled]\n"));

  if (libthread_db_debug || *libthread_db_search_path)
    {
      const char *library;

      library = dladdr_to_soname (*info->td_ta_new_p);
      if (library == NULL)
	library = LIBTHREAD_DB_SO;

      printf_unfiltered (_("Using host libthread_db library \"%s\".\n"),
			 library);
    }

  /* The thread library was detected.  Activate the thread_db target
     if this is the first process using it.  */
  if (thread_db_list->next == NULL)
    push_target (&thread_db_ops);

  /* Enable event reporting, but not when debugging a core file.  */
  if (target_has_execution)
    enable_thread_event_reporting ();

  return 1;
}

/* Attempt to use LIBRARY as libthread_db.  LIBRARY could be absolute,
   relative, or just LIBTHREAD_DB.  */

static int
try_thread_db_load (const char *library, int check_auto_load_safe)
{
  void *handle;
  struct thread_db_info *info;

  if (libthread_db_debug)
    printf_unfiltered (_("Trying host libthread_db library: %s.\n"),
                       library);

  if (check_auto_load_safe)
    {
      if (access (library, R_OK) != 0)
	{
	  /* Do not print warnings by file_is_auto_load_safe if the library does
	     not exist at this place.  */
	  if (libthread_db_debug)
	    printf_unfiltered (_("open failed: %s.\n"), safe_strerror (errno));
	  return 0;
	}

      if (!file_is_auto_load_safe (library, _("auto-load: Loading libthread-db "
					      "library \"%s\" from explicit "
					      "directory.\n"),
				   library))
	return 0;
    }

  handle = dlopen (library, RTLD_NOW);
  if (handle == NULL)
    {
      if (libthread_db_debug)
	printf_unfiltered (_("dlopen failed: %s.\n"), dlerror ());
      return 0;
    }

  if (libthread_db_debug && strchr (library, '/') == NULL)
    {
      void *td_init;

      td_init = dlsym (handle, "td_init");
      if (td_init != NULL)
        {
          const char *const libpath = dladdr_to_soname (td_init);

          if (libpath != NULL)
            printf_unfiltered (_("Host %s resolved to: %s.\n"),
                               library, libpath);
        }
    }

  info = add_thread_db_info (handle);

  /* Do not save system library name, that one is always trusted.  */
  if (strchr (library, '/') != NULL)
    info->filename = gdb_realpath (library);

  if (try_thread_db_load_1 (info))
    return 1;

  /* This library "refused" to work on current inferior.  */
  delete_thread_db_info (ptid_get_pid (inferior_ptid));
  return 0;
}

/* Subroutine of try_thread_db_load_from_pdir to simplify it.
   Try loading libthread_db in directory(OBJ)/SUBDIR.
   SUBDIR may be NULL.  It may also be something like "../lib64".
   The result is true for success.  */

static int
try_thread_db_load_from_pdir_1 (struct objfile *obj, const char *subdir)
{
  struct cleanup *cleanup;
  char *path, *cp;
  int result;
  const char *obj_name = objfile_name (obj);

  if (obj_name[0] != '/')
    {
      warning (_("Expected absolute pathname for libpthread in the"
		 " inferior, but got %s."), obj_name);
      return 0;
    }

  path = xmalloc (strlen (obj_name) + (subdir ? strlen (subdir) + 1 : 0)
		  + 1 + strlen (LIBTHREAD_DB_SO) + 1);
  cleanup = make_cleanup (xfree, path);

  strcpy (path, obj_name);
  cp = strrchr (path, '/');
  /* This should at minimum hit the first character.  */
  gdb_assert (cp != NULL);
  cp[1] = '\0';
  if (subdir != NULL)
    {
      strcat (cp, subdir);
      strcat (cp, "/");
    }
  strcat (cp, LIBTHREAD_DB_SO);

  result = try_thread_db_load (path, 1);

  do_cleanups (cleanup);
  return result;
}

/* Handle $pdir in libthread-db-search-path.
   Look for libthread_db in directory(libpthread)/SUBDIR.
   SUBDIR may be NULL.  It may also be something like "../lib64".
   The result is true for success.  */

static int
try_thread_db_load_from_pdir (const char *subdir)
{
  struct objfile *obj;

  if (!auto_load_thread_db)
    return 0;

  ALL_OBJFILES (obj)
    if (libpthread_name_p (objfile_name (obj)))
      {
	if (try_thread_db_load_from_pdir_1 (obj, subdir))
	  return 1;

	/* We may have found the separate-debug-info version of
	   libpthread, and it may live in a directory without a matching
	   libthread_db.  */
	if (obj->separate_debug_objfile_backlink != NULL)
	  return try_thread_db_load_from_pdir_1 (obj->separate_debug_objfile_backlink,
						 subdir);

	return 0;
      }

  return 0;
}

/* Handle $sdir in libthread-db-search-path.
   Look for libthread_db in the system dirs, or wherever a plain
   dlopen(file_without_path) will look.
   The result is true for success.  */

static int
try_thread_db_load_from_sdir (void)
{
  return try_thread_db_load (LIBTHREAD_DB_SO, 0);
}

/* Try to load libthread_db from directory DIR of length DIR_LEN.
   The result is true for success.  */

static int
try_thread_db_load_from_dir (const char *dir, size_t dir_len)
{
  struct cleanup *cleanup;
  char *path;
  int result;

  if (!auto_load_thread_db)
    return 0;

  path = xmalloc (dir_len + 1 + strlen (LIBTHREAD_DB_SO) + 1);
  cleanup = make_cleanup (xfree, path);

  memcpy (path, dir, dir_len);
  path[dir_len] = '/';
  strcpy (path + dir_len + 1, LIBTHREAD_DB_SO);

  result = try_thread_db_load (path, 1);

  do_cleanups (cleanup);
  return result;
}

/* Search libthread_db_search_path for libthread_db which "agrees"
   to work on current inferior.
   The result is true for success.  */

static int
thread_db_load_search (void)
{
  VEC (char_ptr) *dir_vec;
  struct cleanup *cleanups;
  char *this_dir;
  int i, rc = 0;

  dir_vec = dirnames_to_char_ptr_vec (libthread_db_search_path);
  cleanups = make_cleanup_free_char_ptr_vec (dir_vec);

  for (i = 0; VEC_iterate (char_ptr, dir_vec, i, this_dir); ++i)
    {
      const int pdir_len = sizeof ("$pdir") - 1;
      size_t this_dir_len;

      this_dir_len = strlen (this_dir);

      if (strncmp (this_dir, "$pdir", pdir_len) == 0
	  && (this_dir[pdir_len] == '\0'
	      || this_dir[pdir_len] == '/'))
	{
	  char *subdir = NULL;
	  struct cleanup *free_subdir_cleanup
	    = make_cleanup (null_cleanup, NULL);

	  if (this_dir[pdir_len] == '/')
	    {
	      subdir = xmalloc (strlen (this_dir));
	      make_cleanup (xfree, subdir);
	      strcpy (subdir, this_dir + pdir_len + 1);
	    }
	  rc = try_thread_db_load_from_pdir (subdir);
	  do_cleanups (free_subdir_cleanup);
	  if (rc)
	    break;
	}
      else if (strcmp (this_dir, "$sdir") == 0)
	{
	  if (try_thread_db_load_from_sdir ())
	    {
	      rc = 1;
	      break;
	    }
	}
      else
	{
	  if (try_thread_db_load_from_dir (this_dir, this_dir_len))
	    {
	      rc = 1;
	      break;
	    }
	}
    }

  do_cleanups (cleanups);
  if (libthread_db_debug)
    printf_unfiltered (_("thread_db_load_search returning %d\n"), rc);
  return rc;
}

/* Return non-zero if the inferior has a libpthread.  */

static int
has_libpthread (void)
{
  struct objfile *obj;

  ALL_OBJFILES (obj)
    if (libpthread_name_p (objfile_name (obj)))
      return 1;

  return 0;
}

/* Attempt to load and initialize libthread_db.
   Return 1 on success.  */

static int
thread_db_load (void)
{
  struct thread_db_info *info;

  info = get_thread_db_info (ptid_get_pid (inferior_ptid));

  if (info != NULL)
    return 1;

  /* Don't attempt to use thread_db on executables not running
     yet.  */
  if (!target_has_registers)
    return 0;

  /* Don't attempt to use thread_db for remote targets.  */
  if (!(target_can_run (&current_target) || core_bfd))
    return 0;

  if (thread_db_load_search ())
    return 1;

  /* We couldn't find a libthread_db.
     If the inferior has a libpthread warn the user.  */
  if (has_libpthread ())
    {
      warning (_("Unable to find libthread_db matching inferior's thread"
		 " library, thread debugging will not be available."));
      return 0;
    }

  /* Either this executable isn't using libpthread at all, or it is
     statically linked.  Since we can't easily distinguish these two cases,
     no warning is issued.  */
  return 0;
}

static void
disable_thread_event_reporting (struct thread_db_info *info)
{
  if (info->td_ta_clear_event_p != NULL)
    {
      td_thr_events_t events;

      /* Set the process wide mask saying we aren't interested in any
	 events anymore.  */
      td_event_fillset (&events);
      info->td_ta_clear_event_p (info->thread_agent, &events);
    }

  info->td_create_bp_addr = 0;
  info->td_death_bp_addr = 0;
}

static void
check_thread_signals (void)
{
  if (!thread_signals)
    {
      sigset_t mask;
      int i;

      lin_thread_get_thread_signals (&mask);
      sigemptyset (&thread_stop_set);
      sigemptyset (&thread_print_set);

      for (i = 1; i < NSIG; i++)
	{
	  if (sigismember (&mask, i))
	    {
	      if (signal_stop_update (gdb_signal_from_host (i), 0))
		sigaddset (&thread_stop_set, i);
	      if (signal_print_update (gdb_signal_from_host (i), 0))
		sigaddset (&thread_print_set, i);
	      thread_signals = 1;
	    }
	}
    }
}

/* Check whether thread_db is usable.  This function is called when
   an inferior is created (or otherwise acquired, e.g. attached to)
   and when new shared libraries are loaded into a running process.  */

void
check_for_thread_db (void)
{
  /* Do nothing if we couldn't load libthread_db.so.1.  */
  if (!thread_db_load ())
    return;
}

/* This function is called via the new_objfile observer.  */

static void
thread_db_new_objfile (struct objfile *objfile)
{
  /* This observer must always be called with inferior_ptid set
     correctly.  */

  if (objfile != NULL
      /* libpthread with separate debug info has its debug info file already
	 loaded (and notified without successful thread_db initialization)
	 the time observer_notify_new_objfile is called for the library itself.
	 Static executables have their separate debug info loaded already
	 before the inferior has started.  */
      && objfile->separate_debug_objfile_backlink == NULL
      /* Only check for thread_db if we loaded libpthread,
	 or if this is the main symbol file.
	 We need to check OBJF_MAINLINE to handle the case of debugging
	 a statically linked executable AND the symbol file is specified AFTER
	 the exec file is loaded (e.g., gdb -c core ; file foo).
	 For dynamically linked executables, libpthread can be near the end
	 of the list of shared libraries to load, and in an app of several
	 thousand shared libraries, this can otherwise be painful.  */
      && ((objfile->flags & OBJF_MAINLINE) != 0
	  || libpthread_name_p (objfile_name (objfile))))
    check_for_thread_db ();
}

/* This function is called via the inferior_created observer.
   This handles the case of debugging statically linked executables.  */

static void
thread_db_inferior_created (struct target_ops *target, int from_tty)
{
  check_for_thread_db ();
}

/* Attach to a new thread.  This function is called when we receive a
   TD_CREATE event or when we iterate over all threads and find one
   that wasn't already in our list.  Returns true on success.  */

static int
attach_thread (ptid_t ptid, const td_thrhandle_t *th_p,
	       const td_thrinfo_t *ti_p)
{
  struct private_thread_info *private;
  struct thread_info *tp;
  td_err_e err;
  struct thread_db_info *info;

  /* If we're being called after a TD_CREATE event, we may already
     know about this thread.  There are two ways this can happen.  We
     may have iterated over all threads between the thread creation
     and the TD_CREATE event, for instance when the user has issued
     the `info threads' command before the SIGTRAP for hitting the
     thread creation breakpoint was reported.  Alternatively, the
     thread may have exited and a new one been created with the same
     thread ID.  In the first case we don't need to do anything; in
     the second case we should discard information about the dead
     thread and attach to the new one.  */
  tp = find_thread_ptid (ptid);
  if (tp != NULL)
    {
      /* If tp->private is NULL, then GDB is already attached to this
	 thread, but we do not know anything about it.  We can learn
	 about it here.  This can only happen if we have some other
	 way besides libthread_db to notice new threads (i.e.
	 PTRACE_EVENT_CLONE); assume the same mechanism notices thread
	 exit, so this can not be a stale thread recreated with the
	 same ID.  */
      if (tp->private != NULL)
	{
	  if (!tp->private->dying)
	    return 0;

	  delete_thread (ptid);
	  tp = NULL;
	}
    }

  if (target_has_execution)
    check_thread_signals ();

  /* Under GNU/Linux, we have to attach to each and every thread.  */
  if (target_has_execution
      && tp == NULL)
    {
      int res;

      res = lin_lwp_attach_lwp (ptid_build (ptid_get_pid (ptid),
					    ti_p->ti_lid, 0));
      if (res < 0)
	{
	  /* Error, stop iterating.  */
	  return 0;
	}
      else if (res > 0)
	{
	  /* Pretend this thread doesn't exist yet, and keep
	     iterating.  */
	  return 1;
	}

      /* Otherwise, we sucessfully attached to the thread.  */
    }

  /* Construct the thread's private data.  */
  private = xmalloc (sizeof (struct private_thread_info));
  memset (private, 0, sizeof (struct private_thread_info));

  /* A thread ID of zero may mean the thread library has not initialized
     yet.  But we shouldn't even get here if that's the case.  FIXME:
     if we change GDB to always have at least one thread in the thread
     list this will have to go somewhere else; maybe private == NULL
     until the thread_db target claims it.  */
  gdb_assert (ti_p->ti_tid != 0);
  private->th = *th_p;
  private->tid = ti_p->ti_tid;
  if (ti_p->ti_state == TD_THR_UNKNOWN || ti_p->ti_state == TD_THR_ZOMBIE)
    private->dying = 1;

  /* Add the thread to GDB's thread list.  */
  if (tp == NULL)
    add_thread_with_info (ptid, private);
  else
    tp->private = private;

  info = get_thread_db_info (ptid_get_pid (ptid));

  /* Enable thread event reporting for this thread, except when
     debugging a core file.  */
  if (target_has_execution)
    {
      err = info->td_thr_event_enable_p (th_p, 1);
      if (err != TD_OK)
	error (_("Cannot enable thread event reporting for %s: %s"),
	       target_pid_to_str (ptid), thread_db_err_str (err));
    }

  return 1;
}

static void
detach_thread (ptid_t ptid)
{
  struct thread_info *thread_info;

  /* Don't delete the thread now, because it still reports as active
     until it has executed a few instructions after the event
     breakpoint - if we deleted it now, "info threads" would cause us
     to re-attach to it.  Just mark it as having had a TD_DEATH
     event.  This means that we won't delete it from our thread list
     until we notice that it's dead (via prune_threads), or until
     something re-uses its thread ID.  We'll report the thread exit
     when the underlying LWP dies.  */
  thread_info = find_thread_ptid (ptid);
  gdb_assert (thread_info != NULL && thread_info->private != NULL);
  thread_info->private->dying = 1;
}

static void
thread_db_detach (struct target_ops *ops, const char *args, int from_tty)
{
  struct target_ops *target_beneath = find_target_beneath (ops);
  struct thread_db_info *info;

  info = get_thread_db_info (ptid_get_pid (inferior_ptid));

  if (info)
    {
      if (target_has_execution)
	{
	  disable_thread_event_reporting (info);

	  /* Delete the old thread event breakpoints.  Note that
	     unlike when mourning, we can remove them here because
	     there's still a live inferior to poke at.  In any case,
	     GDB will not try to insert anything in the inferior when
	     removing a breakpoint.  */
	  remove_thread_event_breakpoints ();
	}

      delete_thread_db_info (ptid_get_pid (inferior_ptid));
    }

  target_beneath->to_detach (target_beneath, args, from_tty);

  /* NOTE: From this point on, inferior_ptid is null_ptid.  */

  /* If there are no more processes using libpthread, detach the
     thread_db target ops.  */
  if (!thread_db_list)
    unpush_target (&thread_db_ops);
}

/* Check if PID is currently stopped at the location of a thread event
   breakpoint location.  If it is, read the event message and act upon
   the event.  */

static void
check_event (ptid_t ptid)
{
  struct regcache *regcache = get_thread_regcache (ptid);
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  td_event_msg_t msg;
  td_thrinfo_t ti;
  td_err_e err;
  CORE_ADDR stop_pc;
  int loop = 0;
  struct thread_db_info *info;

  info = get_thread_db_info (ptid_get_pid (ptid));

  /* Bail out early if we're not at a thread event breakpoint.  */
  stop_pc = regcache_read_pc (regcache)
	    - gdbarch_decr_pc_after_break (gdbarch);
  if (stop_pc != info->td_create_bp_addr
      && stop_pc != info->td_death_bp_addr)
    return;

  /* Access an lwp we know is stopped.  */
  info->proc_handle.ptid = ptid;

  /* If we have only looked at the first thread before libpthread was
     initialized, we may not know its thread ID yet.  Make sure we do
     before we add another thread to the list.  */
  if (!have_threads (ptid))
    thread_db_find_new_threads_1 (ptid);

  /* If we are at a create breakpoint, we do not know what new lwp
     was created and cannot specifically locate the event message for it.
     We have to call td_ta_event_getmsg() to get
     the latest message.  Since we have no way of correlating whether
     the event message we get back corresponds to our breakpoint, we must
     loop and read all event messages, processing them appropriately.
     This guarantees we will process the correct message before continuing
     from the breakpoint.

     Currently, death events are not enabled.  If they are enabled,
     the death event can use the td_thr_event_getmsg() interface to
     get the message specifically for that lwp and avoid looping
     below.  */

  loop = 1;

  do
    {
      err = info->td_ta_event_getmsg_p (info->thread_agent, &msg);
      if (err != TD_OK)
	{
	  if (err == TD_NOMSG)
	    return;

	  error (_("Cannot get thread event message: %s"),
		 thread_db_err_str (err));
	}

      err = info->td_thr_get_info_p (msg.th_p, &ti);
      if (err != TD_OK)
	error (_("Cannot get thread info: %s"), thread_db_err_str (err));

      ptid = ptid_build (ptid_get_pid (ptid), ti.ti_lid, 0);

      switch (msg.event)
	{
	case TD_CREATE:
	  /* Call attach_thread whether or not we already know about a
	     thread with this thread ID.  */
	  attach_thread (ptid, msg.th_p, &ti);

	  break;

	case TD_DEATH:

	  if (!in_thread_list (ptid))
	    error (_("Spurious thread death event."));

	  detach_thread (ptid);

	  break;

	default:
	  error (_("Spurious thread event."));
	}
    }
  while (loop);
}

static ptid_t
thread_db_wait (struct target_ops *ops,
		ptid_t ptid, struct target_waitstatus *ourstatus,
		int options)
{
  struct thread_db_info *info;
  struct target_ops *beneath = find_target_beneath (ops);

  ptid = beneath->to_wait (beneath, ptid, ourstatus, options);

  if (ourstatus->kind == TARGET_WAITKIND_IGNORE)
    return ptid;

  if (ourstatus->kind == TARGET_WAITKIND_EXITED
      || ourstatus->kind == TARGET_WAITKIND_SIGNALLED)
    return ptid;

  info = get_thread_db_info (ptid_get_pid (ptid));

  /* If this process isn't using thread_db, we're done.  */
  if (info == NULL)
    return ptid;

  if (ourstatus->kind == TARGET_WAITKIND_EXECD)
    {
      /* New image, it may or may not end up using thread_db.  Assume
	 not unless we find otherwise.  */
      delete_thread_db_info (ptid_get_pid (ptid));
      if (!thread_db_list)
 	unpush_target (&thread_db_ops);

      /* Thread event breakpoints are deleted by
	 update_breakpoints_after_exec.  */

      return ptid;
    }

  /* If we do not know about the main thread yet, this would be a good time to
     find it.  */
  if (ourstatus->kind == TARGET_WAITKIND_STOPPED && !have_threads (ptid))
    thread_db_find_new_threads_1 (ptid);

  if (ourstatus->kind == TARGET_WAITKIND_STOPPED
      && ourstatus->value.sig == GDB_SIGNAL_TRAP)
    /* Check for a thread event.  */
    check_event (ptid);

  if (have_threads (ptid))
    {
      /* Fill in the thread's user-level thread id.  */
      thread_from_lwp (ptid);
    }

  return ptid;
}

static void
thread_db_mourn_inferior (struct target_ops *ops)
{
  struct target_ops *target_beneath = find_target_beneath (ops);

  delete_thread_db_info (ptid_get_pid (inferior_ptid));

  target_beneath->to_mourn_inferior (target_beneath);

  /* Delete the old thread event breakpoints.  Do this after mourning
     the inferior, so that we don't try to uninsert them.  */
  remove_thread_event_breakpoints ();

  /* Detach thread_db target ops.  */
  if (!thread_db_list)
    unpush_target (ops);
}

struct callback_data
{
  struct thread_db_info *info;
  int new_threads;
};

static int
find_new_threads_callback (const td_thrhandle_t *th_p, void *data)
{
  td_thrinfo_t ti;
  td_err_e err;
  ptid_t ptid;
  struct thread_info *tp;
  struct callback_data *cb_data = data;
  struct thread_db_info *info = cb_data->info;

  err = info->td_thr_get_info_p (th_p, &ti);
  if (err != TD_OK)
    error (_("find_new_threads_callback: cannot get thread info: %s"),
	   thread_db_err_str (err));

  if (ti.ti_tid == 0)
    {
      /* A thread ID of zero means that this is the main thread, but
	 glibc has not yet initialized thread-local storage and the
	 pthread library.  We do not know what the thread's TID will
	 be yet.  Just enable event reporting and otherwise ignore
	 it.  */

      /* In that case, we're not stopped in a fork syscall and don't
	 need this glibc bug workaround.  */
      info->need_stale_parent_threads_check = 0;

      if (target_has_execution)
	{
	  err = info->td_thr_event_enable_p (th_p, 1);
	  if (err != TD_OK)
	    error (_("Cannot enable thread event reporting for LWP %d: %s"),
		   (int) ti.ti_lid, thread_db_err_str (err));
	}

      return 0;
    }

  /* Ignore stale parent threads, caused by glibc/BZ5983.  This is a
     bit expensive, as it needs to open /proc/pid/status, so try to
     avoid doing the work if we know we don't have to.  */
  if (info->need_stale_parent_threads_check)
    {
      int tgid = linux_proc_get_tgid (ti.ti_lid);

      if (tgid != -1 && tgid != info->pid)
	return 0;
    }

  ptid = ptid_build (info->pid, ti.ti_lid, 0);
  tp = find_thread_ptid (ptid);
  if (tp == NULL || tp->private == NULL)
    {
      if (attach_thread (ptid, th_p, &ti))
	cb_data->new_threads += 1;
      else
	/* Problem attaching this thread; perhaps it exited before we
	   could attach it?
	   This could mean that the thread list inside glibc itself is in
	   inconsistent state, and libthread_db could go on looping forever
	   (observed with glibc-2.3.6).  To prevent that, terminate
	   iteration: thread_db_find_new_threads_2 will retry.  */
	return 1;
    }

  return 0;
}

/* Helper for thread_db_find_new_threads_2.
   Returns number of new threads found.  */

static int
find_new_threads_once (struct thread_db_info *info, int iteration,
		       td_err_e *errp)
{
  volatile struct gdb_exception except;
  struct callback_data data;
  td_err_e err = TD_ERR;

  data.info = info;
  data.new_threads = 0;

  TRY_CATCH (except, RETURN_MASK_ERROR)
    {
      /* Iterate over all user-space threads to discover new threads.  */
      err = info->td_ta_thr_iter_p (info->thread_agent,
				    find_new_threads_callback,
				    &data,
				    TD_THR_ANY_STATE,
				    TD_THR_LOWEST_PRIORITY,
				    TD_SIGNO_MASK,
				    TD_THR_ANY_USER_FLAGS);
    }

  if (libthread_db_debug)
    {
      if (except.reason < 0)
	exception_fprintf (gdb_stderr, except,
			   "Warning: find_new_threads_once: ");

      printf_filtered (_("Found %d new threads in iteration %d.\n"),
		       data.new_threads, iteration);
    }

  if (errp != NULL)
    *errp = err;

  return data.new_threads;
}

/* Search for new threads, accessing memory through stopped thread
   PTID.  If UNTIL_NO_NEW is true, repeat searching until several
   searches in a row do not discover any new threads.  */

static void
thread_db_find_new_threads_2 (ptid_t ptid, int until_no_new)
{
  td_err_e err = TD_OK;
  struct thread_db_info *info;
  int i, loop;

  info = get_thread_db_info (ptid_get_pid (ptid));

  /* Access an lwp we know is stopped.  */
  info->proc_handle.ptid = ptid;

  if (until_no_new)
    {
      /* Require 4 successive iterations which do not find any new threads.
	 The 4 is a heuristic: there is an inherent race here, and I have
	 seen that 2 iterations in a row are not always sufficient to
	 "capture" all threads.  */
      for (i = 0, loop = 0; loop < 4 && err == TD_OK; ++i, ++loop)
	if (find_new_threads_once (info, i, &err) != 0)
	  {
	    /* Found some new threads.  Restart the loop from beginning.  */
	    loop = -1;
	  }
    }
  else
    find_new_threads_once (info, 0, &err);

  if (err != TD_OK)
    error (_("Cannot find new threads: %s"), thread_db_err_str (err));
}

static void
thread_db_find_new_threads_1 (ptid_t ptid)
{
  thread_db_find_new_threads_2 (ptid, 0);
}

static int
update_thread_core (struct lwp_info *info, void *closure)
{
  info->core = linux_common_core_of_thread (info->ptid);
  return 0;
}

static void
thread_db_find_new_threads (struct target_ops *ops)
{
  struct thread_db_info *info;
  struct inferior *inf;

  ALL_INFERIORS (inf)
    {
      struct thread_info *thread;

      if (inf->pid == 0)
	continue;

      info = get_thread_db_info (inf->pid);
      if (info == NULL)
	continue;

      thread = any_live_thread_of_process (inf->pid);
      if (thread == NULL || thread->executing)
	continue;

      thread_db_find_new_threads_1 (thread->ptid);
    }

  if (target_has_execution)
    iterate_over_lwps (minus_one_ptid /* iterate over all */,
		       update_thread_core, NULL);
}

static char *
thread_db_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  struct thread_info *thread_info = find_thread_ptid (ptid);
  struct target_ops *beneath;

  if (thread_info != NULL && thread_info->private != NULL)
    {
      static char buf[64];
      thread_t tid;

      tid = thread_info->private->tid;
      snprintf (buf, sizeof (buf), "Thread 0x%lx (LWP %ld)",
		tid, ptid_get_lwp (ptid));

      return buf;
    }

  beneath = find_target_beneath (ops);
  if (beneath->to_pid_to_str (beneath, ptid))
    return beneath->to_pid_to_str (beneath, ptid);

  return normal_pid_to_str (ptid);
}

/* Return a string describing the state of the thread specified by
   INFO.  */

static char *
thread_db_extra_thread_info (struct thread_info *info)
{
  if (info->private == NULL)
    return NULL;

  if (info->private->dying)
    return "Exiting";

  return NULL;
}

/* Get the address of the thread local variable in load module LM which
   is stored at OFFSET within the thread local storage for thread PTID.  */

static CORE_ADDR
thread_db_get_thread_local_address (struct target_ops *ops,
				    ptid_t ptid,
				    CORE_ADDR lm,
				    CORE_ADDR offset)
{
  struct thread_info *thread_info;
  struct target_ops *beneath;

  /* If we have not discovered any threads yet, check now.  */
  if (!have_threads (ptid))
    thread_db_find_new_threads_1 (ptid);

  /* Find the matching thread.  */
  thread_info = find_thread_ptid (ptid);

  if (thread_info != NULL && thread_info->private != NULL)
    {
      td_err_e err;
      psaddr_t address;
      struct thread_db_info *info;

      info = get_thread_db_info (ptid_get_pid (ptid));

      /* glibc doesn't provide the needed interface.  */
      if (!info->td_thr_tls_get_addr_p)
	throw_error (TLS_NO_LIBRARY_SUPPORT_ERROR,
		     _("No TLS library support"));

      /* Caller should have verified that lm != 0.  */
      gdb_assert (lm != 0);

      /* Finally, get the address of the variable.  */
      /* Note the cast through uintptr_t: this interface only works if
	 a target address fits in a psaddr_t, which is a host pointer.
	 So a 32-bit debugger can not access 64-bit TLS through this.  */
      err = info->td_thr_tls_get_addr_p (&thread_info->private->th,
					 (psaddr_t)(uintptr_t) lm,
					 offset, &address);

#ifdef THREAD_DB_HAS_TD_NOTALLOC
      /* The memory hasn't been allocated, yet.  */
      if (err == TD_NOTALLOC)
	  /* Now, if libthread_db provided the initialization image's
	     address, we *could* try to build a non-lvalue value from
	     the initialization image.  */
        throw_error (TLS_NOT_ALLOCATED_YET_ERROR,
                     _("TLS not allocated yet"));
#endif

      /* Something else went wrong.  */
      if (err != TD_OK)
        throw_error (TLS_GENERIC_ERROR,
                     (("%s")), thread_db_err_str (err));

      /* Cast assuming host == target.  Joy.  */
      /* Do proper sign extension for the target.  */
      gdb_assert (exec_bfd);
      return (bfd_get_sign_extend_vma (exec_bfd) > 0
	      ? (CORE_ADDR) (intptr_t) address
	      : (CORE_ADDR) (uintptr_t) address);
    }

  beneath = find_target_beneath (ops);
  if (beneath->to_get_thread_local_address)
    return beneath->to_get_thread_local_address (beneath, ptid, lm, offset);
  else
    throw_error (TLS_GENERIC_ERROR,
	         _("TLS not supported on this target"));
}

/* Callback routine used to find a thread based on the TID part of
   its PTID.  */

static int
thread_db_find_thread_from_tid (struct thread_info *thread, void *data)
{
  long *tid = (long *) data;

  if (thread->private->tid == *tid)
    return 1;

  return 0;
}

/* Implement the to_get_ada_task_ptid target method for this target.  */

static ptid_t
thread_db_get_ada_task_ptid (long lwp, long thread)
{
  struct thread_info *thread_info;

  thread_db_find_new_threads_1 (inferior_ptid);
  thread_info = iterate_over_threads (thread_db_find_thread_from_tid, &thread);

  gdb_assert (thread_info != NULL);

  return (thread_info->ptid);
}

static void
thread_db_resume (struct target_ops *ops,
		  ptid_t ptid, int step, enum gdb_signal signo)
{
  struct target_ops *beneath = find_target_beneath (ops);
  struct thread_db_info *info;

  if (ptid_equal (ptid, minus_one_ptid))
    info = get_thread_db_info (ptid_get_pid (inferior_ptid));
  else
    info = get_thread_db_info (ptid_get_pid (ptid));

  /* This workaround is only needed for child fork lwps stopped in a
     PTRACE_O_TRACEFORK event.  When the inferior is resumed, the
     workaround can be disabled.  */
  if (info)
    info->need_stale_parent_threads_check = 0;

  beneath->to_resume (beneath, ptid, step, signo);
}

/* qsort helper function for info_auto_load_libthread_db, sort the
   thread_db_info pointers primarily by their FILENAME and secondarily by their
   PID, both in ascending order.  */

static int
info_auto_load_libthread_db_compare (const void *ap, const void *bp)
{
  struct thread_db_info *a = *(struct thread_db_info **) ap;
  struct thread_db_info *b = *(struct thread_db_info **) bp;
  int retval;

  retval = strcmp (a->filename, b->filename);
  if (retval)
    return retval;

  return (a->pid > b->pid) - (a->pid - b->pid);
}

/* Implement 'info auto-load libthread-db'.  */

static void
info_auto_load_libthread_db (char *args, int from_tty)
{
  struct ui_out *uiout = current_uiout;
  const char *cs = args ? args : "";
  struct thread_db_info *info, **array;
  unsigned info_count, unique_filenames;
  size_t max_filename_len, max_pids_len, pids_len;
  struct cleanup *back_to;
  char *pids;
  int i;

  cs = skip_spaces_const (cs);
  if (*cs)
    error (_("'info auto-load libthread-db' does not accept any parameters"));

  info_count = 0;
  for (info = thread_db_list; info; info = info->next)
    if (info->filename != NULL)
      info_count++;

  array = xmalloc (sizeof (*array) * info_count);
  back_to = make_cleanup (xfree, array);

  info_count = 0;
  for (info = thread_db_list; info; info = info->next)
    if (info->filename != NULL)
      array[info_count++] = info;

  /* Sort ARRAY by filenames and PIDs.  */

  qsort (array, info_count, sizeof (*array),
	 info_auto_load_libthread_db_compare);

  /* Calculate the number of unique filenames (rows) and the maximum string
     length of PIDs list for the unique filenames (columns).  */

  unique_filenames = 0;
  max_filename_len = 0;
  max_pids_len = 0;
  pids_len = 0;
  for (i = 0; i < info_count; i++)
    {
      int pid = array[i]->pid;
      size_t this_pid_len;

      for (this_pid_len = 0; pid != 0; pid /= 10)
	this_pid_len++;

      if (i == 0 || strcmp (array[i - 1]->filename, array[i]->filename) != 0)
	{
	  unique_filenames++;
	  max_filename_len = max (max_filename_len,
				  strlen (array[i]->filename));

	  if (i > 0)
	    {
	      pids_len -= strlen (", ");
	      max_pids_len = max (max_pids_len, pids_len);
	    }
	  pids_len = 0;
	}
      pids_len += this_pid_len + strlen (", ");
    }
  if (i)
    {
      pids_len -= strlen (", ");
      max_pids_len = max (max_pids_len, pids_len);
    }

  /* Table header shifted right by preceding "libthread-db:  " would not match
     its columns.  */
  if (info_count > 0 && args == auto_load_info_scripts_pattern_nl)
    ui_out_text (uiout, "\n");

  make_cleanup_ui_out_table_begin_end (uiout, 2, unique_filenames,
				       "LinuxThreadDbTable");

  ui_out_table_header (uiout, max_filename_len, ui_left, "filename",
		       "Filename");
  ui_out_table_header (uiout, pids_len, ui_left, "PIDs", "Pids");
  ui_out_table_body (uiout);

  pids = xmalloc (max_pids_len + 1);
  make_cleanup (xfree, pids);

  /* Note I is incremented inside the cycle, not at its end.  */
  for (i = 0; i < info_count;)
    {
      struct cleanup *chain = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
      char *pids_end;

      info = array[i];
      ui_out_field_string (uiout, "filename", info->filename);
      pids_end = pids;

      while (i < info_count && strcmp (info->filename, array[i]->filename) == 0)
	{
	  if (pids_end != pids)
	    {
	      *pids_end++ = ',';
	      *pids_end++ = ' ';
	    }
	  pids_end += xsnprintf (pids_end, &pids[max_pids_len + 1] - pids_end,
				 "%u", array[i]->pid);
	  gdb_assert (pids_end < &pids[max_pids_len + 1]);

	  i++;
	}
      *pids_end = '\0';

      ui_out_field_string (uiout, "pids", pids);

      ui_out_text (uiout, "\n");
      do_cleanups (chain);
    }

  do_cleanups (back_to);

  if (info_count == 0)
    ui_out_message (uiout, 0, _("No auto-loaded libthread-db.\n"));
}

static void
init_thread_db_ops (void)
{
  thread_db_ops.to_shortname = "multi-thread";
  thread_db_ops.to_longname = "multi-threaded child process.";
  thread_db_ops.to_doc = "Threads and pthreads support.";
  thread_db_ops.to_detach = thread_db_detach;
  thread_db_ops.to_wait = thread_db_wait;
  thread_db_ops.to_resume = thread_db_resume;
  thread_db_ops.to_mourn_inferior = thread_db_mourn_inferior;
  thread_db_ops.to_find_new_threads = thread_db_find_new_threads;
  thread_db_ops.to_pid_to_str = thread_db_pid_to_str;
  thread_db_ops.to_stratum = thread_stratum;
  thread_db_ops.to_has_thread_control = tc_schedlock;
  thread_db_ops.to_get_thread_local_address
    = thread_db_get_thread_local_address;
  thread_db_ops.to_extra_thread_info = thread_db_extra_thread_info;
  thread_db_ops.to_get_ada_task_ptid = thread_db_get_ada_task_ptid;
  thread_db_ops.to_magic = OPS_MAGIC;

  complete_target_initialization (&thread_db_ops);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_thread_db;

void
_initialize_thread_db (void)
{
  init_thread_db_ops ();

  /* Defer loading of libthread_db.so until inferior is running.
     This allows gdb to load correct libthread_db for a given
     executable -- there could be mutiple versions of glibc,
     compiled with LinuxThreads or NPTL, and until there is
     a running inferior, we can't tell which libthread_db is
     the correct one to load.  */

  libthread_db_search_path = xstrdup (LIBTHREAD_DB_SEARCH_PATH);

  add_setshow_optional_filename_cmd ("libthread-db-search-path",
				     class_support,
				     &libthread_db_search_path, _("\
Set search path for libthread_db."), _("\
Show the current search path or libthread_db."), _("\
This path is used to search for libthread_db to be loaded into \
gdb itself.\n\
Its value is a colon (':') separate list of directories to search.\n\
Setting the search path to an empty list resets it to its default value."),
			    set_libthread_db_search_path,
			    NULL,
			    &setlist, &showlist);

  add_setshow_zuinteger_cmd ("libthread-db", class_maintenance,
			     &libthread_db_debug, _("\
Set libthread-db debugging."), _("\
Show libthread-db debugging."), _("\
When non-zero, libthread-db debugging is enabled."),
			     NULL,
			     show_libthread_db_debug,
			     &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("libthread-db", class_support,
			   &auto_load_thread_db, _("\
Enable or disable auto-loading of inferior specific libthread_db."), _("\
Show whether auto-loading inferior specific libthread_db is enabled."), _("\
If enabled, libthread_db will be searched in 'set libthread-db-search-path'\n\
locations to load libthread_db compatible with the inferior.\n\
Standard system libthread_db still gets loaded even with this option off.\n\
This options has security implications for untrusted inferiors."),
			   NULL, show_auto_load_thread_db,
			   auto_load_set_cmdlist_get (),
			   auto_load_show_cmdlist_get ());

  add_cmd ("libthread-db", class_info, info_auto_load_libthread_db,
	   _("Print the list of loaded inferior specific libthread_db.\n\
Usage: info auto-load libthread-db"),
	   auto_load_info_cmdlist_get ());

  /* Add ourselves to objfile event chain.  */
  observer_attach_new_objfile (thread_db_new_objfile);

  /* Add ourselves to inferior_created event chain.
     This is needed to handle debugging statically linked programs where
     the new_objfile observer won't get called for libpthread.  */
  observer_attach_inferior_created (thread_db_inferior_created);
}
