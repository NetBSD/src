/* Multi-threaded debugging support for the thread_db interface,
   used on operating systems such as Solaris and Linux.
   Copyright 1999 Free Software Foundation, Inc.

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

/* This module implements a thread_stratum target that sits on top of
   a normal process_stratum target (such as procfs or ptrace).  The
   process_stratum target must install this thread_stratum target when
   it detects the presence of the thread_db shared library.

   This module will then use the thread_db API to add thread-awareness
   to the functionality provided by the process_stratum target (or in
   some cases, to add user-level thread awareness on top of the 
   kernel-level thread awareness that is already provided by the 
   process_stratum target).

   Solaris threads (for instance) are a multi-level thread implementation;
   the kernel provides a Light Weight Process (LWP) which the procfs 
   process_stratum module is aware of.  This module must then mediate
   the relationship between kernel LWP threads and user (eg. posix)
   threads.

   Linux threads are likely to be different -- but the thread_db 
   library API should make the difference largely transparent to GDB.

   */

/* The thread_db API provides a number of functions that give the caller
   access to the inner workings of the child process's thread library. 
   We will be using the following (others may be added):

   td_thr_validate		Confirm valid "live" thread
   td_thr_get_info		Get info about a thread
   td_thr_getgregs		Get thread's general registers
   td_thr_getfpregs		Get thread's floating point registers
   td_thr_setgregs		Set thread's general registers
   td_thr_setfpregs		Set thread's floating point registers
   td_ta_map_id2thr		Get thread handle from thread id
   td_ta_map_lwp2thr		Get thread handle from LWP id
   td_ta_thr_iter		Iterate over all threads (with callback)

   In return, the debugger has to provide certain services to the 
   thread_db library.  Some of these aren't actually required to do
   anything in practice.  For instance, the thread_db expects to be
   able to stop the child process and start it again: but in our
   context, the child process will always be stopped already when we
   invoke the thread_db library, so the functions that we provide for
   the library to stop and start the child process are no-ops.

   Here is the list of functions which we export to the thread_db
   library, divided into no-op functions vs. functions that actually
   have to do something:

   No-op functions:

   ps_pstop			Stop the child process
   ps_pcontinue			Continue the child process
   ps_lstop			Stop a specific LWP (kernel thread)
   ps_lcontinue			Continue an LWP
   ps_lgetxregsize		Get size of LWP's xregs (sparc)
   ps_lgetxregs			Get LWP's xregs (sparc)
   ps_lsetxregs			Set LWP's xregs (sparc)

   Functions that have to do useful work:

   ps_pglobal_lookup		Get the address of a global symbol
   ps_pdread			Read memory, data segment
   ps_ptread			Read memory, text segment
   ps_pdwrite			Write memory, data segment
   ps_ptwrite			Write memory, text segment
   ps_lgetregs			Get LWP's general registers
   ps_lgetfpregs		Get LWP's floating point registers
   ps_lsetregs			Set LWP's general registers
   ps_lsetfpregs		Set LWP's floating point registers
   ps_lgetLDT			Get LWP's Local Descriptor Table (x86)
   
   Thus, if we ask the thread_db library to give us the general registers
   for user thread X, thread_db may figure out that user thread X is 
   actually mapped onto kernel thread Y.  Thread_db does not know how
   to obtain the registers for kernel thread Y, but GDB does, so thread_db
   turns the request right back to us via the ps_lgetregs callback.  */

#include "defs.h"
#include "gdbthread.h"
#include "target.h"
#include "inferior.h"
#include "gdbcmd.h"

#include "gdb_wait.h"

#include <time.h>

#if defined(USE_PROC_FS) || defined(HAVE_GREGSET_T)
#include <sys/procfs.h>
#endif

#if defined (HAVE_PROC_SERVICE_H)
#include <proc_service.h>	/* defines incoming API (ps_* callbacks) */
#else
#include "gdb_proc_service.h"
#endif

#if defined HAVE_STDINT_H	/* Pre-5.2 systems don't have this header */
#if defined (HAVE_THREAD_DB_H)
#include <thread_db.h>		/* defines outgoing API (td_thr_* calls) */
#else
#include "gdb_thread_db.h"
#endif

#include <dlfcn.h>		/* dynamic library interface */

#ifndef TIDGET
#define TIDGET(PID)		(((PID) & 0x7fffffff) >> 16)
#define PIDGET(PID)		(((PID) & 0xffff))
#define MERGEPID(PID, TID)	(((PID) & 0xffff) | ((TID) << 16))
#endif

/* Macros for superimposing PID and TID into inferior_pid.  */
#define THREAD_FLAG		0x80000000
#define is_thread(ARG)		(((ARG) & THREAD_FLAG) != 0)
#define is_lwp(ARG)		(((ARG) & THREAD_FLAG) == 0)
#define GET_LWP(PID)		TIDGET (PID)
#define GET_THREAD(PID)		TIDGET (PID)
#define BUILD_LWP(TID, PID)	MERGEPID (PID, TID)
#define BUILD_THREAD(TID, PID)	(MERGEPID (PID, TID) | THREAD_FLAG)

/*
 * target_beneath is a pointer to the target_ops underlying this one.
 */

static struct target_ops *target_beneath;


/*
 * target vector defined in this module:
 */

static struct target_ops thread_db_ops;

/*
 * Typedefs required to resolve differences between the thread_db
 * and proc_service API defined on different versions of Solaris:
 */

#if defined(PROC_SERVICE_IS_OLD)
typedef const struct ps_prochandle *gdb_ps_prochandle_t;
typedef char *gdb_ps_read_buf_t;
typedef char *gdb_ps_write_buf_t;
typedef int gdb_ps_size_t;
#else
typedef struct ps_prochandle *gdb_ps_prochandle_t;
typedef void *gdb_ps_read_buf_t;
typedef const void *gdb_ps_write_buf_t;
typedef size_t gdb_ps_size_t;
#endif

/* Unfortunately glibc 2.1.3 was released with a broken prfpregset_t
   type.  We let configure check for this lossage, and make
   appropriate typedefs here.  */

#ifdef PRFPREGSET_T_BROKEN
typedef elf_fpregset_t gdb_prfpregset_t;
#else
typedef prfpregset_t gdb_prfpregset_t;
#endif

/* 
 * proc_service callback functions, called by thread_db.
 */

ps_err_e
ps_pstop (gdb_ps_prochandle_t ph)		/* Process stop */
{
  return PS_OK;
}

ps_err_e
ps_pcontinue (gdb_ps_prochandle_t ph)		/* Process continue */
{
  return PS_OK;
}

ps_err_e
ps_lstop (gdb_ps_prochandle_t ph,		/* LWP stop */
	  lwpid_t lwpid)
{
  return PS_OK;
}

ps_err_e
ps_lcontinue (gdb_ps_prochandle_t ph,		/* LWP continue */
	      lwpid_t lwpid)
{
  return PS_OK;
}

ps_err_e
ps_lgetxregsize (gdb_ps_prochandle_t ph,	/* Get XREG size */
		 lwpid_t lwpid,
		 int *xregsize)
{
  return PS_OK;
}

ps_err_e
ps_lgetxregs (gdb_ps_prochandle_t ph,		/* Get XREGS */
	      lwpid_t lwpid,
	      caddr_t xregset)
{
  return PS_OK;
}

ps_err_e
ps_lsetxregs (gdb_ps_prochandle_t ph,		/* Set XREGS */
	      lwpid_t lwpid,
	      caddr_t xregset)
{
  return PS_OK;
}

void
ps_plog (const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  vfprintf_filtered (gdb_stderr, fmt, args);
}

/* Look up a symbol in GDB's global symbol table.
   Return the symbol's address.
   FIXME: it would be more correct to look up the symbol in the context 
   of the LD_OBJECT_NAME provided.  However we're probably fairly safe 
   as long as there aren't name conflicts with other libraries.  */

ps_err_e
ps_pglobal_lookup (gdb_ps_prochandle_t ph,
		   const char *ld_object_name,	/* the library name */
		   const char *ld_symbol_name,	/* the symbol name */
		   paddr_t    *ld_symbol_addr)	/* return the symbol addr */
{
  struct minimal_symbol *ms;

  ms = lookup_minimal_symbol (ld_symbol_name, NULL, NULL);

  if (!ms)
    return PS_NOSYM;

  *ld_symbol_addr = SYMBOL_VALUE_ADDRESS (ms);

  return PS_OK;
}

