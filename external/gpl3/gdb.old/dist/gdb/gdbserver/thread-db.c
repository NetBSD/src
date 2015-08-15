/* Thread management interface, for the remote server for GDB.
   Copyright (C) 2002-2014 Free Software Foundation, Inc.

   Contributed by MontaVista Software.

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

#include "server.h"

#include "linux-low.h"

extern int debug_threads;

static int thread_db_use_events;

#include "gdb_proc_service.h"
#include "gdb_thread_db.h"
#include "gdb_vecs.h"

#ifndef USE_LIBTHREAD_DB_DIRECTLY
#include <dlfcn.h>
#endif

#include <stdint.h>
#include <limits.h>
#include <ctype.h>

struct thread_db
{
  /* Structure that identifies the child process for the
     <proc_service.h> interface.  */
  struct ps_prochandle proc_handle;

  /* Connection to the libthread_db library.  */
  td_thragent_t *thread_agent;

  /* If this flag has been set, we've already asked GDB for all
     symbols we might need; assume symbol cache misses are
     failures.  */
  int all_symbols_looked_up;

#ifndef USE_LIBTHREAD_DB_DIRECTLY
  /* Handle of the libthread_db from dlopen.  */
  void *handle;
#endif

  /* Thread creation event breakpoint.  The code at this location in
     the child process will be called by the pthread library whenever
     a new thread is created.  By setting a special breakpoint at this
     location, GDB can detect when a new thread is created.  We obtain
     this location via the td_ta_event_addr call.  Note that if the
     running kernel supports tracing clones, then we don't need to use
     (and in fact don't use) this magic thread event breakpoint to
     learn about threads.  */
  struct breakpoint *td_create_bp;

  /* Addresses of libthread_db functions.  */
  td_err_e (*td_ta_new_p) (struct ps_prochandle * ps, td_thragent_t **ta);
  td_err_e (*td_ta_event_getmsg_p) (const td_thragent_t *ta,
				    td_event_msg_t *msg);
  td_err_e (*td_ta_set_event_p) (const td_thragent_t *ta,
				 td_thr_events_t *event);
  td_err_e (*td_ta_event_addr_p) (const td_thragent_t *ta,
				  td_event_e event, td_notify_t *ptr);
  td_err_e (*td_ta_map_lwp2thr_p) (const td_thragent_t *ta, lwpid_t lwpid,
				   td_thrhandle_t *th);
  td_err_e (*td_thr_get_info_p) (const td_thrhandle_t *th,
				 td_thrinfo_t *infop);
  td_err_e (*td_thr_event_enable_p) (const td_thrhandle_t *th, int event);
  td_err_e (*td_ta_thr_iter_p) (const td_thragent_t *ta,
				td_thr_iter_f *callback, void *cbdata_p,
				td_thr_state_e state, int ti_pri,
				sigset_t *ti_sigmask_p,
				unsigned int ti_user_flags);
  td_err_e (*td_thr_tls_get_addr_p) (const td_thrhandle_t *th,
				     psaddr_t map_address,
				     size_t offset, psaddr_t *address);
  const char ** (*td_symbol_list_p) (void);
};

static char *libthread_db_search_path;

static int find_one_thread (ptid_t);
static int find_new_threads_callback (const td_thrhandle_t *th_p, void *data);

static const char *
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
#ifdef HAVE_TD_VERSION
    case TD_VERSION:
      return "version mismatch between libthread_db and libpthread";
#endif
    default:
      xsnprintf (buf, sizeof (buf), "unknown thread_db error '%d'", err);
      return buf;
    }
}

#if 0
static char *
thread_db_state_str (td_thr_state_e state)
{
  static char buf[64];

  switch (state)
    {
    case TD_THR_STOPPED:
      return "stopped by debugger";
    case TD_THR_RUN:
      return "runnable";
    case TD_THR_ACTIVE:
      return "active";
    case TD_THR_ZOMBIE:
      return "zombie";
    case TD_THR_SLEEP:
      return "sleeping";
    case TD_THR_STOPPED_ASLEEP:
      return "stopped by debugger AND blocked";
    default:
      xsnprintf (buf, sizeof (buf), "unknown thread_db state %d", state);
      return buf;
    }
}
#endif