/* Worker function for all memory reads and writes: */
static ps_err_e rw_common (const struct ps_prochandle *ph, 
			   paddr_t addr,
			   char *buf, 
			   int size, 
			   int write_p);

/* target_xfer_memory direction consts */
enum {PS_READ = 0, PS_WRITE = 1};

ps_err_e
ps_pdread (gdb_ps_prochandle_t ph, 	/* read from data segment */
	   paddr_t             addr,
	   gdb_ps_read_buf_t   buf,
	   gdb_ps_size_t       size)
{
  return rw_common (ph, addr, buf, size, PS_READ);
}

ps_err_e
ps_pdwrite (gdb_ps_prochandle_t ph,	/* write to data segment */
	    paddr_t             addr,
	    gdb_ps_write_buf_t  buf,
	    gdb_ps_size_t       size)
{
  return rw_common (ph, addr, (char *) buf, size, PS_WRITE);
}

ps_err_e
ps_ptread (gdb_ps_prochandle_t ph,	/* read from text segment */
	   paddr_t             addr,
	   gdb_ps_read_buf_t   buf,
	   gdb_ps_size_t       size)
{
  return rw_common (ph, addr, buf, size, PS_READ);
}

ps_err_e
ps_ptwrite (gdb_ps_prochandle_t ph,	/* write to text segment */
	    paddr_t             addr,
	    gdb_ps_write_buf_t  buf,
	    gdb_ps_size_t       size)
{
  return rw_common (ph, addr, (char *) buf, size, PS_WRITE);
}

static struct cleanup *save_inferior_pid    (void);
static void            restore_inferior_pid (void *saved_pid);
static char *thr_err_string   (td_err_e);
static char *thr_state_string (td_thr_state_e);

struct ps_prochandle {
  int pid;
};

struct ps_prochandle main_prochandle;
td_thragent_t *      main_threadagent;

/* 
 * Common proc_service routine for reading and writing memory.  
 */

/* FIXME: once we've munged the inferior_pid, why can't we
   simply call target_read/write_memory and return?  */


static ps_err_e
rw_common (const struct ps_prochandle *ph,
	   paddr_t addr,
	   char   *buf,
	   int     size,
	   int     write_p)
{
  struct cleanup *old_chain = save_inferior_pid ();
  int to_do = size;
  int done  = 0;

  inferior_pid = main_prochandle.pid;

  while (to_do > 0)
    {
      done = current_target.to_xfer_memory (addr, buf, size, write_p, 
					    &current_target);
      if (done <= 0)
	{
	  if (write_p == PS_READ)
	    print_sys_errmsg ("rw_common (): read", errno);
	  else
	    print_sys_errmsg ("rw_common (): write", errno);

	  return PS_ERR;
	}
      to_do -= done;
      buf   += done;
    }
  do_cleanups (old_chain);
  return PS_OK;
}

/* Cleanup functions used by the register callbacks
   (which have to manipulate the global inferior_pid).  */

ps_err_e
ps_lgetregs (gdb_ps_prochandle_t ph,		/* Get LWP general regs */
	     lwpid_t     lwpid,
	     prgregset_t gregset)
{
  struct cleanup *old_chain = save_inferior_pid ();

  inferior_pid = BUILD_LWP (lwpid, main_prochandle.pid);
  current_target.to_fetch_registers (-1);

  fill_gregset (gregset, -1);
  do_cleanups (old_chain);

  return PS_OK;
}

ps_err_e
ps_lsetregs (gdb_ps_prochandle_t ph,		/* Set LWP general regs */
	     lwpid_t           lwpid,
	     const prgregset_t gregset)
{
  struct cleanup *old_chain = save_inferior_pid ();

  inferior_pid = BUILD_LWP (lwpid, main_prochandle.pid);
  supply_gregset (gregset);
  current_target.to_store_registers (-1);
  do_cleanups (old_chain);
  return PS_OK;
}

ps_err_e
ps_lgetfpregs (gdb_ps_prochandle_t ph,		/* Get LWP float regs */
	       lwpid_t       lwpid,
	       gdb_prfpregset_t *fpregset)
{
  struct cleanup *old_chain = save_inferior_pid ();

  inferior_pid = BUILD_LWP (lwpid, main_prochandle.pid);
  current_target.to_fetch_registers (-1);
  fill_fpregset (fpregset, -1);
  do_cleanups (old_chain);
  return PS_OK;
}

ps_err_e
ps_lsetfpregs (gdb_ps_prochandle_t ph,		/* Set LWP float regs */
	       lwpid_t             lwpid,
	       const gdb_prfpregset_t *fpregset)
{
  struct cleanup *old_chain = save_inferior_pid ();

  inferior_pid = BUILD_LWP (lwpid, main_prochandle.pid);
  supply_fpregset (fpregset);
  current_target.to_store_registers (-1);
  do_cleanups (old_chain);
  return PS_OK;
}

/*
 * ps_getpid
 *
 * return the main pid for the child process
 * (special for Linux -- not used on Solaris)
 */

pid_t
ps_getpid (gdb_ps_prochandle_t ph)
{
  return ph->pid;
}

#ifdef TM_I386SOL2_H

/* Reads the local descriptor table of a LWP.  */

ps_err_e
ps_lgetLDT (gdb_ps_prochandle_t ph, lwpid_t lwpid,
	    struct ssd *pldt)
{
  /* NOTE: only used on Solaris, therefore OK to refer to procfs.c */
  extern struct ssd *procfs_find_LDT_entry (int);
  struct ssd *ret;

  ret = procfs_find_LDT_entry (BUILD_LWP (lwpid, 
					  PIDGET (main_prochandle.pid)));
  if (ret)
    {
      memcpy (pldt, ret, sizeof (struct ssd));
      return PS_OK;
    }
  else	/* LDT not found. */
    return PS_ERR;
}
#endif /* TM_I386SOL2_H */

/*
 * Pointers to thread_db functions:
 *
 * These are a dynamic library mechanism.
 * The dlfcn.h interface will be used to initialize these 
 * so that they point to the appropriate functions in the
 * thread_db dynamic library.  This is done dynamically 
 * so that GDB can still run on systems that lack thread_db.  
 */

static td_err_e (*p_td_init)              (void);

static td_err_e (*p_td_ta_new)            (const struct ps_prochandle *ph_p, 
					   td_thragent_t **ta_pp);

static td_err_e (*p_td_ta_delete)         (td_thragent_t *ta_p);

static td_err_e (*p_td_ta_get_nthreads)   (const td_thragent_t *ta_p,
					   int *nthread_p);


static td_err_e (*p_td_ta_thr_iter)       (const td_thragent_t *ta_p,
					   td_thr_iter_f *cb,
					   void *cbdata_p,
					   td_thr_state_e state,
					   int ti_pri, 
					   sigset_t *ti_sigmask_p,
					   unsigned ti_user_flags);

static td_err_e (*p_td_ta_event_addr)     (const td_thragent_t *ta_p,
					   u_long event,
					   td_notify_t *notify_p);

static td_err_e (*p_td_ta_event_getmsg)   (const td_thragent_t *ta_p,
					   td_event_msg_t *msg);

static td_err_e (*p_td_ta_set_event)      (const td_thragent_t *ta_p,
					   td_thr_events_t *events);

static td_err_e (*p_td_thr_validate)      (const td_thrhandle_t *th_p);

static td_err_e (*p_td_thr_event_enable)  (const td_thrhandle_t *th_p,
					   int on_off);

static td_err_e (*p_td_thr_get_info)      (const td_thrhandle_t *th_p,
					   td_thrinfo_t *ti_p);

static td_err_e (*p_td_thr_getgregs)      (const td_thrhandle_t *th_p,
					   prgregset_t regset);

static td_err_e (*p_td_thr_setgregs)      (const td_thrhandle_t *th_p,
					   const prgregset_t regset);

static td_err_e (*p_td_thr_getfpregs)     (const td_thrhandle_t *th_p,
					   gdb_prfpregset_t *fpregset);

static td_err_e (*p_td_thr_setfpregs)     (const td_thrhandle_t *th_p,
					   const gdb_prfpregset_t *fpregset);

static td_err_e (*p_td_ta_map_id2thr)     (const td_thragent_t *ta_p,
					   thread_t tid,
					   td_thrhandle_t *th_p);

static td_err_e (*p_td_ta_map_lwp2thr)    (const td_thragent_t *ta_p,
					   lwpid_t lwpid,
					   td_thrhandle_t *th_p);

/*
 * API and target vector initialization function: thread_db_initialize.
 *
 * NOTE: this function is deliberately NOT named with the GDB convention
 * of module initializer function names that begin with "_initialize".
 * This module is NOT intended to be auto-initialized at GDB startup.
 * Rather, it will only be initialized when a multi-threaded child
 * process is detected.
 *
 */

/* 
 * Initializer for thread_db library interface.
 * This function does the dynamic library stuff (dlopen, dlsym), 
 * and then calls the thread_db library's one-time initializer 
 * function (td_init).  If everything succeeds, this function
 * returns true; otherwise it returns false, and this module
 * cannot be used.
 */

static int
init_thread_db_library ()
{
  void *dlhandle;
  td_err_e ret;

  /* Open a handle to the "thread_db" dynamic library.  */
  if ((dlhandle = dlopen ("libthread_db.so.1", RTLD_NOW)) == NULL)
    return 0;			/* fail */

  /* Initialize pointers to the dynamic library functions we will use.
   * Note that we are not calling the functions here -- we are only
   * establishing pointers to them.
   */

  /* td_init: initialize thread_db library. */
  if ((p_td_init = dlsym (dlhandle, "td_init")) == NULL)
    return 0;			/* fail */
  /* td_ta_new: register a target process with thread_db.  */
  if ((p_td_ta_new = dlsym (dlhandle, "td_ta_new")) == NULL)
    return 0;			/* fail */
  /* td_ta_delete: un-register a target process with thread_db.  */
  if ((p_td_ta_delete = dlsym (dlhandle, "td_ta_delete")) == NULL)
    return 0;			/* fail */

  /* td_ta_map_id2thr: get thread handle from thread id.  */
  if ((p_td_ta_map_id2thr = dlsym (dlhandle, "td_ta_map_id2thr")) == NULL)
    return 0;			/* fail */
  /* td_ta_map_lwp2thr: get thread handle from lwp id.  */
  if ((p_td_ta_map_lwp2thr = dlsym (dlhandle, "td_ta_map_lwp2thr")) == NULL)
    return 0;			/* fail */
  /* td_ta_get_nthreads: get number of threads in target process.  */
  if ((p_td_ta_get_nthreads = dlsym (dlhandle, "td_ta_get_nthreads")) == NULL)
    return 0;			/* fail */
  /* td_ta_thr_iter: iterate over all thread handles.  */
  if ((p_td_ta_thr_iter = dlsym (dlhandle, "td_ta_thr_iter")) == NULL)
    return 0;			/* fail */

  /* td_thr_validate: make sure a thread handle is real and alive.  */
  if ((p_td_thr_validate = dlsym (dlhandle, "td_thr_validate")) == NULL)
    return 0;			/* fail */
  /* td_thr_get_info: get a bunch of info about a thread.  */
  if ((p_td_thr_get_info = dlsym (dlhandle, "td_thr_get_info")) == NULL)
    return 0;			/* fail */
  /* td_thr_getgregs: get general registers for thread.  */
  if ((p_td_thr_getgregs = dlsym (dlhandle, "td_thr_getgregs")) == NULL)
    return 0;			/* fail */
  /* td_thr_setgregs: set general registers for thread.  */
  if ((p_td_thr_setgregs = dlsym (dlhandle, "td_thr_setgregs")) == NULL)
    return 0;			/* fail */
  /* td_thr_getfpregs: get floating point registers for thread.  */
  if ((p_td_thr_getfpregs = dlsym (dlhandle, "td_thr_getfpregs")) == NULL)
    return 0;			/* fail */
  /* td_thr_setfpregs: set floating point registers for thread.  */
  if ((p_td_thr_setfpregs = dlsym (dlhandle, "td_thr_setfpregs")) == NULL)
    return 0;			/* fail */
  
  ret = p_td_init ();
  if (ret != TD_OK)
    {
      warning ("init_thread_db: td_init: %s", thr_err_string (ret));
      return 0;
    }

  /* Optional functions:
     We can still debug even if the following functions are not found.  */

  /* td_ta_event_addr: get the breakpoint address for specified event.  */
  p_td_ta_event_addr = dlsym (dlhandle, "td_ta_event_addr");

  /* td_ta_event_getmsg: get the next event message for the process.  */
  p_td_ta_event_getmsg = dlsym (dlhandle, "td_ta_event_getmsg");

  /* td_ta_set_event: request notification of an event.  */
  p_td_ta_set_event = dlsym (dlhandle, "td_ta_set_event");

  /* td_thr_event_enable: enable event reporting in a thread.  */
  p_td_thr_event_enable = dlsym (dlhandle, "td_thr_event_enable");

  return 1;			/* success */
}

/*
 * Local utility functions:
 */


/*

   LOCAL FUNCTION

   save_inferior_pid - Save inferior_pid on the cleanup list
   restore_inferior_pid - Restore inferior_pid from the cleanup list

   SYNOPSIS

   struct cleanup *save_inferior_pid (void);
   void            restore_inferior_pid (void *saved_pid);

   DESCRIPTION

   These two functions act in unison to restore inferior_pid in
   case of an error.

   NOTES

   inferior_pid is a global variable that needs to be changed by many
   of these routines before calling functions in procfs.c.  In order
   to guarantee that inferior_pid gets restored (in case of errors),
   you need to call save_inferior_pid before changing it.  At the end
   of the function, you should invoke do_cleanups to restore it.

 */

static struct cleanup *
save_inferior_pid (void)
{
  int *saved_pid_ptr;
  
  saved_pid_ptr = xmalloc (sizeof (int));
  *saved_pid_ptr = inferior_pid;
  return make_cleanup (restore_inferior_pid, saved_pid_ptr);
}

static void
restore_inferior_pid (void *arg)
{
  int *saved_pid_ptr = arg;
  inferior_pid = *saved_pid_ptr;
  free (arg);
}

/*

   LOCAL FUNCTION

   thr_err_string - Convert a thread_db error code to a string

   SYNOPSIS

   char * thr_err_string (errcode)

   DESCRIPTION

   Return a string description of the thread_db errcode.  If errcode
   is unknown, then return an <unknown> message.

 */

static char *
thr_err_string (errcode)
     td_err_e errcode;
{
  static char buf[50];

  switch (errcode) {
  case TD_OK:		return "generic 'call succeeded'";
  case TD_ERR:		return "generic error";
  case TD_NOTHR:	return "no thread to satisfy query";
  case TD_NOSV:		return "no sync handle to satisfy query";
  case TD_NOLWP:	return "no lwp to satisfy query";
  case TD_BADPH:	return "invalid process handle";
  case TD_BADTH:	return "invalid thread handle";
  case TD_BADSH:	return "invalid synchronization handle";
  case TD_BADTA:	return "invalid thread agent";
  case TD_BADKEY:	return "invalid key";
  case TD_NOMSG:	return "no event message for getmsg";
  case TD_NOFPREGS:	return "FPU register set not available";
  case TD_NOLIBTHREAD:	return "application not linked with libthread";
  case TD_NOEVENT:	return "requested event is not supported";
  case TD_NOCAPAB:	return "capability not available";
  case TD_DBERR:	return "debugger service failed";
  case TD_NOAPLIC:	return "operation not applicable to";
  case TD_NOTSD:	return "no thread-specific data for this thread";
  case TD_MALLOC:	return "malloc failed";
  case TD_PARTIALREG:	return "only part of register set was written/read";
  case TD_NOXREGS:	return "X register set not available for this thread";
  default:
    sprintf (buf, "unknown thread_db error '%d'", errcode);
    return buf;
  }
}

/*

   LOCAL FUNCTION

   thr_state_string - Convert a thread_db state code to a string

   SYNOPSIS

   char *thr_state_string (statecode)

   DESCRIPTION

   Return the thread_db state string associated with statecode.  
   If statecode is unknown, then return an <unknown> message.

 */

static char *
thr_state_string (statecode)
     td_thr_state_e statecode;
{
  static char buf[50];

  switch (statecode) {
  case TD_THR_STOPPED:		return "stopped by debugger";
  case TD_THR_RUN:		return "runnable";
  case TD_THR_ACTIVE:		return "active";
  case TD_THR_ZOMBIE:		return "zombie";
  case TD_THR_SLEEP:		return "sleeping";
  case TD_THR_STOPPED_ASLEEP:	return "stopped by debugger AND blocked";
  default:
    sprintf (buf, "unknown thread_db state %d", statecode);
    return buf;
  }
}