static int
thread_db_create_event (CORE_ADDR where)
{
  td_event_msg_t msg;
  td_err_e err;
  struct lwp_info *lwp;
  struct thread_db *thread_db = current_process ()->private->thread_db;

  if (thread_db->td_ta_event_getmsg_p == NULL)
    fatal ("unexpected thread_db->td_ta_event_getmsg_p == NULL");

  if (debug_threads)
    fprintf (stderr, "Thread creation event.\n");

  /* FIXME: This assumes we don't get another event.
     In the LinuxThreads implementation, this is safe,
     because all events come from the manager thread
     (except for its own creation, of course).  */
  err = thread_db->td_ta_event_getmsg_p (thread_db->thread_agent, &msg);
  if (err != TD_OK)
    fprintf (stderr, "thread getmsg err: %s\n",
	     thread_db_err_str (err));

  /* If we do not know about the main thread yet, this would be a good time to
     find it.  We need to do this to pick up the main thread before any newly
     created threads.  */
  lwp = get_thread_lwp (current_inferior);
  if (lwp->thread_known == 0)
    find_one_thread (lwp->head.id);

  /* msg.event == TD_EVENT_CREATE */

  find_new_threads_callback (msg.th_p, NULL);

  return 0;
}

static int
thread_db_enable_reporting (void)
{
  td_thr_events_t events;
  td_notify_t notify;
  td_err_e err;
  struct thread_db *thread_db = current_process ()->private->thread_db;

  if (thread_db->td_ta_set_event_p == NULL
      || thread_db->td_ta_event_addr_p == NULL
      || thread_db->td_ta_event_getmsg_p == NULL)
    /* This libthread_db is missing required support.  */
    return 0;

  /* Set the process wide mask saying which events we're interested in.  */
  td_event_emptyset (&events);
  td_event_addset (&events, TD_CREATE);

  err = thread_db->td_ta_set_event_p (thread_db->thread_agent, &events);
  if (err != TD_OK)
    {
      warning ("Unable to set global thread event mask: %s",
	       thread_db_err_str (err));
      return 0;
    }

  /* Get address for thread creation breakpoint.  */
  err = thread_db->td_ta_event_addr_p (thread_db->thread_agent, TD_CREATE,
				       &notify);
  if (err != TD_OK)
    {
      warning ("Unable to get location for thread creation breakpoint: %s",
	       thread_db_err_str (err));
      return 0;
    }
  thread_db->td_create_bp
    = set_breakpoint_at ((CORE_ADDR) (unsigned long) notify.u.bptaddr,
			 thread_db_create_event);

  return 1;
}

static int
find_one_thread (ptid_t ptid)
{
  td_thrhandle_t th;
  td_thrinfo_t ti;
  td_err_e err;
  struct thread_info *inferior;
  struct lwp_info *lwp;
  struct thread_db *thread_db = current_process ()->private->thread_db;
  int lwpid = ptid_get_lwp (ptid);

  inferior = (struct thread_info *) find_inferior_id (&all_threads, ptid);
  lwp = get_thread_lwp (inferior);
  if (lwp->thread_known)
    return 1;

  /* Get information about this thread.  */
  err = thread_db->td_ta_map_lwp2thr_p (thread_db->thread_agent, lwpid, &th);
  if (err != TD_OK)
    error ("Cannot get thread handle for LWP %d: %s",
	   lwpid, thread_db_err_str (err));

  err = thread_db->td_thr_get_info_p (&th, &ti);
  if (err != TD_OK)
    error ("Cannot get thread info for LWP %d: %s",
	   lwpid, thread_db_err_str (err));

  if (debug_threads)
    fprintf (stderr, "Found thread %ld (LWP %d)\n",
	     ti.ti_tid, ti.ti_lid);

  if (lwpid != ti.ti_lid)
    {
      warning ("PID mismatch!  Expected %ld, got %ld",
	       (long) lwpid, (long) ti.ti_lid);
      return 0;
    }

  if (thread_db_use_events)
    {
      err = thread_db->td_thr_event_enable_p (&th, 1);
      if (err != TD_OK)
	error ("Cannot enable thread event reporting for %d: %s",
	       ti.ti_lid, thread_db_err_str (err));
    }

  /* If the new thread ID is zero, a final thread ID will be available
     later.  Do not enable thread debugging yet.  */
  if (ti.ti_tid == 0)
    return 0;

  lwp->thread_known = 1;
  lwp->th = th;

  return 1;
}