/*
 * Local thread/event list.
 * This data structure will be used to hold a list of threads and 
 * pending/deliverable events.
 */

typedef struct THREADINFO {
  thread_t       tid;		/* thread ID */
  pid_t          lid;		/* process/lwp ID */
  td_thr_state_e state;		/* thread state (a la thread_db) */
  td_thr_type_e  type;		/* thread type (a la thread_db) */
  int            pending;	/* true if holding a pending event */
  int            status;	/* wait status of any interesting event */
} threadinfo;

threadinfo * threadlist;
int threadlist_max = 0;		/* current size of table */
int threadlist_top = 0;		/* number of threads now in table */
#define THREADLIST_ALLOC 100	/* chunk size by which to expand table */

static threadinfo *
insert_thread (tid, lid, state, type)
     int            tid;
     int            lid;
     td_thr_state_e state;
     td_thr_type_e  type;
{
  if (threadlist_top >= threadlist_max)
    {
      threadlist_max += THREADLIST_ALLOC;
      threadlist      = realloc (threadlist, 
				 threadlist_max * sizeof (threadinfo));
      if (threadlist == NULL)
	return NULL;
    }
  threadlist[threadlist_top].tid     = tid;
  threadlist[threadlist_top].lid     = lid;
  threadlist[threadlist_top].state   = state;
  threadlist[threadlist_top].type    = type;
  threadlist[threadlist_top].pending = 0;
  threadlist[threadlist_top].status  = 0;

  return &threadlist[threadlist_top++];
}

static void
empty_threadlist ()
{
  threadlist_top = 0;
}

static threadinfo *
next_pending_event ()
{
  int i;

  for (i = 0; i < threadlist_top; i++)
    if (threadlist[i].pending)
      return &threadlist[i];

  return NULL;
}

static void
threadlist_iter (func, data, state, type)
     int (*func) ();
     void *data;
     td_thr_state_e state;
     td_thr_type_e  type;
{
  int i;

  for (i = 0; i < threadlist_top; i++)
    if ((state == TD_THR_ANY_STATE || state == threadlist[i].state) &&
	(type  == TD_THR_ANY_TYPE  || type  == threadlist[i].type))
      if ((*func) (&threadlist[i], data) != 0)
	break;

  return;
}     

/*
 * Global state
 * 
 * Here we keep state information all collected in one place.
 */

/* This flag is set when we activate, so that we don't do it twice. 
   Defined in linux-thread.c and used for inter-target syncronization.  */
extern int using_thread_db;

/* The process id for which we've stopped.
 * This is only set when we actually stop all threads.
 * Otherwise it's zero.
 */
static int event_pid;

/*
 * The process id for a new thread to which we've just attached.
 * This process needs special handling at resume time.
 */
static int attach_pid;


/*
 * thread_db event handling:
 *
 * The mechanism for event notification via the thread_db API.
 * These events are implemented as breakpoints.  The thread_db
 * library gives us an address where we can set a breakpoint.
 * When the breakpoint is hit, it represents an event of interest
 * such as:
 *   Thread creation
 *   Thread death
 *   Thread reap
 */

/* Location of the thread creation event breakpoint.  The code at this
   location in the child process will be called by the pthread library
   whenever a new thread is created.  By setting a special breakpoint
   at this location, GDB can detect when a new thread is created.  We
   obtain this location via the td_ta_event_addr call.  */

static CORE_ADDR thread_creation_bkpt_address;

/* Location of the thread death event breakpoint.  The code at this
   location in the child process will be called by the pthread library
   whenever a thread is destroyed.  By setting a special breakpoint at
   this location, GDB can detect when a new thread is created.  We
   obtain this location via the td_ta_event_addr call.  */

static CORE_ADDR thread_death_bkpt_address;

/* This function handles the global parts of enabling thread events.
   The thread-specific enabling is handled per-thread elsewhere.  */

static void
enable_thread_event_reporting (ta)
     td_thragent_t *ta;
{
  td_thr_events_t events;
  td_notify_t     notify;
  CORE_ADDR       addr;

  if (p_td_ta_set_event     == NULL ||
      p_td_ta_event_addr    == NULL ||
      p_td_ta_event_getmsg  == NULL ||
      p_td_thr_event_enable == NULL)
    return;	/* can't do thread event reporting without these funcs */

  /* set process wide mask saying which events we are interested in */
  td_event_emptyset (&events);
  td_event_addset (&events, TD_CREATE);
  td_event_addset (&events, TD_DEATH);

  if (p_td_ta_set_event (ta, &events) != TD_OK)
    {
      warning ("unable to set global thread event mask");
      return;
    }

  /* Delete previous thread event breakpoints, if any.  */
  remove_thread_event_breakpoints ();

  /* create breakpoints -- thread creation and death */
  /* thread creation */
  /* get breakpoint location */
  if (p_td_ta_event_addr (ta, TD_CREATE, &notify) != TD_OK)
    {
      warning ("unable to get location for thread creation breakpoint");
      return;
    }

  /* Set up the breakpoint. */
  create_thread_event_breakpoint (notify.u.bptaddr);

  /* Save it's location. */
  thread_creation_bkpt_address = notify.u.bptaddr;

  /* thread death */
  /* get breakpoint location */
  if (p_td_ta_event_addr (ta, TD_DEATH, &notify) != TD_OK)
    {
      warning ("unable to get location for thread death breakpoint");
      return;
    }
  /* Set up the breakpoint. */
  create_thread_event_breakpoint (notify.u.bptaddr);

  /* Save it's location. */
  thread_death_bkpt_address = notify.u.bptaddr;
}

/* This function handles the global parts of disabling thread events.
   The thread-specific enabling is handled per-thread elsewhere.  */

static void
disable_thread_event_reporting (ta)
     td_thragent_t *ta;
{
  td_thr_events_t events;

  /* set process wide mask saying we aren't interested in any events */
  td_event_emptyset (&events);
  p_td_ta_set_event (main_threadagent, &events);

  /* Delete thread event breakpoints, if any.  */
  remove_thread_event_breakpoints ();
  thread_creation_bkpt_address = 0;
  thread_death_bkpt_address = 0;
}

/* check_for_thread_event
   
   if it's a thread event we recognize (currently
   we only recognize creation and destruction
   events), return 1; else return 0.  */


static int
check_for_thread_event (struct target_waitstatus *tws, int event_pid)
{
  /* FIXME: to be more efficient, we should keep a static 
     list of threads, and update it only here (with td_ta_thr_iter). */
}

static void
thread_db_push_target (void)
{
  /* Called ONLY from thread_db_new_objfile after td_ta_new call succeeds. */

  /* Push this target vector */
  push_target (&thread_db_ops);
  /* Find the underlying process-layer target for calling later.  */
  target_beneath = find_target_beneath (&thread_db_ops);
  using_thread_db = 1;
  /* Turn on thread_db event-reporting API.  */
  enable_thread_event_reporting (main_threadagent);
}

static void
thread_db_unpush_target (void)
{
  /* Must be called whenever we remove ourself from the target stack! */

  using_thread_db = 0;
  target_beneath = NULL;

  /* delete local list of threads */
  empty_threadlist ();
  /* Turn off the thread_db API.  */
  p_td_ta_delete (main_threadagent);
  /* Unpush this target vector */
  unpush_target (&thread_db_ops);
  /* Reset linuxthreads module.  */
  linuxthreads_discard_global_state ();
}

/*
 * New objfile hook function:
 * Called for each new objfile (image, shared lib) in the target process.
 *
 * The purpose of this function is to detect that the target process
 * is linked with the (appropriate) thread library.  So every time a
 * new target shared library is detected, we will call td_ta_new.
 * If it succeeds, we know we have a multi-threaded target process
 * that we can debug using the thread_db API.
 */

/* 
 * new_objfile function:
 *
 * connected to target_new_objfile_hook, this function gets called
 * every time a new binary image is loaded.
 *
 * At each call, we attempt to open the thread_db connection to the
 * child process.  If it succeeds, we know we have a libthread process
 * and we can debug it with this target vector.  Therefore we push
 * ourself onto the target stack.
 */

static void (*target_new_objfile_chain)   (struct objfile *objfile);
static int stop_or_attach_thread_callback (const td_thrhandle_t *th, 
					   void *data);
static int wait_thread_callback           (const td_thrhandle_t *th, 
					   void *data);

static void
thread_db_new_objfile (struct objfile *objfile)
{
  td_err_e   ret;
  
  if (using_thread_db)			/* libthread already detected, and */
    goto quit;				/* thread target vector activated. */

  if (objfile == NULL)
    goto quit;	/* un-interesting object file */

  /* Initialize our "main prochandle" with the main inferior pid.  */
  main_prochandle.pid = PIDGET (inferior_pid);

  /* Now attempt to open a thread_db connection to the 
     thread library running in the child process.  */
  ret = p_td_ta_new (&main_prochandle, &main_threadagent);
  switch (ret) {
  default:
    warning ("Unexpected error initializing thread_db: %s", 
	     thr_err_string (ret));
    break;
  case TD_NOLIBTHREAD:	/* expected: no libthread in child process (yet) */
    break;	
  case TD_OK:		/* libthread detected in child: we go live now! */
    thread_db_push_target ();
    event_pid = inferior_pid;	/* for resume */

    /* Now stop everyone else, and attach any new threads you find.  */
    p_td_ta_thr_iter (main_threadagent, 
		      stop_or_attach_thread_callback,
		      (void *) 0,
		      TD_THR_ANY_STATE,
		      TD_THR_LOWEST_PRIORITY,
		      TD_SIGNO_MASK,
		      TD_THR_ANY_USER_FLAGS);

    /* Now go call wait on all the threads you've stopped:
       This allows us to absorb the SIGKILL event, and to make sure
       that the thread knows that it is stopped (Linux peculiarity).  */
    p_td_ta_thr_iter (main_threadagent, 
		      wait_thread_callback,
		      (void *) 0,
		      TD_THR_ANY_STATE,
		      TD_THR_LOWEST_PRIORITY,
		      TD_SIGNO_MASK,
		      TD_THR_ANY_USER_FLAGS);

    break;
  }
quit:
  if (target_new_objfile_chain)
    target_new_objfile_chain (objfile);
}


/* 

   LOCAL FUNCTION

   thread_db_alive     - test thread for "aliveness"

   SYNOPSIS

   static bool thread_db_alive (int pid);

   DESCRIPTION

   returns true if thread still active in inferior.

 */

static int
thread_db_alive (pid)
     int pid;
{
  if (is_thread (pid))		/* user-space (non-kernel) thread */
    {
      td_thrhandle_t th;
      td_err_e ret;

      pid = GET_THREAD (pid);
      if ((ret = p_td_ta_map_id2thr (main_threadagent, pid, &th)) != TD_OK)
	return 0;		/* thread not found */
      if ((ret = p_td_thr_validate (&th)) != TD_OK)
	return 0;		/* thread not valid */
      return 1;			/* known thread: return true */
    }
  else if (target_beneath->to_thread_alive)
    return target_beneath->to_thread_alive (pid);
  else
    return 0;		/* default to "not alive" (shouldn't happen anyway) */
}

/*
 * get_lwp_from_thread_handle
 */

static int	/* lwpid_t or pid_t */
get_lwp_from_thread_handle (th)
     td_thrhandle_t *th;
{
  td_thrinfo_t ti;
  td_err_e     ret;

  if ((ret = p_td_thr_get_info (th, &ti)) != TD_OK)
    error ("get_lwp_from_thread_handle: thr_get_info failed: %s", 
	   thr_err_string (ret));

  return ti.ti_lid;
}

/*
 * get_lwp_from_thread_id
 */

static int	/* lwpid_t or pid_t */
get_lwp_from_thread_id (tid)
     int tid;	/* thread_t? */
{
  td_thrhandle_t th;
  td_err_e       ret;

  if ((ret = p_td_ta_map_id2thr (main_threadagent, tid, &th)) != TD_OK)
    error ("get_lwp_from_thread_id: map_id2thr failed: %s", 
	   thr_err_string (ret));

  return get_lwp_from_thread_handle (&th);
}

/* 
 * pid_to_str has to handle user-space threads.
 * If not a user-space thread, then pass the request on to the 
 * underlying stratum if it can handle it: else call normal_pid_to_str.
 */

static char *
thread_db_pid_to_str (int pid)
{
  static char buf[100];
  td_thrhandle_t th;
  td_thrinfo_t ti;
  td_err_e ret;

  if (is_thread (pid))
    {
      if ((ret = p_td_ta_map_id2thr (main_threadagent, 
				     GET_THREAD (pid),
				     &th)) != TD_OK)
	error ("thread_db: map_id2thr failed: %s", thr_err_string (ret));

      if ((ret = p_td_thr_get_info (&th, &ti)) != TD_OK)
	error ("thread_db: thr_get_info failed: %s", thr_err_string (ret));

      if (ti.ti_state == TD_THR_ACTIVE &&
	  ti.ti_lid != 0)
	sprintf (buf, "Thread %d (LWP %d)", ti.ti_tid, ti.ti_lid);
      else
	sprintf (buf, "Thread %d (%s)", ti.ti_tid,
		 thr_state_string (ti.ti_state));
    }
  else if (GET_LWP (pid))
    sprintf (buf, "LWP %d", GET_LWP (pid));
  else return normal_pid_to_str (pid);

  return buf;
}

/* 
 * thread_db target vector functions:
 */

static void
thread_db_files_info (struct target_ops *tgt_vector)
{
  /* This function will be unnecessary in real life.  */
  printf_filtered ("thread_db stratum:\n");
  target_beneath->to_files_info (tgt_vector);
}

/* 
 * xfer_memory has to munge the inferior_pid before passing the call
 * down to the target layer.  
 */

static int
thread_db_xfer_memory (memaddr, myaddr, len, dowrite, target)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
     int dowrite;
     struct target_ops *target;	/* ignored */
{
  struct cleanup *old_chain;
  int ret;

  old_chain = save_inferior_pid ();

  if (is_thread (inferior_pid) ||
      !target_thread_alive (inferior_pid))
    {
      /* FIXME: use the LID/LWP, so that underlying process layer
	 can read memory from specific threads?  */
      inferior_pid = main_prochandle.pid;
    }

  ret = target_beneath->to_xfer_memory (memaddr, myaddr, len,
					   dowrite, target);
  do_cleanups (old_chain);
  return ret;
}

/* 
 * fetch_registers has to determine if inferior_pid is a user-space thread.
 * If so, we use the thread_db API to get the registers.
 * And if not, we call the underlying process stratum.
 */

static void
thread_db_fetch_registers (regno)
     int regno;
{
  td_thrhandle_t thandle;
  gdb_prfpregset_t fpregset;
  prgregset_t gregset;
  thread_t thread;
  td_err_e ret;

  if (!is_thread (inferior_pid))	/* kernel thread */
    {			/* pass the request on to the target underneath.  */
      target_beneath->to_fetch_registers (regno);
      return;
    }

  /* convert inferior_pid into a td_thrhandle_t */

  if ((thread = GET_THREAD (inferior_pid)) == 0)
    error ("fetch_registers:  thread == 0");

  if ((ret = p_td_ta_map_id2thr (main_threadagent, thread, &thandle)) != TD_OK)
    error ("fetch_registers: td_ta_map_id2thr: %s", thr_err_string (ret));

  /* Get the integer regs: 
     For the sparc, TD_PARTIALREG means that only i0->i7, l0->l7, 
     pc and sp are saved (by a thread context switch).  */
  if ((ret = p_td_thr_getgregs (&thandle, gregset)) != TD_OK &&
      ret != TD_PARTIALREG)
    error ("fetch_registers: td_thr_getgregs %s", thr_err_string (ret));

  /* And, now the fp regs */
  if ((ret = p_td_thr_getfpregs (&thandle, &fpregset)) != TD_OK &&
      ret != TD_NOFPREGS)
    error ("fetch_registers: td_thr_getfpregs %s", thr_err_string (ret));

/* Note that we must call supply_{g fp}regset *after* calling the td routines
   because the td routines call ps_lget* which affect the values stored in the
   registers array.  */

  supply_gregset (gregset);
  supply_fpregset (&fpregset);

}

/* 
 * store_registers has to determine if inferior_pid is a user-space thread.
 * If so, we use the thread_db API to get the registers.
 * And if not, we call the underlying process stratum.
 */