/* Attach a thread.  Return true on success.  */

static int
attach_thread (const td_thrhandle_t *th_p, td_thrinfo_t *ti_p)
{
  struct lwp_info *lwp;

  if (debug_threads)
    fprintf (stderr, "Attaching to thread %ld (LWP %d)\n",
	     ti_p->ti_tid, ti_p->ti_lid);
  linux_attach_lwp (ti_p->ti_lid);
  lwp = find_lwp_pid (pid_to_ptid (ti_p->ti_lid));
  if (lwp == NULL)
    {
      warning ("Could not attach to thread %ld (LWP %d)\n",
	       ti_p->ti_tid, ti_p->ti_lid);
      return 0;
    }

  lwp->thread_known = 1;
  lwp->th = *th_p;

  if (thread_db_use_events)
    {
      td_err_e err;
      struct thread_db *thread_db = current_process ()->private->thread_db;

      err = thread_db->td_thr_event_enable_p (th_p, 1);
      if (err != TD_OK)
	error ("Cannot enable thread event reporting for %d: %s",
	       ti_p->ti_lid, thread_db_err_str (err));
    }

  return 1;
}

/* Attach thread if we haven't seen it yet.
   Increment *COUNTER if we have attached a new thread.
   Return false on failure.  */

static int
maybe_attach_thread (const td_thrhandle_t *th_p, td_thrinfo_t *ti_p,
		     int *counter)
{
  struct lwp_info *lwp;

  lwp = find_lwp_pid (pid_to_ptid (ti_p->ti_lid));
  if (lwp != NULL)
    return 1;

  if (!attach_thread (th_p, ti_p))
    return 0;

  if (counter != NULL)
    *counter += 1;

  return 1;
}

static int
find_new_threads_callback (const td_thrhandle_t *th_p, void *data)
{
  td_thrinfo_t ti;
  td_err_e err;
  struct thread_db *thread_db = current_process ()->private->thread_db;

  err = thread_db->td_thr_get_info_p (th_p, &ti);
  if (err != TD_OK)
    error ("Cannot get thread info: %s", thread_db_err_str (err));

  /* Check for zombies.  */
  if (ti.ti_state == TD_THR_UNKNOWN || ti.ti_state == TD_THR_ZOMBIE)
    return 0;

  if (!maybe_attach_thread (th_p, &ti, (int *) data))
    {
      /* Terminate iteration early: we might be looking at stale data in
	 the inferior.  The thread_db_find_new_threads will retry.  */
      return 1;
    }

  return 0;
}

static void
thread_db_find_new_threads (void)
{
  td_err_e err;
  ptid_t ptid = current_ptid;
  struct thread_db *thread_db = current_process ()->private->thread_db;
  int loop, iteration;

  /* This function is only called when we first initialize thread_db.
     First locate the initial thread.  If it is not ready for
     debugging yet, then stop.  */
  if (find_one_thread (ptid) == 0)
    return;

  /* Require 4 successive iterations which do not find any new threads.
     The 4 is a heuristic: there is an inherent race here, and I have
     seen that 2 iterations in a row are not always sufficient to
     "capture" all threads.  */
  for (loop = 0, iteration = 0; loop < 4; ++loop, ++iteration)
    {
      int new_thread_count = 0;

      /* Iterate over all user-space threads to discover new threads.  */
      err = thread_db->td_ta_thr_iter_p (thread_db->thread_agent,
					 find_new_threads_callback,
					 &new_thread_count,
					 TD_THR_ANY_STATE,
					 TD_THR_LOWEST_PRIORITY,
					 TD_SIGNO_MASK, TD_THR_ANY_USER_FLAGS);
      if (debug_threads)
	fprintf (stderr, "Found %d threads in iteration %d.\n",
		 new_thread_count, iteration);

      if (new_thread_count != 0)
	{
	  /* Found new threads.  Restart iteration from beginning.  */
	  loop = -1;
	}
    }
  if (err != TD_OK)
    error ("Cannot find new threads: %s", thread_db_err_str (err));
}

/* Cache all future symbols that thread_db might request.  We can not
   request symbols at arbitrary states in the remote protocol, only
   when the client tells us that new symbols are available.  So when
   we load the thread library, make sure to check the entire list.  */

static void
thread_db_look_up_symbols (void)
{
  struct thread_db *thread_db = current_process ()->private->thread_db;
  const char **sym_list;
  CORE_ADDR unused;

  for (sym_list = thread_db->td_symbol_list_p (); *sym_list; sym_list++)
    look_up_one_symbol (*sym_list, &unused, 1);

  /* We're not interested in any other libraries loaded after this
     point, only in symbols in libpthread.so.  */
  thread_db->all_symbols_looked_up = 1;
}

int
thread_db_look_up_one_symbol (const char *name, CORE_ADDR *addrp)
{
  struct thread_db *thread_db = current_process ()->private->thread_db;
  int may_ask_gdb = !thread_db->all_symbols_looked_up;

  /* If we've passed the call to thread_db_look_up_symbols, then
     anything not in the cache must not exist; we're not interested
     in any libraries loaded after that point, only in symbols in
     libpthread.so.  It might not be an appropriate time to look
     up a symbol, e.g. while we're trying to fetch registers.  */
  return look_up_one_symbol (name, addrp, may_ask_gdb);
}

int
thread_db_get_tls_address (struct thread_info *thread, CORE_ADDR offset,
			   CORE_ADDR load_module, CORE_ADDR *address)
{
  psaddr_t addr;
  td_err_e err;
  struct lwp_info *lwp;
  struct thread_info *saved_inferior;
  struct process_info *proc;
  struct thread_db *thread_db;

  proc = get_thread_process (thread);
  thread_db = proc->private->thread_db;

  /* If the thread layer is not (yet) initialized, fail.  */
  if (thread_db == NULL || !thread_db->all_symbols_looked_up)
    return TD_ERR;

  if (thread_db->td_thr_tls_get_addr_p == NULL)
    return -1;

  lwp = get_thread_lwp (thread);
  if (!lwp->thread_known)
    find_one_thread (lwp->head.id);
  if (!lwp->thread_known)
    return TD_NOTHR;

  saved_inferior = current_inferior;
  current_inferior = thread;
  /* Note the cast through uintptr_t: this interface only works if
     a target address fits in a psaddr_t, which is a host pointer.
     So a 32-bit debugger can not access 64-bit TLS through this.  */
  err = thread_db->td_thr_tls_get_addr_p (&lwp->th,
					  (psaddr_t) (uintptr_t) load_module,
					  offset, &addr);
  current_inferior = saved_inferior;
  if (err == TD_OK)
    {
      *address = (CORE_ADDR) (uintptr_t) addr;
      return 0;
    }
  else
    return err;
}

#ifdef USE_LIBTHREAD_DB_DIRECTLY