static void
thread_db_store_registers (regno)
     int regno;
{
  td_thrhandle_t thandle;
  gdb_prfpregset_t fpregset;
  prgregset_t  gregset;
  thread_t thread;
  td_err_e ret;

  if (!is_thread (inferior_pid))	/* Kernel thread: */
    {		/* pass the request on to the underlying target vector.  */
      target_beneath->to_store_registers (regno);
      return;
    }

  /* convert inferior_pid into a td_thrhandle_t */

  if ((thread = GET_THREAD (inferior_pid)) == 0)
    error ("store_registers: thread == 0");

  if ((ret = p_td_ta_map_id2thr (main_threadagent, thread, &thandle)) != TD_OK)
    error ("store_registers: td_ta_map_id2thr %s", thr_err_string (ret));

  if (regno != -1)
    {				/* Not writing all the regs */
      /* save new register value */
      /* MVS: I don't understand this... */
      char old_value[REGISTER_SIZE];

      memcpy (old_value, &registers[REGISTER_BYTE (regno)], REGISTER_SIZE);

      if ((ret = p_td_thr_getgregs (&thandle, gregset)) != TD_OK)
	error ("store_registers: td_thr_getgregs %s", thr_err_string (ret));
      if ((ret = p_td_thr_getfpregs (&thandle, &fpregset)) != TD_OK)
	error ("store_registers: td_thr_getfpregs %s", thr_err_string (ret));

      /* restore new register value */
      memcpy (&registers[REGISTER_BYTE (regno)], old_value, REGISTER_SIZE);

    }

  fill_gregset  (gregset, regno);
  fill_fpregset (&fpregset, regno);

  if ((ret = p_td_thr_setgregs (&thandle, gregset)) != TD_OK)
    error ("store_registers: td_thr_setgregs %s", thr_err_string (ret));
  if ((ret = p_td_thr_setfpregs (&thandle, &fpregset)) != TD_OK &&
      ret != TD_NOFPREGS)
    error ("store_registers: td_thr_setfpregs %s", thr_err_string (ret));
}

static void
handle_new_thread (tid, lid, verbose)
     int tid;	/* user thread id */
     int lid;	/* kernel thread id */
     int verbose;
{
  int gdb_pid = BUILD_THREAD (tid, main_prochandle.pid);
  int wait_pid, wait_status;

  if (verbose)
    printf_filtered ("[New %s]\n", target_pid_to_str (gdb_pid));
  add_thread (gdb_pid);

  if (lid != main_prochandle.pid)
    {
      attach_thread (lid);
      /* According to the Eric Paire model, we now have to send
	 the restart signal to the new thread -- however, empirically,
	 I do not find that to be necessary.  */
      attach_pid = lid;
    }
}

static void
test_for_new_thread (tid, lid, verbose)
     int tid;
     int lid;
     int verbose;
{
  if (!in_thread_list (BUILD_THREAD (tid, main_prochandle.pid)))
    handle_new_thread (tid, lid, verbose);
}

/* 
 * Callback function that gets called once per USER thread 
 * (i.e., not kernel) thread by td_ta_thr_iter.
 */

static int
find_new_threads_callback (th, ignored)
     const td_thrhandle_t *th;
     void *ignored;
{
  td_thrinfo_t ti;
  td_err_e     ret;

  if ((ret = p_td_thr_get_info (th, &ti)) != TD_OK)
    {
      warning ("find_new_threads_callback: %s", thr_err_string (ret));
      return -1;		/* bail out, get_info failed. */
    }

  /* FIXME: 
     As things now stand, this should never detect a new thread.
     But if it does, we could be in trouble because we aren't calling
     wait_thread_callback for it.  */
  test_for_new_thread (ti.ti_tid, ti.ti_lid, 0);
  return 0;
}

/* 
 * find_new_threads uses the thread_db iterator function to discover
 * user-space threads.  Then if the underlying process stratum has a
 * find_new_threads method, we call that too.
 */

static void
thread_db_find_new_threads ()
{
  if (inferior_pid == -1)	/* FIXME: still necessary? */
    {
      printf_filtered ("No process.\n");
      return;
    }
  p_td_ta_thr_iter (main_threadagent, 
		    find_new_threads_callback, 
		    (void *) 0, 
		    TD_THR_ANY_STATE, 
		    TD_THR_LOWEST_PRIORITY,
		    TD_SIGNO_MASK, 
		    TD_THR_ANY_USER_FLAGS);
  if (target_beneath->to_find_new_threads)
    target_beneath->to_find_new_threads ();
}

/*
 * Resume all threads, or resume a single thread.
 * If step is true, then single-step the appropriate thread
 * (or single-step inferior_pid, but continue everyone else).
 * If signo is true, then send that signal to at least one thread.
 */

/*
 * This function is called once for each thread before resuming.
 * It sends continue (no step, and no signal) to each thread except
 *   the main thread, and
 *   the event thread (the one that stopped at a breakpoint etc.)
 *
 * The event thread is handled separately so that it can be sent
 * the stepping and signal args with which target_resume was called.
 *
 * The main thread is resumed last, so that the thread_db proc_service
 * callbacks will still work during the iterator function.
 */

static int
resume_thread_callback (th, data)
     const td_thrhandle_t *th;
     void *data;
{
  td_thrinfo_t ti;
  td_err_e     ret;

  if ((ret = p_td_thr_get_info (th, &ti)) != TD_OK)
    {
      warning ("resume_thread_callback: %s", thr_err_string (ret));
      return -1;		/* bail out, get_info failed. */
    }
  /* FIXME: 
     As things now stand, this should never detect a new thread.
     But if it does, we could be in trouble because we aren't calling
     wait_thread_callback for it.  */
  test_for_new_thread (ti.ti_tid, ti.ti_lid, 1);

  if (ti.ti_lid != main_prochandle.pid &&
      ti.ti_lid != event_pid)
    {
      /* Unconditionally continue the thread with no signal.
	 Only the event thread will get a signal of any kind.  */

      target_beneath->to_resume (ti.ti_lid, 0, 0);
    }
  return 0;
}

static int
new_resume_thread_callback (thread, data)
     threadinfo *thread;
     void *data;
{
  if (thread->lid != event_pid &&
      thread->lid != main_prochandle.pid)
    {
      /* Unconditionally continue the thread with no signal (for now).  */

      target_beneath->to_resume (thread->lid, 0, 0);
    }
  return 0;
}

static int last_resume_pid;
static int last_resume_step;
static int last_resume_signo;

static void
thread_db_resume (pid, step, signo)
     int pid;
     int step;
     enum target_signal signo;
{
  last_resume_pid   = pid;
  last_resume_step  = step;
  last_resume_signo = signo;

  /* resuming a specific pid? */
  if (pid != -1)
    {
      if (is_thread (pid))
	pid = get_lwp_from_thread_id (GET_THREAD (pid));
      else if (GET_LWP (pid))
	pid = GET_LWP (pid);
    }

  /* Apparently the interpretation of 'pid' is dependent on 'step':
     If step is true, then a specific pid means 'step only this pid'.
     But if step is not true, then pid means 'continue ALL pids, but
     give the signal only to this one'.  */
  if (pid != -1 && step)
    {
      /* FIXME: is this gonna work in all circumstances? */
      target_beneath->to_resume (pid, step, signo);
    }
  else
    {
      /* 1) Continue all threads except the event thread and the main thread.
	 2) resume the event thread with step and signo.
	 3) If event thread != main thread, continue the main thread.

	 Note: order of 2 and 3 may need to be reversed.  */

      threadlist_iter (new_resume_thread_callback, 
			(void *) 0, 
			TD_THR_ANY_STATE, 
			TD_THR_ANY_TYPE);
      /* now resume event thread, and if necessary also main thread. */
      if (event_pid)
	{
	  target_beneath->to_resume (event_pid, step, signo);
	}
      if (event_pid != main_prochandle.pid)
	{
	  target_beneath->to_resume (main_prochandle.pid, 0, 0);
	}
    }
}

/* All new threads will be attached.
   All previously known threads will be stopped using kill (SIGKILL).  */