static int
thread_db_load_search (void)
{
  td_err_e err;
  struct thread_db *tdb;
  struct process_info *proc = current_process ();

  if (proc->private->thread_db != NULL)
    fatal ("unexpected: proc->private->thread_db != NULL");

  tdb = xcalloc (1, sizeof (*tdb));
  proc->private->thread_db = tdb;

  tdb->td_ta_new_p = &td_ta_new;

  /* Attempt to open a connection to the thread library.  */
  err = tdb->td_ta_new_p (&tdb->proc_handle, &tdb->thread_agent);
  if (err != TD_OK)
    {
      if (debug_threads)
	fprintf (stderr, "td_ta_new(): %s\n", thread_db_err_str (err));
      free (tdb);
      proc->private->thread_db = NULL;
      return 0;
    }

  tdb->td_ta_map_lwp2thr_p = &td_ta_map_lwp2thr;
  tdb->td_thr_get_info_p = &td_thr_get_info;
  tdb->td_ta_thr_iter_p = &td_ta_thr_iter;
  tdb->td_symbol_list_p = &td_symbol_list;

  /* This is required only when thread_db_use_events is on.  */
  tdb->td_thr_event_enable_p = &td_thr_event_enable;

  /* These are not essential.  */
  tdb->td_ta_event_addr_p = &td_ta_event_addr;
  tdb->td_ta_set_event_p = &td_ta_set_event;
  tdb->td_ta_event_getmsg_p = &td_ta_event_getmsg;
  tdb->td_thr_tls_get_addr_p = &td_thr_tls_get_addr;

  return 1;
}

#else

static int
try_thread_db_load_1 (void *handle)
{
  td_err_e err;
  struct thread_db *tdb;
  struct process_info *proc = current_process ();

  if (proc->private->thread_db != NULL)
    fatal ("unexpected: proc->private->thread_db != NULL");

  tdb = xcalloc (1, sizeof (*tdb));
  proc->private->thread_db = tdb;

  tdb->handle = handle;

  /* Initialize pointers to the dynamic library functions we will use.
     Essential functions first.  */

#define CHK(required, a)					\
  do								\
    {								\
      if ((a) == NULL)						\
	{							\
	  if (debug_threads)					\
	    fprintf (stderr, "dlsym: %s\n", dlerror ());	\
	  if (required)						\
	    {							\
	      free (tdb);					\
	      proc->private->thread_db = NULL;			\
	      return 0;						\
	    }							\
	}							\
    }								\
  while (0)

  CHK (1, tdb->td_ta_new_p = dlsym (handle, "td_ta_new"));

  /* Attempt to open a connection to the thread library.  */
  err = tdb->td_ta_new_p (&tdb->proc_handle, &tdb->thread_agent);
  if (err != TD_OK)
    {
      if (debug_threads)
	fprintf (stderr, "td_ta_new(): %s\n", thread_db_err_str (err));
      free (tdb);
      proc->private->thread_db = NULL;
      return 0;
    }

  CHK (1, tdb->td_ta_map_lwp2thr_p = dlsym (handle, "td_ta_map_lwp2thr"));
  CHK (1, tdb->td_thr_get_info_p = dlsym (handle, "td_thr_get_info"));
  CHK (1, tdb->td_ta_thr_iter_p = dlsym (handle, "td_ta_thr_iter"));
  CHK (1, tdb->td_symbol_list_p = dlsym (handle, "td_symbol_list"));

  /* This is required only when thread_db_use_events is on.  */
  CHK (thread_db_use_events,
       tdb->td_thr_event_enable_p = dlsym (handle, "td_thr_event_enable"));

  /* These are not essential.  */
  CHK (0, tdb->td_ta_event_addr_p = dlsym (handle, "td_ta_event_addr"));
  CHK (0, tdb->td_ta_set_event_p = dlsym (handle, "td_ta_set_event"));
  CHK (0, tdb->td_ta_event_getmsg_p = dlsym (handle, "td_ta_event_getmsg"));
  CHK (0, tdb->td_thr_tls_get_addr_p = dlsym (handle, "td_thr_tls_get_addr"));

#undef CHK

  return 1;
}

#ifdef HAVE_DLADDR

/* Lookup a library in which given symbol resides.
   Note: this is looking in the GDBSERVER process, not in the inferior.
   Returns library name, or NULL.  */

static const char *
dladdr_to_soname (const void *addr)
{
  Dl_info info;

  if (dladdr (addr, &info) != 0)
    return info.dli_fname;
  return NULL;
}

#endif