static int
stop_or_attach_thread_callback (const td_thrhandle_t *th, void *data)
{
  td_thrinfo_t ti;
  td_err_e     ret;
  int          gdb_pid;
  int on_off = 1;

  if ((ret = p_td_thr_get_info (th, &ti)) != TD_OK)
    {
      warning ("stop_or_attach_thread_callback: %s", thr_err_string (ret));
      return -1;		/* bail out, get_info failed. */
    }

  /* First add it to our internal list.  
     We build this list anew at every wait event.  */
  insert_thread (ti.ti_tid, ti.ti_lid, ti.ti_state, ti.ti_type);
  /* Now: if we've already seen it, stop it, else add it and attach it.  */
  gdb_pid = BUILD_THREAD (ti.ti_tid, main_prochandle.pid);
  if (!in_thread_list (gdb_pid))	/* new thread */
    {
      handle_new_thread (ti.ti_tid, ti.ti_lid, 1);
      /* Enable thread events */
      if (p_td_thr_event_enable)
	if ((ret = p_td_thr_event_enable (th, on_off)) != TD_OK)
	  warning ("stop_or_attach_thread: %s", thr_err_string (ret));
    }
  else if (ti.ti_lid != event_pid &&
	   ti.ti_lid != main_prochandle.pid)
    {
      ret = (td_err_e) kill (ti.ti_lid, SIGSTOP);
    }

  return 0;
}
     
/*
 * Wait for signal N from pid PID.
 * If wait returns any other signals, put them back before returning.
 */

static void
wait_for_stop (pid)
     int pid;
{
  int i;
  int retpid;
  int status;

  /* Array of wait/signal status */
  /* FIXME: wrong data structure, we need a queue.
     Realtime signals may be delivered more than once.  
     And at that, we really can't handle them (see below).  */
#if defined (NSIG)
  static int   wstatus [NSIG];
#elif defined (_NSIG)
  static int   wstatus [_NSIG];
#else
#error No definition for number of signals!
#endif

  /* clear wait/status list */
  memset (&wstatus, 0, sizeof (wstatus));

  /* Now look for SIGSTOP event on all threads except event thread.  */
  do {
    errno = 0;
    if (pid == main_prochandle.pid)
      retpid = waitpid (pid, &status, 0);
    else
      retpid = waitpid (pid, &status, __WCLONE);

    if (retpid > 0)
      if (WSTOPSIG (status) == SIGSTOP)
	{
	  /* Got the SIGSTOP event we're looking for.
	     Throw it away, and throw any other events back!  */
	  for (i = 0; i < sizeof(wstatus) / sizeof (wstatus[0]); i++)
	    if (wstatus[i])
	      if (i != SIGSTOP)
		{
		  kill (retpid, i);
		}
	  break;	/* all done */
	}
      else
	{
	  int signo;
	  /* Oops, got an event other than SIGSTOP.
	     Save it, and throw it back after we find the SIGSTOP event.  */
	  
	  /* FIXME (how?)  This method is going to fail for realtime
	     signals, which cannot be put back simply by using kill.  */

	  if (WIFEXITED (status))
	    error ("Ack!  Thread Exited event.  What do I do now???");
	  else if (WIFSTOPPED (status))
	    signo = WSTOPSIG (status);
	  else
	    signo = WTERMSIG (status);
	  
	  /* If a thread other than the event thread has hit a GDB
	     breakpoint (as opposed to some random trap signal), then
	     just arrange for it to hit it again later.  Back up the
	     PC if necessary.  Don't forward the SIGTRAP signal to
	     the thread.  We will handle the current event, eventually
	     we will resume all the threads, and this one will get
	     it's breakpoint trap again.

	     If we do not do this, then we run the risk that the user
	     will delete or disable the breakpoint, but the thread will
	     have already tripped on it.  */

	  if (retpid != event_pid &&
	      signo == SIGTRAP &&
	      breakpoint_inserted_here_p (read_pc_pid (retpid) - 
					  DECR_PC_AFTER_BREAK))
	    {
	      /* Set the pc to before the trap and DO NOT re-send the signal */
	      if (DECR_PC_AFTER_BREAK)
		write_pc_pid (read_pc_pid (retpid) - DECR_PC_AFTER_BREAK,
			      retpid);
	    }

	  /* Since SIGINT gets forwarded to the entire process group
	     (in the case where ^C is typed at the tty / console), 
	     just ignore all SIGINTs from other than the event thread.  */
	  else if (retpid != event_pid && signo == SIGINT)
	    { /* do nothing.  Signal will disappear into oblivion!  */
	      ;
	    }

	  else /* This is some random signal other than a breakpoint. */
	    {
	      wstatus [signo] = 1;
	    }
	  child_resume (retpid, 0, TARGET_SIGNAL_0);
	  continue;
	}

  } while (errno == 0 || errno == EINTR);
}

/*
 * wait_thread_callback
 *
 * Calls waitpid for each thread, repeatedly if necessary, until
 * SIGSTOP is returned.  Afterward, if any other signals were returned
 * by waitpid, return them to the thread's pending queue by calling kill.
 */

static int
wait_thread_callback (const td_thrhandle_t *th, void *data)
{
  td_thrinfo_t ti;
  td_err_e     ret;

  if ((ret = p_td_thr_get_info (th, &ti)) != TD_OK)
    {
      warning ("wait_thread_callback: %s", thr_err_string (ret));
      return -1;		/* bail out, get_info failed. */
    }

  /* This callback to act on all threads except the event thread: */
  if (ti.ti_lid == event_pid || 	/* no need to wait (no sigstop) */
      ti.ti_lid == main_prochandle.pid)	/* no need to wait (already waited) */
    return 0;	/* don't wait on the event thread.  */

  wait_for_stop (ti.ti_lid);
  return 0;	/* finished: next thread. */
}

static int
new_wait_thread_callback (thread, data)
     threadinfo *thread;
     void *data;
{
  /* don't wait on the event thread -- it's already stopped and waited.  
     Ditto the main thread.  */
  if (thread->lid != event_pid &&
      thread->lid != main_prochandle.pid)
    {
      wait_for_stop (thread->lid);
    }
  return 0;
}

/* 
 * Wait for any thread to stop, by calling the underlying wait method.
 * The PID returned by the underlying target may be a kernel thread,
 * in which case we will want to convert it to the corresponding
 * user-space thread.  
 */

static int
thread_db_wait (int pid, struct target_waitstatus *ourstatus)
{
  td_thrhandle_t thandle;
  td_thrinfo_t ti;
  td_err_e ret;
  lwpid_t lwp;
  int retpid;
  int status;
  int save_errno;

  /* OK, we're about to wait for an event from the running inferior.
     Make sure we're ignoring the right signals.  */

  check_all_signal_numbers ();	/* see if magic signals changed. */

  event_pid = 0;
  attach_pid = 0;

  /* FIXME: should I do the wait right here inline?  */
#if 0
  if (pid == -1)
    lwp = -1;
  else
    lwp = get_lwp_from_thread_id (GET_THREAD (pid));
#endif


  save_errno = linux_child_wait (-1, &retpid, &status);
  store_waitstatus (ourstatus, status);

  /* Thread ID is irrelevant if the target process exited.
     FIXME: do I have any killing to do?
     Can I get this event mistakenly from a thread?  */
  if (ourstatus->kind == TARGET_WAITKIND_EXITED)
    return retpid;

  /* OK, we got an event of interest.
     Go stop all threads and look for new ones.
     FIXME: maybe don't do this for the restart signal?  Optimization...  */
  event_pid = retpid;

  /* If the last call to resume was for a specific thread, then we don't
     need to stop everyone else: they should already be stopped.  */
  if (last_resume_step == 0 || last_resume_pid == -1)
    {
      /* Main thread must be stopped before calling the iterator.  */
      if (retpid != main_prochandle.pid)
	{
	  kill (main_prochandle.pid, SIGSTOP);
	  wait_for_stop (main_prochandle.pid);
	}

      empty_threadlist ();
      /* Now stop everyone else, and attach any new threads you find.  */
      p_td_ta_thr_iter (main_threadagent, 
			stop_or_attach_thread_callback,
			(void *) 0,
			TD_THR_ANY_STATE,
			TD_THR_LOWEST_PRIORITY,
			TD_SIGNO_MASK,
			TD_THR_ANY_USER_FLAGS);

      /* Now go call wait on all the threads we've stopped:
	 This allows us to absorb the SIGKILL event, and to make sure
	 that the thread knows that it is stopped (Linux peculiarity).  */

      threadlist_iter (new_wait_thread_callback, 
		       (void *) 0,
		       TD_THR_ANY_STATE, 
		       TD_THR_ANY_TYPE);
    }

  /* Convert the kernel thread id to the corresponding thread id.  */

  /* If the process layer does not furnish an lwp,
     then perhaps the returned pid IS the lwp... */
  if ((lwp = GET_LWP (retpid)) == 0)
    lwp = retpid;

  if ((ret = p_td_ta_map_lwp2thr (main_threadagent, lwp, &thandle)) != TD_OK)
    return retpid;	/* LWP is not mapped onto a user-space thread. */

  if ((ret = p_td_thr_validate (&thandle)) != TD_OK)
    return retpid;	/* LWP is not mapped onto a valid thread. */

  if ((ret = p_td_thr_get_info (&thandle, &ti)) != TD_OK)
    {
      warning ("thread_db: thr_get_info failed ('%s')", thr_err_string (ret));
      return retpid;
    }

  retpid = BUILD_THREAD (ti.ti_tid, main_prochandle.pid);
  /* If this is a new user thread, notify GDB about it.  */
  if (!in_thread_list (retpid))
    {
      printf_filtered ("[New %s]\n", target_pid_to_str (retpid));
      add_thread (retpid);
    }

#if 0
  /* Now detect if this is a thread creation/deletion event: */
  check_for_thread_event (ourstatus, retpid);
#endif
  return retpid;
}

/* 
 * kill has to call the underlying kill.
 * FIXME: I'm not sure if it's necessary to check inferior_pid any more,
 * but we might need to fix inferior_pid up if it's a user thread.
 */

static int
kill_thread_callback (th, data)
     td_thrhandle_t *th;
     void *data;
{
  td_thrinfo_t ti;
  td_err_e     ret;

  /* Fixme: 
     For Linux, threads may need to be waited.  */
  if ((ret = p_td_thr_get_info (th, &ti)) != TD_OK)
    {
      warning ("kill_thread_callback: %s", thr_err_string (ret));
      return -1;		/* bail out, get_info failed. */
    }

  if (ti.ti_lid != main_prochandle.pid)
    {
      kill (ti.ti_lid, SIGKILL);
    }
  return 0;
}


static void thread_db_kill (void)
{
  int rpid;
  int status;

  /* Fixme: 
     For Linux, threads may need to be waited.  */
  if (inferior_pid != 0)
    {
      /* Go kill the children first.  Save the main thread for last. */
      p_td_ta_thr_iter (main_threadagent, 
			kill_thread_callback, 
			(void *) 0, 
			TD_THR_ANY_STATE,
			TD_THR_LOWEST_PRIORITY,
			TD_SIGNO_MASK,
			TD_THR_ANY_USER_FLAGS);

      /* Turn off thread_db event-reporting API *before* killing the
	 main thread, since this operation requires child memory access.
	 Can't move this into thread_db_unpush target because then 
	 detach would not work.  */
      disable_thread_event_reporting (main_threadagent);

      inferior_pid = main_prochandle.pid;

      /* 
       * Since both procfs_kill and ptrace_kill call target_mourn, 
       * it should be sufficient for me to call one of them.
       * That will result in my mourn being called, which will both
       * unpush me and call the underlying mourn.
       */
      target_beneath->to_kill ();
    }

  /* Wait for all threads. */
  /* FIXME: need a universal wait_for_signal func? */
  do
    {
      rpid = waitpid (-1, &status, __WCLONE | WNOHANG);
    }
  while (rpid > 0 || errno == EINTR);

  do
    {
      rpid = waitpid (-1, &status, WNOHANG);
    }
  while (rpid > 0 || errno == EINTR);
}

/* 
 * Mourn has to remove us from the target stack, 
 * and then call the underlying mourn.
 */

static void thread_db_mourn_inferior (void)
{
  thread_db_unpush_target ();
  target_mourn_inferior ();	/* call the underlying mourn */
}

/* 
 * Detach has to remove us from the target stack, 
 * and then call the underlying detach.
 *
 * But first, it has to detach all the cloned threads!
 */

static int
detach_thread_callback (th, data)
     td_thrhandle_t *th;
     void *data;
{
  /* Called once per thread.  */
  td_thrinfo_t ti;
  td_err_e     ret;

  if ((ret = p_td_thr_get_info (th, &ti)) != TD_OK)
    {
      warning ("detach_thread_callback: %s", thr_err_string (ret));
      return -1;                /* bail out, get_info failed. */
    }

  if (!in_thread_list (BUILD_THREAD (ti.ti_tid, main_prochandle.pid)))
    return 0;	/* apparently we don't know this one.  */

  /* Save main thread for last, or the iterator will fail! */
  if (ti.ti_lid != main_prochandle.pid)
    {
      struct cleanup *old_chain;
      int off = 0;

      /* Time to detach this thread. 
	 First disable thread_db event reporting for the thread.  */
      if (p_td_thr_event_enable &&
	  (ret = p_td_thr_event_enable (th, off)) != TD_OK)
	{
	  warning ("detach_thread_callback: %s\n", thr_err_string (ret));
	  return 0;
	}

      /* Now cancel any pending SIGTRAPS.  FIXME!  */

      /* Call underlying detach method.  FIXME just detach it.  */
      old_chain = save_inferior_pid ();
      inferior_pid = ti.ti_lid;
      detach (TARGET_SIGNAL_0);
      do_cleanups (old_chain);
    }
  return 0;
}

static void
thread_db_detach (char *args, int from_tty)
{
  td_err_e ret;

  if ((ret = p_td_ta_thr_iter (main_threadagent, 
			       detach_thread_callback, 
			       (void *) 0, 
			       TD_THR_ANY_STATE,
			       TD_THR_LOWEST_PRIORITY,
			       TD_SIGNO_MASK,
			       TD_THR_ANY_USER_FLAGS))
      != TD_OK)
    warning ("detach (thr_iter): %s", thr_err_string (ret));

  /* Turn off thread_db event-reporting API 
     (before detaching the main thread) */
  disable_thread_event_reporting (main_threadagent);

  thread_db_unpush_target ();

  /* above call nullifies target_beneath, so don't use that! */
  inferior_pid = PIDGET (inferior_pid);
  target_detach (args, from_tty);
}


/* 
 * We never want to actually create the inferior!
 *
 * If this is ever called, it means we were on the target stack
 * when the user said "run".  But we don't want to be on the new
 * inferior's target stack until the thread_db / libthread
 * connection is ready to be made.
 *
 * So, what shall we do?
 * Unpush ourselves from the stack, and then invoke
 * find_default_create_inferior, which will invoke the
 * appropriate process_stratum target to do the create.
 */

static void
thread_db_create_inferior (exec_file, allargs, env)
     char *exec_file;
     char *allargs;
     char **env;
{
  thread_db_unpush_target ();
  find_default_create_inferior (exec_file, allargs, env);
}

/* 
 * Thread_db target vector initializer.
 */

void
init_thread_db_ops ()
{
  thread_db_ops.to_shortname        = "multi-thread";
  thread_db_ops.to_longname         = "multi-threaded child process.";
  thread_db_ops.to_doc              = "Threads and pthreads support.";
  thread_db_ops.to_files_info       = thread_db_files_info;
  thread_db_ops.to_create_inferior  = thread_db_create_inferior;
  thread_db_ops.to_detach           = thread_db_detach;
  thread_db_ops.to_wait             = thread_db_wait;
  thread_db_ops.to_resume           = thread_db_resume;
  thread_db_ops.to_mourn_inferior   = thread_db_mourn_inferior;
  thread_db_ops.to_kill             = thread_db_kill;
  thread_db_ops.to_xfer_memory      = thread_db_xfer_memory;
  thread_db_ops.to_fetch_registers  = thread_db_fetch_registers;
  thread_db_ops.to_store_registers  = thread_db_store_registers;
  thread_db_ops.to_thread_alive     = thread_db_alive;
  thread_db_ops.to_find_new_threads = thread_db_find_new_threads;
  thread_db_ops.to_pid_to_str       = thread_db_pid_to_str;
  thread_db_ops.to_stratum          = thread_stratum;
  thread_db_ops.to_has_thread_control = tc_schedlock;
  thread_db_ops.to_magic            = OPS_MAGIC;
}
#endif	/* HAVE_STDINT_H */

/*
 * Module constructor / initializer function.
 * If connection to thread_db dynamic library is successful, 
 * then initialize this module's target vectors and the 
 * new_objfile hook.
 */


void
_initialize_thread_db ()
{
#ifdef HAVE_STDINT_H	/* stub out entire module, leave initializer empty */
  if (init_thread_db_library ())
    {
      init_thread_db_ops ();
      add_target (&thread_db_ops);
      /*
       * Hook up to the new_objfile event.
       * If someone is already there, arrange for him to be called
       * after we are.
       */
      target_new_objfile_chain = target_new_objfile_hook;
      target_new_objfile_hook  = thread_db_new_objfile;
    }
#endif	/* HAVE_STDINT_H */
}