static int
try_thread_db_load (const char *library)
{
  void *handle;

  if (debug_threads)
    fprintf (stderr, "Trying host libthread_db library: %s.\n",
	     library);
  handle = dlopen (library, RTLD_NOW);
  if (handle == NULL)
    {
      if (debug_threads)
	fprintf (stderr, "dlopen failed: %s.\n", dlerror ());
      return 0;
    }

#ifdef HAVE_DLADDR
  if (debug_threads && strchr (library, '/') == NULL)
    {
      void *td_init;

      td_init = dlsym (handle, "td_init");
      if (td_init != NULL)
	{
	  const char *const libpath = dladdr_to_soname (td_init);

	  if (libpath != NULL)
	    fprintf (stderr, "Host %s resolved to: %s.\n",
		     library, libpath);
	}
    }
#endif

  if (try_thread_db_load_1 (handle))
    return 1;

  /* This library "refused" to work on current inferior.  */
  dlclose (handle);
  return 0;
}

/* Handle $sdir in libthread-db-search-path.
   Look for libthread_db in the system dirs, or wherever a plain
   dlopen(file_without_path) will look.
   The result is true for success.  */

static int
try_thread_db_load_from_sdir (void)
{
  return try_thread_db_load (LIBTHREAD_DB_SO);
}

/* Try to load libthread_db from directory DIR of length DIR_LEN.
   The result is true for success.  */

static int
try_thread_db_load_from_dir (const char *dir, size_t dir_len)
{
  char path[PATH_MAX];

  if (dir_len + 1 + strlen (LIBTHREAD_DB_SO) + 1 > sizeof (path))
    {
      char *cp = xmalloc (dir_len + 1);

      memcpy (cp, dir, dir_len);
      cp[dir_len] = '\0';
      warning (_("libthread-db-search-path component too long,"
		 " ignored: %s."), cp);
      free (cp);
      return 0;
    }

  memcpy (path, dir, dir_len);
  path[dir_len] = '/';
  strcpy (path + dir_len + 1, LIBTHREAD_DB_SO);
  return try_thread_db_load (path);
}

/* Search libthread_db_search_path for libthread_db which "agrees"
   to work on current inferior.
   The result is true for success.  */

static int
thread_db_load_search (void)
{
  VEC (char_ptr) *dir_vec;
  char *this_dir;
  int i, rc = 0;

  if (libthread_db_search_path == NULL)
    libthread_db_search_path = xstrdup (LIBTHREAD_DB_SEARCH_PATH);

  dir_vec = dirnames_to_char_ptr_vec (libthread_db_search_path);

  for (i = 0; VEC_iterate (char_ptr, dir_vec, i, this_dir); ++i)
    {
      const int pdir_len = sizeof ("$pdir") - 1;
      size_t this_dir_len;

      this_dir_len = strlen (this_dir);

      if (strncmp (this_dir, "$pdir", pdir_len) == 0
	  && (this_dir[pdir_len] == '\0'
	      || this_dir[pdir_len] == '/'))
	{
	  /* We don't maintain a list of loaded libraries so we don't know
	     where libpthread lives.  We *could* fetch the info, but we don't
	     do that yet.  Ignore it.  */
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

  free_char_ptr_vec (dir_vec);
  if (debug_threads)
    fprintf (stderr, "thread_db_load_search returning %d\n", rc);
  return rc;
}

#endif  /* USE_LIBTHREAD_DB_DIRECTLY */

int
thread_db_init (int use_events)
{
  struct process_info *proc = current_process ();

  /* FIXME drow/2004-10-16: This is the "overall process ID", which
     GNU/Linux calls tgid, "thread group ID".  When we support
     attaching to threads, the original thread may not be the correct
     thread.  We would have to get the process ID from /proc for NPTL.
     For LinuxThreads we could do something similar: follow the chain
     of parent processes until we find the highest one we're attached
     to, and use its tgid.

     This isn't the only place in gdbserver that assumes that the first
     process in the list is the thread group leader.  */

  thread_db_use_events = use_events;

  if (thread_db_load_search ())
    {
      if (use_events && thread_db_enable_reporting () == 0)
	{
	  /* Keep trying; maybe event reporting will work later.  */
	  thread_db_mourn (proc);
	  return 0;
	}
      thread_db_find_new_threads ();
      thread_db_look_up_symbols ();
      return 1;
    }

  return 0;
}

static int
any_thread_of (struct inferior_list_entry *entry, void *args)
{
  int *pid_p = args;

  if (ptid_get_pid (entry->id) == *pid_p)
    return 1;

  return 0;
}

static void
switch_to_process (struct process_info *proc)
{
  int pid = pid_of (proc);

  current_inferior =
    (struct thread_info *) find_inferior (&all_threads,
					  any_thread_of, &pid);
}

/* Disconnect from libthread_db and free resources.  */

static void
disable_thread_event_reporting (struct process_info *proc)
{
  struct thread_db *thread_db = proc->private->thread_db;
  if (thread_db)
    {
      td_err_e (*td_ta_clear_event_p) (const td_thragent_t *ta,
				       td_thr_events_t *event);

#ifndef USE_LIBTHREAD_DB_DIRECTLY
      td_ta_clear_event_p = dlsym (thread_db->handle, "td_ta_clear_event");
#else
      td_ta_clear_event_p = &td_ta_clear_event;
#endif

      if (td_ta_clear_event_p != NULL)
	{
	  struct thread_info *saved_inferior = current_inferior;
	  td_thr_events_t events;

	  switch_to_process (proc);

	  /* Set the process wide mask saying we aren't interested
	     in any events anymore.  */
	  td_event_fillset (&events);
	  (*td_ta_clear_event_p) (thread_db->thread_agent, &events);

	  current_inferior = saved_inferior;
	}
    }
}

static void
remove_thread_event_breakpoints (struct process_info *proc)
{
  struct thread_db *thread_db = proc->private->thread_db;

  if (thread_db->td_create_bp != NULL)
    {
      struct thread_info *saved_inferior = current_inferior;

      switch_to_process (proc);

      delete_breakpoint (thread_db->td_create_bp);
      thread_db->td_create_bp = NULL;

      current_inferior = saved_inferior;
    }
}

void
thread_db_detach (struct process_info *proc)
{
  struct thread_db *thread_db = proc->private->thread_db;

  if (thread_db)
    {
      disable_thread_event_reporting (proc);
      remove_thread_event_breakpoints (proc);
    }
}

/* Disconnect from libthread_db and free resources.  */

void
thread_db_mourn (struct process_info *proc)
{
  struct thread_db *thread_db = proc->private->thread_db;
  if (thread_db)
    {
      td_err_e (*td_ta_delete_p) (td_thragent_t *);

#ifndef USE_LIBTHREAD_DB_DIRECTLY
      td_ta_delete_p = dlsym (thread_db->handle, "td_ta_delete");
#else
      td_ta_delete_p = &td_ta_delete;
#endif

      if (td_ta_delete_p != NULL)
	(*td_ta_delete_p) (thread_db->thread_agent);

#ifndef USE_LIBTHREAD_DB_DIRECTLY
      dlclose (thread_db->handle);
#endif  /* USE_LIBTHREAD_DB_DIRECTLY  */

      free (thread_db);
      proc->private->thread_db = NULL;
    }
}

/* Handle "set libthread-db-search-path" monitor command and return 1.
   For any other command, return 0.  */

int
thread_db_handle_monitor_command (char *mon)
{
  const char *cmd = "set libthread-db-search-path";
  size_t cmd_len = strlen (cmd);

  if (strncmp (mon, cmd, cmd_len) == 0
      && (mon[cmd_len] == '\0'
	  || mon[cmd_len] == ' '))
    {
      const char *cp = mon + cmd_len;

      if (libthread_db_search_path != NULL)
	free (libthread_db_search_path);

      /* Skip leading space (if any).  */
      while (isspace (*cp))
	++cp;

      if (*cp == '\0')
	cp = LIBTHREAD_DB_SEARCH_PATH;
      libthread_db_search_path = xstrdup (cp);

      monitor_output ("libthread-db-search-path set to `");
      monitor_output (libthread_db_search_path);
      monitor_output ("'\n");
      return 1;
    }

  /* Tell server.c to perform default processing.  */
  return 0;
}
