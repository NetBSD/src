/* Low level interface for debugging AIX 4.3+ pthreads.

   Copyright (C) 1999-2020 Free Software Foundation, Inc.
   Written by Nick Duffek <nsd@redhat.com>.

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


/* This module uses the libpthdebug.a library provided by AIX 4.3+ for
   debugging pthread applications.

   Some name prefix conventions:
     pthdb_	provided by libpthdebug.a
     pdc_	callbacks that this module provides to libpthdebug.a
     pd_	variables or functions interfacing with libpthdebug.a

   libpthdebug peculiarities:

     - pthdb_ptid_pthread() is prototyped in <sys/pthdebug.h>, but
       it's not documented, and after several calls it stops working
       and causes other libpthdebug functions to fail.

     - pthdb_tid_pthread() doesn't always work after
       pthdb_session_update(), but it does work after cycling through
       all threads using pthdb_pthread().

     */

#include "defs.h"
#include "gdbthread.h"
#include "target.h"
#include "inferior.h"
#include "regcache.h"
#include "gdbcmd.h"
#include "ppc-tdep.h"
#include "observable.h"
#include "objfiles.h"

#include <procinfo.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sched.h>
#include <sys/pthdebug.h>

#if !HAVE_DECL_GETTHRDS
extern int getthrds (pid_t, struct thrdsinfo64 *, int, tid_t *, int);
#endif

/* Whether to emit debugging output.  */
static bool debug_aix_thread;

/* In AIX 5.1, functions use pthdb_tid_t instead of tid_t.  */
#ifndef PTHDB_VERSION_3
#define pthdb_tid_t	tid_t
#endif

/* Return whether to treat PID as a debuggable thread id.  */

#define PD_TID(ptid)	(pd_active && ptid.tid () != 0)

/* pthdb_user_t value that we pass to pthdb functions.  0 causes
   PTHDB_BAD_USER errors, so use 1.  */

#define PD_USER	1

/* Success and failure values returned by pthdb callbacks.  */

#define PDC_SUCCESS	PTHDB_SUCCESS
#define PDC_FAILURE	PTHDB_CALLBACK

/* Private data attached to each element in GDB's thread list.  */

struct aix_thread_info : public private_thread_info
{
  pthdb_pthread_t pdtid;	 /* thread's libpthdebug id */
  pthdb_tid_t tid;			/* kernel thread id */
};

/* Return the aix_thread_info attached to THREAD.  */

static aix_thread_info *
get_aix_thread_info (thread_info *thread)
{
  return static_cast<aix_thread_info *> (thread->priv.get ());
}

/* Information about a thread of which libpthdebug is aware.  */

struct pd_thread {
  pthdb_pthread_t pdtid;
  pthread_t pthid;
  pthdb_tid_t tid;
};

/* This module's target-specific operations, active while pd_able is true.  */

static const target_info aix_thread_target_info = {
  "aix-threads",
  N_("AIX pthread support"),
  N_("AIX pthread support")
};

class aix_thread_target final : public target_ops
{
public:
  const target_info &info () const override
  { return aix_thread_target_info; }

  strata stratum () const override { return thread_stratum; }

  void detach (inferior *, int) override;
  void resume (ptid_t, int, enum gdb_signal) override;
  ptid_t wait (ptid_t, struct target_waitstatus *, int) override;

  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;

  void mourn_inferior () override;

  bool thread_alive (ptid_t ptid) override;

  std::string pid_to_str (ptid_t) override;

  const char *extra_thread_info (struct thread_info *) override;

  ptid_t get_ada_task_ptid (long lwp, long thread) override;
};

static aix_thread_target aix_thread_ops;

/* Address of the function that libpthread will call when libpthdebug
   is ready to be initialized.  */

static CORE_ADDR pd_brk_addr;

/* Whether the current application is debuggable by pthdb.  */

static int pd_able = 0;

/* Whether a threaded application is being debugged.  */

static int pd_active = 0;

/* Whether the current architecture is 64-bit.  
   Only valid when pd_able is true.  */

static int arch64;

/* Forward declarations for pthdb callbacks.  */

static int pdc_symbol_addrs (pthdb_user_t, pthdb_symbol_t *, int);
static int pdc_read_data (pthdb_user_t, void *, pthdb_addr_t, size_t);
static int pdc_write_data (pthdb_user_t, void *, pthdb_addr_t, size_t);
static int pdc_read_regs (pthdb_user_t user, pthdb_tid_t tid,
			  unsigned long long flags, 
			  pthdb_context_t *context);
static int pdc_write_regs (pthdb_user_t user, pthdb_tid_t tid,
			   unsigned long long flags, 
			   pthdb_context_t *context);
static int pdc_alloc (pthdb_user_t, size_t, void **);
static int pdc_realloc (pthdb_user_t, void *, size_t, void **);
static int pdc_dealloc (pthdb_user_t, void *);

/* pthdb callbacks.  */

static pthdb_callbacks_t pd_callbacks = {
  pdc_symbol_addrs,
  pdc_read_data,
  pdc_write_data,
  pdc_read_regs,
  pdc_write_regs,
  pdc_alloc,
  pdc_realloc,
  pdc_dealloc,
  NULL
};

/* Current pthdb session.  */

static pthdb_session_t pd_session;

/* Return a printable representation of pthdebug function return
   STATUS.  */

static const char *
pd_status2str (int status)
{
  switch (status)
    {
    case PTHDB_SUCCESS:		return "SUCCESS";
    case PTHDB_NOSYS:		return "NOSYS";
    case PTHDB_NOTSUP:		return "NOTSUP";
    case PTHDB_BAD_VERSION:	return "BAD_VERSION";
    case PTHDB_BAD_USER:	return "BAD_USER";
    case PTHDB_BAD_SESSION:	return "BAD_SESSION";
    case PTHDB_BAD_MODE:	return "BAD_MODE";
    case PTHDB_BAD_FLAGS:	return "BAD_FLAGS";
    case PTHDB_BAD_CALLBACK:	return "BAD_CALLBACK";
    case PTHDB_BAD_POINTER:	return "BAD_POINTER";
    case PTHDB_BAD_CMD:		return "BAD_CMD";
    case PTHDB_BAD_PTHREAD:	return "BAD_PTHREAD";
    case PTHDB_BAD_ATTR:	return "BAD_ATTR";
    case PTHDB_BAD_MUTEX:	return "BAD_MUTEX";
    case PTHDB_BAD_MUTEXATTR:	return "BAD_MUTEXATTR";
    case PTHDB_BAD_COND:	return "BAD_COND";
    case PTHDB_BAD_CONDATTR:	return "BAD_CONDATTR";
    case PTHDB_BAD_RWLOCK:	return "BAD_RWLOCK";
    case PTHDB_BAD_RWLOCKATTR:	return "BAD_RWLOCKATTR";
    case PTHDB_BAD_KEY:		return "BAD_KEY";
    case PTHDB_BAD_PTID:	return "BAD_PTID";
    case PTHDB_BAD_TID:		return "BAD_TID";
    case PTHDB_CALLBACK:	return "CALLBACK";
    case PTHDB_CONTEXT:		return "CONTEXT";
    case PTHDB_HELD:		return "HELD";
    case PTHDB_NOT_HELD:	return "NOT_HELD";
    case PTHDB_MEMORY:		return "MEMORY";
    case PTHDB_NOT_PTHREADED:	return "NOT_PTHREADED";
    case PTHDB_SYMBOL:		return "SYMBOL";
    case PTHDB_NOT_AVAIL:	return "NOT_AVAIL";
    case PTHDB_INTERNAL:	return "INTERNAL";
    default:			return "UNKNOWN";
    }
}

/* A call to ptrace(REQ, ID, ...) just returned RET.  Check for
   exceptional conditions and either return nonlocally or else return
   1 for success and 0 for failure.  */

static int
ptrace_check (int req, int id, int ret)
{
  if (ret == 0 && !errno)
    return 1;

  /* According to ptrace(2), ptrace may fail with EPERM if "the
     Identifier parameter corresponds to a kernel thread which is
     stopped in kernel mode and whose computational state cannot be
     read or written."  This happens quite often with register reads.  */

  switch (req)
    {
    case PTT_READ_GPRS:
    case PTT_READ_FPRS:
    case PTT_READ_SPRS:
      if (ret == -1 && errno == EPERM)
	{
	  if (debug_aix_thread)
	    fprintf_unfiltered (gdb_stdlog, 
				"ptrace (%d, %d) = %d (errno = %d)\n",
				req, id, ret, errno);
	  return ret == -1 ? 0 : 1;
	}
      break;
    }
  error (_("aix-thread: ptrace (%d, %d) returned %d (errno = %d %s)"),
	 req, id, ret, errno, safe_strerror (errno));
  return 0;  /* Not reached.  */
}

/* Call ptracex (REQ, ID, ADDR, DATA, BUF) or
   ptrace64 (REQ, ID, ADDR, DATA, BUF) if HAVE_PTRACE64.
   Return success.  */

#ifdef HAVE_PTRACE64
# define ptracex(request, pid, addr, data, buf) \
	 ptrace64 (request, pid, addr, data, buf)
#endif

static int
ptrace64aix (int req, int id, long long addr, int data, int *buf)
{
  errno = 0;
  return ptrace_check (req, id, ptracex (req, id, addr, data, buf));
}

/* Call ptrace (REQ, ID, ADDR, DATA, BUF) or
   ptrace64 (REQ, ID, ADDR, DATA, BUF) if HAVE_PTRACE64.
   Return success.  */

#ifdef HAVE_PTRACE64
# define ptrace(request, pid, addr, data, buf) \
	 ptrace64 (request, pid, addr, data, buf)
# define addr_ptr long long
#else
# define addr_ptr int *
#endif

static int
ptrace32 (int req, int id, addr_ptr addr, int data, int *buf)
{
  errno = 0;
  return ptrace_check (req, id, 
		       ptrace (req, id, addr, data, buf));
}

/* If *PIDP is a composite process/thread id, convert it to a
   process id.  */

static void
pid_to_prc (ptid_t *ptidp)
{
  ptid_t ptid;

  ptid = *ptidp;
  if (PD_TID (ptid))
    *ptidp = ptid_t (ptid.pid ());
}

/* pthdb callback: for <i> from 0 to COUNT, set SYMBOLS[<i>].addr to
   the address of SYMBOLS[<i>].name.  */

static int
pdc_symbol_addrs (pthdb_user_t user, pthdb_symbol_t *symbols, int count)
{
  struct bound_minimal_symbol ms;
  int i;
  char *name;

  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog,
      "pdc_symbol_addrs (user = %ld, symbols = 0x%lx, count = %d)\n",
      user, (long) symbols, count);

  for (i = 0; i < count; i++)
    {
      name = symbols[i].name;
      if (debug_aix_thread)
	fprintf_unfiltered (gdb_stdlog, 
			    "  symbols[%d].name = \"%s\"\n", i, name);

      if (!*name)
	symbols[i].addr = 0;
      else
	{
	  ms = lookup_minimal_symbol (name, NULL, NULL);
	  if (ms.minsym == NULL)
	    {
	      if (debug_aix_thread)
		fprintf_unfiltered (gdb_stdlog, " returning PDC_FAILURE\n");
	      return PDC_FAILURE;
	    }
	  symbols[i].addr = BMSYMBOL_VALUE_ADDRESS (ms);
	}
      if (debug_aix_thread)
	fprintf_unfiltered (gdb_stdlog, "  symbols[%d].addr = %s\n",
			    i, hex_string (symbols[i].addr));
    }
  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog, " returning PDC_SUCCESS\n");
  return PDC_SUCCESS;
}

/* Read registers call back function should be able to read the
   context information of a debuggee kernel thread from an active
   process or from a core file.  The information should be formatted
   in context64 form for both 32-bit and 64-bit process.  
   If successful return 0, else non-zero is returned.  */

static int
pdc_read_regs (pthdb_user_t user, 
	       pthdb_tid_t tid,
	       unsigned long long flags,
	       pthdb_context_t *context)
{
  /* This function doesn't appear to be used, so we could probably
   just return 0 here.  HOWEVER, if it is not defined, the OS will
   complain and several thread debug functions will fail.  In case
   this is needed, I have implemented what I think it should do,
   however this code is untested.  */

  uint64_t gprs64[ppc_num_gprs];
  uint32_t gprs32[ppc_num_gprs];
  double fprs[ppc_num_fprs];
  struct ptxsprs sprs64;
  struct ptsprs sprs32;
  
  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog, "pdc_read_regs tid=%d flags=%s\n",
                        (int) tid, hex_string (flags));

  /* General-purpose registers.  */
  if (flags & PTHDB_FLAG_GPRS)
    {
      if (arch64)
	{
	  if (!ptrace64aix (PTT_READ_GPRS, tid, 
			    (unsigned long) gprs64, 0, NULL))
	    memset (gprs64, 0, sizeof (gprs64));
	  memcpy (context->gpr, gprs64, sizeof(gprs64));
	}
      else
	{
	  if (!ptrace32 (PTT_READ_GPRS, tid, (uintptr_t) gprs32, 0, NULL))
	    memset (gprs32, 0, sizeof (gprs32));
	  memcpy (context->gpr, gprs32, sizeof(gprs32));
	}
    }

  /* Floating-point registers.  */
  if (flags & PTHDB_FLAG_FPRS)
    {
      if (!ptrace32 (PTT_READ_FPRS, tid, (uintptr_t) fprs, 0, NULL))
	memset (fprs, 0, sizeof (fprs));
      memcpy (context->fpr, fprs, sizeof(fprs));
    }

  /* Special-purpose registers.  */
  if (flags & PTHDB_FLAG_SPRS)
    {
      if (arch64)
	{
	  if (!ptrace64aix (PTT_READ_SPRS, tid, 
			    (unsigned long) &sprs64, 0, NULL))
	    memset (&sprs64, 0, sizeof (sprs64));
      	  memcpy (&context->msr, &sprs64, sizeof(sprs64));
	}
      else
	{
	  if (!ptrace32 (PTT_READ_SPRS, tid, (uintptr_t) &sprs32, 0, NULL))
	    memset (&sprs32, 0, sizeof (sprs32));
      	  memcpy (&context->msr, &sprs32, sizeof(sprs32));
	}
    }  
  return 0;
}

/* Write register function should be able to write requested context
   information to specified debuggee's kernel thread id.
   If successful return 0, else non-zero is returned.  */

static int
pdc_write_regs (pthdb_user_t user,
		pthdb_tid_t tid,
		unsigned long long flags,
		pthdb_context_t *context)
{ 
  /* This function doesn't appear to be used, so we could probably
     just return 0 here.  HOWEVER, if it is not defined, the OS will
     complain and several thread debug functions will fail.  In case
     this is needed, I have implemented what I think it should do,
     however this code is untested.  */

  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog, "pdc_write_regs tid=%d flags=%s\n",
                        (int) tid, hex_string (flags));

  /* General-purpose registers.  */
  if (flags & PTHDB_FLAG_GPRS)
    {
      if (arch64)
	ptrace64aix (PTT_WRITE_GPRS, tid, 
		     (unsigned long) context->gpr, 0, NULL);
      else
	ptrace32 (PTT_WRITE_GPRS, tid, (uintptr_t) context->gpr, 0, NULL);
    }

 /* Floating-point registers.  */
  if (flags & PTHDB_FLAG_FPRS)
    {
      ptrace32 (PTT_WRITE_FPRS, tid, (uintptr_t) context->fpr, 0, NULL);
    }

  /* Special-purpose registers.  */
  if (flags & PTHDB_FLAG_SPRS)
    {
      if (arch64)
	{
	  ptrace64aix (PTT_WRITE_SPRS, tid, 
		       (unsigned long) &context->msr, 0, NULL);
	}
      else
	{
	  ptrace32 (PTT_WRITE_SPRS, tid, (uintptr_t) &context->msr, 0, NULL);
	}
    }
  return 0;
}

/* pthdb callback: read LEN bytes from process ADDR into BUF.  */

static int
pdc_read_data (pthdb_user_t user, void *buf, 
	       pthdb_addr_t addr, size_t len)
{
  int status, ret;

  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog,
      "pdc_read_data (user = %ld, buf = 0x%lx, addr = %s, len = %ld)\n",
      user, (long) buf, hex_string (addr), len);

  status = target_read_memory (addr, (gdb_byte *) buf, len);
  ret = status == 0 ? PDC_SUCCESS : PDC_FAILURE;

  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog, "  status=%d, returning %s\n",
			status, pd_status2str (ret));
  return ret;
}

/* pthdb callback: write LEN bytes from BUF to process ADDR.  */

static int
pdc_write_data (pthdb_user_t user, void *buf, 
		pthdb_addr_t addr, size_t len)
{
  int status, ret;

  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog,
      "pdc_write_data (user = %ld, buf = 0x%lx, addr = %s, len = %ld)\n",
      user, (long) buf, hex_string (addr), len);

  status = target_write_memory (addr, (gdb_byte *) buf, len);
  ret = status == 0 ? PDC_SUCCESS : PDC_FAILURE;

  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog, "  status=%d, returning %s\n", status,
			pd_status2str (ret));
  return ret;
}

/* pthdb callback: allocate a LEN-byte buffer and store a pointer to it
   in BUFP.  */

static int
pdc_alloc (pthdb_user_t user, size_t len, void **bufp)
{
  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog,
                        "pdc_alloc (user = %ld, len = %ld, bufp = 0x%lx)\n",
			user, len, (long) bufp);
  *bufp = xmalloc (len);
  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog, 
			"  malloc returned 0x%lx\n", (long) *bufp);

  /* Note: xmalloc() can't return 0; therefore PDC_FAILURE will never
     be returned.  */

  return *bufp ? PDC_SUCCESS : PDC_FAILURE;
}

/* pthdb callback: reallocate BUF, which was allocated by the alloc or
   realloc callback, so that it contains LEN bytes, and store a
   pointer to the result in BUFP.  */

static int
pdc_realloc (pthdb_user_t user, void *buf, size_t len, void **bufp)
{
  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog,
      "pdc_realloc (user = %ld, buf = 0x%lx, len = %ld, bufp = 0x%lx)\n",
      user, (long) buf, len, (long) bufp);
  *bufp = xrealloc (buf, len);
  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog, 
			"  realloc returned 0x%lx\n", (long) *bufp);
  return *bufp ? PDC_SUCCESS : PDC_FAILURE;
}

/* pthdb callback: free BUF, which was allocated by the alloc or
   realloc callback.  */

static int
pdc_dealloc (pthdb_user_t user, void *buf)
{
  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog, 
			"pdc_free (user = %ld, buf = 0x%lx)\n", user,
                        (long) buf);
  xfree (buf);
  return PDC_SUCCESS;
}

/* Return a printable representation of pthread STATE.  */

static char *
state2str (pthdb_state_t state)
{
  switch (state)
    {
    case PST_IDLE:
      /* i18n: Like "Thread-Id %d, [state] idle" */
      return _("idle");      /* being created */
    case PST_RUN:
      /* i18n: Like "Thread-Id %d, [state] running" */
      return _("running");   /* running */
    case PST_SLEEP:
      /* i18n: Like "Thread-Id %d, [state] sleeping" */
      return _("sleeping");  /* awaiting an event */
    case PST_READY:
      /* i18n: Like "Thread-Id %d, [state] ready" */
      return _("ready");     /* runnable */
    case PST_TERM:
      /* i18n: Like "Thread-Id %d, [state] finished" */
      return _("finished");  /* awaiting a join/detach */
    default:
      /* i18n: Like "Thread-Id %d, [state] unknown" */
      return _("unknown");
    }
}

/* qsort() comparison function for sorting pd_thread structs by pthid.  */

static int
pcmp (const void *p1v, const void *p2v)
{
  struct pd_thread *p1 = (struct pd_thread *) p1v;
  struct pd_thread *p2 = (struct pd_thread *) p2v;
  return p1->pthid < p2->pthid ? -1 : p1->pthid > p2->pthid;
}

/* iterate_over_threads() callback for counting GDB threads.

   Do not count the main thread (whose tid is zero).  This matches
   the list of threads provided by the pthreaddebug library, which
   does not include that main thread either, and thus allows us
   to compare the two lists.  */

static int
giter_count (struct thread_info *thread, void *countp)
{
  if (PD_TID (thread->ptid))
    (*(int *) countp)++;
  return 0;
}

/* iterate_over_threads() callback for accumulating GDB thread pids.

   Do not include the main thread (whose tid is zero).  This matches
   the list of threads provided by the pthreaddebug library, which
   does not include that main thread either, and thus allows us
   to compare the two lists.  */

static int
giter_accum (struct thread_info *thread, void *bufp)
{
  if (PD_TID (thread->ptid))
    {
      **(struct thread_info ***) bufp = thread;
      (*(struct thread_info ***) bufp)++;
    }
  return 0;
}

/* ptid comparison function */

static int
ptid_cmp (ptid_t ptid1, ptid_t ptid2)
{
  if (ptid1.pid () < ptid2.pid ())
    return -1;
  else if (ptid1.pid () > ptid2.pid ())
    return 1;
  else if (ptid1.tid () < ptid2.tid ())
    return -1;
  else if (ptid1.tid () > ptid2.tid ())
    return 1;
  else if (ptid1.lwp () < ptid2.lwp ())
    return -1;
  else if (ptid1.lwp () > ptid2.lwp ())
    return 1;
  else
    return 0;
}

/* qsort() comparison function for sorting thread_info structs by pid.  */

static int
gcmp (const void *t1v, const void *t2v)
{
  struct thread_info *t1 = *(struct thread_info **) t1v;
  struct thread_info *t2 = *(struct thread_info **) t2v;
  return ptid_cmp (t1->ptid, t2->ptid);
}

/* Search through the list of all kernel threads for the thread
   that has stopped on a SIGTRAP signal, and return its TID.
   Return 0 if none found.  */

static pthdb_tid_t
get_signaled_thread (void)
{
  struct thrdsinfo64 thrinf;
  tid_t ktid = 0;

  while (1)
  {
    if (getthrds (inferior_ptid.pid (), &thrinf, 
          	  sizeof (thrinf), &ktid, 1) != 1)
      break;

    if (thrinf.ti_cursig == SIGTRAP)
      return thrinf.ti_tid;
  }

  /* Didn't find any thread stopped on a SIGTRAP signal.  */
  return 0;
}

/* Synchronize GDB's thread list with libpthdebug's.

   There are some benefits of doing this every time the inferior stops:

     - allows users to run thread-specific commands without needing to
       run "info threads" first

     - helps pthdb_tid_pthread() work properly (see "libpthdebug
       peculiarities" at the top of this module)

     - simplifies the demands placed on libpthdebug, which seems to
       have difficulty with certain call patterns */

static void
sync_threadlists (void)
{
  int cmd, status, infpid;
  int pcount, psize, pi, gcount, gi;
  struct pd_thread *pbuf;
  struct thread_info **gbuf, **g, *thread;
  pthdb_pthread_t pdtid;
  pthread_t pthid;
  pthdb_tid_t tid;

  /* Accumulate an array of libpthdebug threads sorted by pthread id.  */

  pcount = 0;
  psize = 1;
  pbuf = XNEWVEC (struct pd_thread, psize);

  for (cmd = PTHDB_LIST_FIRST;; cmd = PTHDB_LIST_NEXT)
    {
      status = pthdb_pthread (pd_session, &pdtid, cmd);
      if (status != PTHDB_SUCCESS || pdtid == PTHDB_INVALID_PTHREAD)
	break;

      status = pthdb_pthread_ptid (pd_session, pdtid, &pthid);
      if (status != PTHDB_SUCCESS || pthid == PTHDB_INVALID_PTID)
	continue;

      if (pcount == psize)
	{
	  psize *= 2;
	  pbuf = (struct pd_thread *) xrealloc (pbuf, 
						psize * sizeof *pbuf);
	}
      pbuf[pcount].pdtid = pdtid;
      pbuf[pcount].pthid = pthid;
      pcount++;
    }

  for (pi = 0; pi < pcount; pi++)
    {
      status = pthdb_pthread_tid (pd_session, pbuf[pi].pdtid, &tid);
      if (status != PTHDB_SUCCESS)
	tid = PTHDB_INVALID_TID;
      pbuf[pi].tid = tid;
    }

  qsort (pbuf, pcount, sizeof *pbuf, pcmp);

  /* Accumulate an array of GDB threads sorted by pid.  */

  gcount = 0;
  iterate_over_threads (giter_count, &gcount);
  g = gbuf = XNEWVEC (struct thread_info *, gcount);
  iterate_over_threads (giter_accum, &g);
  qsort (gbuf, gcount, sizeof *gbuf, gcmp);

  /* Apply differences between the two arrays to GDB's thread list.  */

  infpid = inferior_ptid.pid ();
  for (pi = gi = 0; pi < pcount || gi < gcount;)
    {
      if (pi == pcount)
	{
	  delete_thread (gbuf[gi]);
	  gi++;
	}
      else if (gi == gcount)
	{
	  aix_thread_info *priv = new aix_thread_info;
	  priv->pdtid = pbuf[pi].pdtid;
	  priv->tid = pbuf[pi].tid;

	  process_stratum_target *proc_target
	    = current_inferior ()->process_target ();
	  thread = add_thread_with_info (proc_target,
					 ptid_t (infpid, 0, pbuf[pi].pthid),
					 priv);

	  pi++;
	}
      else
	{
	  ptid_t pptid, gptid;
	  int cmp_result;

	  pptid = ptid_t (infpid, 0, pbuf[pi].pthid);
	  gptid = gbuf[gi]->ptid;
	  pdtid = pbuf[pi].pdtid;
	  tid = pbuf[pi].tid;

	  cmp_result = ptid_cmp (pptid, gptid);

	  if (cmp_result == 0)
	    {
	      aix_thread_info *priv = get_aix_thread_info (gbuf[gi]);

	      priv->pdtid = pdtid;
	      priv->tid = tid;
	      pi++;
	      gi++;
	    }
	  else if (cmp_result > 0)
	    {
	      delete_thread (gbuf[gi]);
	      gi++;
	    }
	  else
	    {
	      process_stratum_target *proc_target
		= current_inferior ()->process_target ();
	      thread = add_thread (proc_target, pptid);

	      aix_thread_info *priv = new aix_thread_info;
	      thread->priv.reset (priv);
	      priv->pdtid = pdtid;
	      priv->tid = tid;
	      pi++;
	    }
	}
    }

  xfree (pbuf);
  xfree (gbuf);
}

/* Iterate_over_threads() callback for locating a thread, using
   the TID of its associated kernel thread.  */

static int
iter_tid (struct thread_info *thread, void *tidp)
{
  const pthdb_tid_t tid = *(pthdb_tid_t *)tidp;
  aix_thread_info *priv = get_aix_thread_info (thread);

  return priv->tid == tid;
}

/* Synchronize libpthdebug's state with the inferior and with GDB,
   generate a composite process/thread <pid> for the current thread,
   set inferior_ptid to <pid> if SET_INFPID, and return <pid>.  */

static ptid_t
pd_update (int set_infpid)
{
  int status;
  ptid_t ptid;
  pthdb_tid_t tid;
  struct thread_info *thread = NULL;

  if (!pd_active)
    return inferior_ptid;

  status = pthdb_session_update (pd_session);
  if (status != PTHDB_SUCCESS)
    return inferior_ptid;

  sync_threadlists ();

  /* Define "current thread" as one that just received a trap signal.  */

  tid = get_signaled_thread ();
  if (tid != 0)
    thread = iterate_over_threads (iter_tid, &tid);
  if (!thread)
    ptid = inferior_ptid;
  else
    {
      ptid = thread->ptid;
      if (set_infpid)
	switch_to_thread (thread);
    }
  return ptid;
}

/* Try to start debugging threads in the current process.
   If successful and SET_INFPID, set inferior_ptid to reflect the
   current thread.  */

static ptid_t
pd_activate (int set_infpid)
{
  int status;
		
  status = pthdb_session_init (PD_USER, arch64 ? PEM_64BIT : PEM_32BIT,
			       PTHDB_FLAG_REGS, &pd_callbacks, 
			       &pd_session);
  if (status != PTHDB_SUCCESS)
    {
      return inferior_ptid;
    }
  pd_active = 1;
  return pd_update (set_infpid);
}

/* Undo the effects of pd_activate().  */

static void
pd_deactivate (void)
{
  if (!pd_active)
    return;
  pthdb_session_destroy (pd_session);
  
  pid_to_prc (&inferior_ptid);
  pd_active = 0;
}

/* An object file has just been loaded.  Check whether the current
   application is pthreaded, and if so, prepare for thread debugging.  */

static void
pd_enable (void)
{
  int status;
  char *stub_name;
  struct bound_minimal_symbol ms;

  /* Don't initialize twice.  */
  if (pd_able)
    return;

  /* Check application word size.  */
  arch64 = register_size (target_gdbarch (), 0) == 8;

  /* Check whether the application is pthreaded.  */
  stub_name = NULL;
  status = pthdb_session_pthreaded (PD_USER, PTHDB_FLAG_REGS,
				    &pd_callbacks, &stub_name);
  if ((status != PTHDB_SUCCESS
       && status != PTHDB_NOT_PTHREADED) || !stub_name)
    return;

  /* Set a breakpoint on the returned stub function.  */
  ms = lookup_minimal_symbol (stub_name, NULL, NULL);
  if (ms.minsym == NULL)
    return;
  pd_brk_addr = BMSYMBOL_VALUE_ADDRESS (ms);
  if (!create_thread_event_breakpoint (target_gdbarch (), pd_brk_addr))
    return;

  /* Prepare for thread debugging.  */
  push_target (&aix_thread_ops);
  pd_able = 1;

  /* If we're debugging a core file or an attached inferior, the
     pthread library may already have been initialized, so try to
     activate thread debugging.  */
  pd_activate (1);
}

/* Undo the effects of pd_enable().  */

static void
pd_disable (void)
{
  if (!pd_able)
    return;
  if (pd_active)
    pd_deactivate ();
  pd_able = 0;
  unpush_target (&aix_thread_ops);
}

/* new_objfile observer callback.

   If OBJFILE is non-null, check whether a threaded application is
   being debugged, and if so, prepare for thread debugging.

   If OBJFILE is null, stop debugging threads.  */

static void
new_objfile (struct objfile *objfile)
{
  if (objfile)
    pd_enable ();
  else
    pd_disable ();
}

/* Attach to process specified by ARGS.  */

static void
aix_thread_inferior_created (struct target_ops *ops, int from_tty)
{
  pd_enable ();
}

/* Detach from the process attached to by aix_thread_attach().  */

void
aix_thread_target::detach (inferior *inf, int from_tty)
{
  target_ops *beneath = this->beneath ();

  pd_disable ();
  beneath->detach (inf, from_tty);
}

/* Tell the inferior process to continue running thread PID if != -1
   and all threads otherwise.  */

void
aix_thread_target::resume (ptid_t ptid, int step, enum gdb_signal sig)
{
  struct thread_info *thread;
  pthdb_tid_t tid[2];

  if (!PD_TID (ptid))
    {
      scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);
      
      inferior_ptid = ptid_t (inferior_ptid.pid ());
      beneath ()->resume (ptid, step, sig);
    }
  else
    {
      thread = find_thread_ptid (current_inferior (), ptid);
      if (!thread)
	error (_("aix-thread resume: unknown pthread %ld"),
	       ptid.lwp ());

      aix_thread_info *priv = get_aix_thread_info (thread);

      tid[0] = priv->tid;
      if (tid[0] == PTHDB_INVALID_TID)
	error (_("aix-thread resume: no tid for pthread %ld"),
	       ptid.lwp ());
      tid[1] = 0;

      if (arch64)
	ptrace64aix (PTT_CONTINUE, tid[0], (long long) 1,
		     gdb_signal_to_host (sig), (PTRACE_TYPE_ARG5) tid);
      else
	ptrace32 (PTT_CONTINUE, tid[0], (addr_ptr) 1,
		  gdb_signal_to_host (sig), (PTRACE_TYPE_ARG5) tid);
    }
}

/* Wait for thread/process ID if != -1 or for any thread otherwise.
   If an error occurs, return -1, else return the pid of the stopped
   thread.  */

ptid_t
aix_thread_target::wait (ptid_t ptid, struct target_waitstatus *status,
			 int options)
{
  {
    scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);

    pid_to_prc (&ptid);

    inferior_ptid = ptid_t (inferior_ptid.pid ());
    ptid = beneath ()->wait (ptid, status, options);
  }

  if (ptid.pid () == -1)
    return ptid_t (-1);

  /* Check whether libpthdebug might be ready to be initialized.  */
  if (!pd_active && status->kind == TARGET_WAITKIND_STOPPED
      && status->value.sig == GDB_SIGNAL_TRAP)
    {
      process_stratum_target *proc_target
	= current_inferior ()->process_target ();
      struct regcache *regcache = get_thread_regcache (proc_target, ptid);
      struct gdbarch *gdbarch = regcache->arch ();

      if (regcache_read_pc (regcache)
	  - gdbarch_decr_pc_after_break (gdbarch) == pd_brk_addr)
	return pd_activate (0);
    }

  return pd_update (0);
}

/* Record that the 64-bit general-purpose registers contain VALS.  */

static void
supply_gprs64 (struct regcache *regcache, uint64_t *vals)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (regcache->arch ());
  int regno;

  for (regno = 0; regno < ppc_num_gprs; regno++)
    regcache->raw_supply (tdep->ppc_gp0_regnum + regno,
			  (char *) (vals + regno));
}

/* Record that 32-bit register REGNO contains VAL.  */

static void
supply_reg32 (struct regcache *regcache, int regno, uint32_t val)
{
  regcache->raw_supply (regno, (char *) &val);
}

/* Record that the floating-point registers contain VALS.  */

static void
supply_fprs (struct regcache *regcache, double *vals)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int regno;

  /* This function should never be called on architectures without
     floating-point registers.  */
  gdb_assert (ppc_floating_point_unit_p (gdbarch));

  for (regno = tdep->ppc_fp0_regnum;
       regno < tdep->ppc_fp0_regnum + ppc_num_fprs;
       regno++)
    regcache->raw_supply (regno,
			  (char *) (vals + regno - tdep->ppc_fp0_regnum));
}

/* Predicate to test whether given register number is a "special" register.  */
static int
special_register_p (struct gdbarch *gdbarch, int regno)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  return regno == gdbarch_pc_regnum (gdbarch)
      || regno == tdep->ppc_ps_regnum
      || regno == tdep->ppc_cr_regnum
      || regno == tdep->ppc_lr_regnum
      || regno == tdep->ppc_ctr_regnum
      || regno == tdep->ppc_xer_regnum
      || (tdep->ppc_fpscr_regnum >= 0 && regno == tdep->ppc_fpscr_regnum)
      || (tdep->ppc_mq_regnum >= 0 && regno == tdep->ppc_mq_regnum);
}


/* Record that the special registers contain the specified 64-bit and
   32-bit values.  */

static void
supply_sprs64 (struct regcache *regcache,
	       uint64_t iar, uint64_t msr, uint32_t cr,
	       uint64_t lr, uint64_t ctr, uint32_t xer,
	       uint32_t fpscr)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  regcache->raw_supply (gdbarch_pc_regnum (gdbarch), (char *) &iar);
  regcache->raw_supply (tdep->ppc_ps_regnum, (char *) &msr);
  regcache->raw_supply (tdep->ppc_cr_regnum, (char *) &cr);
  regcache->raw_supply (tdep->ppc_lr_regnum, (char *) &lr);
  regcache->raw_supply (tdep->ppc_ctr_regnum, (char *) &ctr);
  regcache->raw_supply (tdep->ppc_xer_regnum, (char *) &xer);
  if (tdep->ppc_fpscr_regnum >= 0)
    regcache->raw_supply (tdep->ppc_fpscr_regnum, (char *) &fpscr);
}

/* Record that the special registers contain the specified 32-bit
   values.  */

static void
supply_sprs32 (struct regcache *regcache,
	       uint32_t iar, uint32_t msr, uint32_t cr,
	       uint32_t lr, uint32_t ctr, uint32_t xer,
	       uint32_t fpscr)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  regcache->raw_supply (gdbarch_pc_regnum (gdbarch), (char *) &iar);
  regcache->raw_supply (tdep->ppc_ps_regnum, (char *) &msr);
  regcache->raw_supply (tdep->ppc_cr_regnum, (char *) &cr);
  regcache->raw_supply (tdep->ppc_lr_regnum, (char *) &lr);
  regcache->raw_supply (tdep->ppc_ctr_regnum, (char *) &ctr);
  regcache->raw_supply (tdep->ppc_xer_regnum, (char *) &xer);
  if (tdep->ppc_fpscr_regnum >= 0)
    regcache->raw_supply (tdep->ppc_fpscr_regnum, (char *) &fpscr);
}

/* Fetch all registers from pthread PDTID, which doesn't have a kernel
   thread.

   There's no way to query a single register from a non-kernel
   pthread, so there's no need for a single-register version of this
   function.  */

static void
fetch_regs_user_thread (struct regcache *regcache, pthdb_pthread_t pdtid)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int status, i;
  pthdb_context_t ctx;

  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog, 
			"fetch_regs_user_thread %lx\n", (long) pdtid);
  status = pthdb_pthread_context (pd_session, pdtid, &ctx);
  if (status != PTHDB_SUCCESS)
    error (_("aix-thread: fetch_registers: pthdb_pthread_context returned %s"),
           pd_status2str (status));

  /* General-purpose registers.  */

  if (arch64)
    supply_gprs64 (regcache, ctx.gpr);
  else
    for (i = 0; i < ppc_num_gprs; i++)
      supply_reg32 (regcache, tdep->ppc_gp0_regnum + i, ctx.gpr[i]);

  /* Floating-point registers.  */

  if (ppc_floating_point_unit_p (gdbarch))
    supply_fprs (regcache, ctx.fpr);

  /* Special registers.  */

  if (arch64)
    supply_sprs64 (regcache, ctx.iar, ctx.msr, ctx.cr, ctx.lr, ctx.ctr,
			     ctx.xer, ctx.fpscr);
  else
    supply_sprs32 (regcache, ctx.iar, ctx.msr, ctx.cr, ctx.lr, ctx.ctr,
			     ctx.xer, ctx.fpscr);
}

/* Fetch register REGNO if != -1 or all registers otherwise from
   kernel thread TID.

   AIX provides a way to query all of a kernel thread's GPRs, FPRs, or
   SPRs, but there's no way to query individual registers within those
   groups.  Therefore, if REGNO != -1, this function fetches an entire
   group.

   Unfortunately, kernel thread register queries often fail with
   EPERM, indicating that the thread is in kernel space.  This breaks
   backtraces of threads other than the current one.  To make that
   breakage obvious without throwing an error to top level (which is
   bad e.g. during "info threads" output), zero registers that can't
   be retrieved.  */

static void
fetch_regs_kernel_thread (struct regcache *regcache, int regno,
			  pthdb_tid_t tid)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  uint64_t gprs64[ppc_num_gprs];
  uint32_t gprs32[ppc_num_gprs];
  double fprs[ppc_num_fprs];
  struct ptxsprs sprs64;
  struct ptsprs sprs32;
  int i;

  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog,
	"fetch_regs_kernel_thread tid=%lx regno=%d arch64=%d\n",
	(long) tid, regno, arch64);

  /* General-purpose registers.  */
  if (regno == -1
      || (tdep->ppc_gp0_regnum <= regno
          && regno < tdep->ppc_gp0_regnum + ppc_num_gprs))
    {
      if (arch64)
	{
	  if (!ptrace64aix (PTT_READ_GPRS, tid, 
			    (unsigned long) gprs64, 0, NULL))
	    memset (gprs64, 0, sizeof (gprs64));
	  supply_gprs64 (regcache, gprs64);
	}
      else
	{
	  if (!ptrace32 (PTT_READ_GPRS, tid, (uintptr_t) gprs32, 0, NULL))
	    memset (gprs32, 0, sizeof (gprs32));
	  for (i = 0; i < ppc_num_gprs; i++)
	    supply_reg32 (regcache, tdep->ppc_gp0_regnum + i, gprs32[i]);
	}
    }

  /* Floating-point registers.  */

  if (ppc_floating_point_unit_p (gdbarch)
      && (regno == -1
          || (regno >= tdep->ppc_fp0_regnum
              && regno < tdep->ppc_fp0_regnum + ppc_num_fprs)))
    {
      if (!ptrace32 (PTT_READ_FPRS, tid, (uintptr_t) fprs, 0, NULL))
	memset (fprs, 0, sizeof (fprs));
      supply_fprs (regcache, fprs);
    }

  /* Special-purpose registers.  */

  if (regno == -1 || special_register_p (gdbarch, regno))
    {
      if (arch64)
	{
	  if (!ptrace64aix (PTT_READ_SPRS, tid, 
			    (unsigned long) &sprs64, 0, NULL))
	    memset (&sprs64, 0, sizeof (sprs64));
	  supply_sprs64 (regcache, sprs64.pt_iar, sprs64.pt_msr,
			 sprs64.pt_cr, sprs64.pt_lr, sprs64.pt_ctr,
			 sprs64.pt_xer, sprs64.pt_fpscr);
	}
      else
	{
	  if (!ptrace32 (PTT_READ_SPRS, tid, (uintptr_t) &sprs32, 0, NULL))
	    memset (&sprs32, 0, sizeof (sprs32));
	  supply_sprs32 (regcache, sprs32.pt_iar, sprs32.pt_msr, sprs32.pt_cr,
			 sprs32.pt_lr, sprs32.pt_ctr, sprs32.pt_xer,
			 sprs32.pt_fpscr);

	  if (tdep->ppc_mq_regnum >= 0)
	    regcache->raw_supply (tdep->ppc_mq_regnum, (char *) &sprs32.pt_mq);
	}
    }
}

/* Fetch register REGNO if != -1 or all registers otherwise from the
   thread/process connected to REGCACHE.  */

void
aix_thread_target::fetch_registers (struct regcache *regcache, int regno)
{
  struct thread_info *thread;
  pthdb_tid_t tid;

  if (!PD_TID (regcache->ptid ()))
    beneath ()->fetch_registers (regcache, regno);
  else
    {
      thread = find_thread_ptid (current_inferior (), regcache->ptid ());
      aix_thread_info *priv = get_aix_thread_info (thread);
      tid = priv->tid;

      if (tid == PTHDB_INVALID_TID)
	fetch_regs_user_thread (regcache, priv->pdtid);
      else
	fetch_regs_kernel_thread (regcache, regno, tid);
    }
}

/* Store the gp registers into an array of uint32_t or uint64_t.  */

static void
fill_gprs64 (const struct regcache *regcache, uint64_t *vals)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (regcache->arch ());
  int regno;

  for (regno = 0; regno < ppc_num_gprs; regno++)
    if (REG_VALID == regcache->get_register_status
		       (tdep->ppc_gp0_regnum + regno))
      regcache->raw_collect (tdep->ppc_gp0_regnum + regno, vals + regno);
}

static void 
fill_gprs32 (const struct regcache *regcache, uint32_t *vals)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (regcache->arch ());
  int regno;

  for (regno = 0; regno < ppc_num_gprs; regno++)
    if (REG_VALID == regcache->get_register_status
		       (tdep->ppc_gp0_regnum + regno))
      regcache->raw_collect (tdep->ppc_gp0_regnum + regno, vals + regno);
}

/* Store the floating point registers into a double array.  */
static void
fill_fprs (const struct regcache *regcache, double *vals)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int regno;

  /* This function should never be called on architectures without
     floating-point registers.  */
  gdb_assert (ppc_floating_point_unit_p (gdbarch));

  for (regno = tdep->ppc_fp0_regnum;
       regno < tdep->ppc_fp0_regnum + ppc_num_fprs;
       regno++)
    if (REG_VALID == regcache->get_register_status (regno))
      regcache->raw_collect (regno, vals + regno - tdep->ppc_fp0_regnum);
}

/* Store the special registers into the specified 64-bit and 32-bit
   locations.  */

static void
fill_sprs64 (const struct regcache *regcache,
	     uint64_t *iar, uint64_t *msr, uint32_t *cr,
	     uint64_t *lr, uint64_t *ctr, uint32_t *xer,
	     uint32_t *fpscr)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Verify that the size of the size of the IAR buffer is the
     same as the raw size of the PC (in the register cache).  If
     they're not, then either GDB has been built incorrectly, or
     there's some other kind of internal error.  To be really safe,
     we should check all of the sizes.   */
  gdb_assert (sizeof (*iar) == register_size
				 (gdbarch, gdbarch_pc_regnum (gdbarch)));

  if (REG_VALID == regcache->get_register_status (gdbarch_pc_regnum (gdbarch)))
    regcache->raw_collect (gdbarch_pc_regnum (gdbarch), iar);
  if (REG_VALID == regcache->get_register_status (tdep->ppc_ps_regnum))
    regcache->raw_collect (tdep->ppc_ps_regnum, msr);
  if (REG_VALID == regcache->get_register_status (tdep->ppc_cr_regnum))
    regcache->raw_collect (tdep->ppc_cr_regnum, cr);
  if (REG_VALID == regcache->get_register_status (tdep->ppc_lr_regnum))
    regcache->raw_collect (tdep->ppc_lr_regnum, lr);
  if (REG_VALID == regcache->get_register_status (tdep->ppc_ctr_regnum))
    regcache->raw_collect (tdep->ppc_ctr_regnum, ctr);
  if (REG_VALID == regcache->get_register_status (tdep->ppc_xer_regnum))
    regcache->raw_collect (tdep->ppc_xer_regnum, xer);
  if (tdep->ppc_fpscr_regnum >= 0
      && REG_VALID == regcache->get_register_status (tdep->ppc_fpscr_regnum))
    regcache->raw_collect (tdep->ppc_fpscr_regnum, fpscr);
}

static void
fill_sprs32 (const struct regcache *regcache,
	     uint32_t *iar, uint32_t *msr, uint32_t *cr,
	     uint32_t *lr, uint32_t *ctr, uint32_t *xer,
	     uint32_t *fpscr)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Verify that the size of the size of the IAR buffer is the
     same as the raw size of the PC (in the register cache).  If
     they're not, then either GDB has been built incorrectly, or
     there's some other kind of internal error.  To be really safe,
     we should check all of the sizes.  */
  gdb_assert (sizeof (*iar) == register_size (gdbarch,
					      gdbarch_pc_regnum (gdbarch)));

  if (REG_VALID == regcache->get_register_status (gdbarch_pc_regnum (gdbarch)))
    regcache->raw_collect (gdbarch_pc_regnum (gdbarch), iar);
  if (REG_VALID == regcache->get_register_status (tdep->ppc_ps_regnum))
    regcache->raw_collect (tdep->ppc_ps_regnum, msr);
  if (REG_VALID == regcache->get_register_status (tdep->ppc_cr_regnum))
    regcache->raw_collect (tdep->ppc_cr_regnum, cr);
  if (REG_VALID == regcache->get_register_status (tdep->ppc_lr_regnum))
    regcache->raw_collect (tdep->ppc_lr_regnum, lr);
  if (REG_VALID == regcache->get_register_status (tdep->ppc_ctr_regnum))
    regcache->raw_collect (tdep->ppc_ctr_regnum, ctr);
  if (REG_VALID == regcache->get_register_status (tdep->ppc_xer_regnum))
    regcache->raw_collect (tdep->ppc_xer_regnum, xer);
  if (tdep->ppc_fpscr_regnum >= 0
      && REG_VALID == regcache->get_register_status (tdep->ppc_fpscr_regnum))
    regcache->raw_collect (tdep->ppc_fpscr_regnum, fpscr);
}

/* Store all registers into pthread PDTID, which doesn't have a kernel
   thread.

   It's possible to store a single register into a non-kernel pthread,
   but I doubt it's worth the effort.  */

static void
store_regs_user_thread (const struct regcache *regcache, pthdb_pthread_t pdtid)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  int status, i;
  pthdb_context_t ctx;
  uint32_t int32;
  uint64_t int64;

  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog, 
			"store_regs_user_thread %lx\n", (long) pdtid);

  /* Retrieve the thread's current context for its non-register
     values.  */
  status = pthdb_pthread_context (pd_session, pdtid, &ctx);
  if (status != PTHDB_SUCCESS)
    error (_("aix-thread: store_registers: pthdb_pthread_context returned %s"),
           pd_status2str (status));

  /* Collect general-purpose register values from the regcache.  */

  for (i = 0; i < ppc_num_gprs; i++)
    if (REG_VALID == regcache->get_register_status (tdep->ppc_gp0_regnum + i))
      {
	if (arch64)
	  {
	    regcache->raw_collect (tdep->ppc_gp0_regnum + i, (void *) &int64);
	    ctx.gpr[i] = int64;
	  }
	else
	  {
	    regcache->raw_collect (tdep->ppc_gp0_regnum + i, (void *) &int32);
	    ctx.gpr[i] = int32;
	  }
      }

  /* Collect floating-point register values from the regcache.  */
  if (ppc_floating_point_unit_p (gdbarch))
    fill_fprs (regcache, ctx.fpr);

  /* Special registers (always kept in ctx as 64 bits).  */
  if (arch64)
    {
      fill_sprs64 (regcache, &ctx.iar, &ctx.msr, &ctx.cr, &ctx.lr, &ctx.ctr,
			     &ctx.xer, &ctx.fpscr);
    }
  else
    {
      /* Problem: ctx.iar etc. are 64 bits, but raw_registers are 32.
	 Solution: use 32-bit temp variables.  */
      uint32_t tmp_iar, tmp_msr, tmp_cr, tmp_lr, tmp_ctr, tmp_xer,
	       tmp_fpscr;

      fill_sprs32 (regcache, &tmp_iar, &tmp_msr, &tmp_cr, &tmp_lr, &tmp_ctr,
			     &tmp_xer, &tmp_fpscr);
      if (REG_VALID == regcache->get_register_status
			 (gdbarch_pc_regnum (gdbarch)))
	ctx.iar = tmp_iar;
      if (REG_VALID == regcache->get_register_status (tdep->ppc_ps_regnum))
	ctx.msr = tmp_msr;
      if (REG_VALID == regcache->get_register_status (tdep->ppc_cr_regnum))
	ctx.cr  = tmp_cr;
      if (REG_VALID == regcache->get_register_status (tdep->ppc_lr_regnum))
	ctx.lr  = tmp_lr;
      if (REG_VALID == regcache->get_register_status (tdep->ppc_ctr_regnum))
	ctx.ctr = tmp_ctr;
      if (REG_VALID == regcache->get_register_status (tdep->ppc_xer_regnum))
	ctx.xer = tmp_xer;
      if (REG_VALID == regcache->get_register_status (tdep->ppc_xer_regnum))
	ctx.fpscr = tmp_fpscr;
    }

  status = pthdb_pthread_setcontext (pd_session, pdtid, &ctx);
  if (status != PTHDB_SUCCESS)
    error (_("aix-thread: store_registers: "
	     "pthdb_pthread_setcontext returned %s"),
           pd_status2str (status));
}

/* Store register REGNO if != -1 or all registers otherwise into
   kernel thread TID.

   AIX provides a way to set all of a kernel thread's GPRs, FPRs, or
   SPRs, but there's no way to set individual registers within those
   groups.  Therefore, if REGNO != -1, this function stores an entire
   group.  */

static void
store_regs_kernel_thread (const struct regcache *regcache, int regno,
			  pthdb_tid_t tid)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);
  uint64_t gprs64[ppc_num_gprs];
  uint32_t gprs32[ppc_num_gprs];
  double fprs[ppc_num_fprs];
  struct ptxsprs sprs64;
  struct ptsprs  sprs32;

  if (debug_aix_thread)
    fprintf_unfiltered (gdb_stdlog, 
			"store_regs_kernel_thread tid=%lx regno=%d\n",
                        (long) tid, regno);

  /* General-purpose registers.  */
  if (regno == -1
      || (tdep->ppc_gp0_regnum <= regno
          && regno < tdep->ppc_gp0_regnum + ppc_num_fprs))
    {
      if (arch64)
	{
	  /* Pre-fetch: some regs may not be in the cache.  */
	  ptrace64aix (PTT_READ_GPRS, tid, (unsigned long) gprs64, 0, NULL);
	  fill_gprs64 (regcache, gprs64);
	  ptrace64aix (PTT_WRITE_GPRS, tid, (unsigned long) gprs64, 0, NULL);
	}
      else
	{
	  /* Pre-fetch: some regs may not be in the cache.  */
	  ptrace32 (PTT_READ_GPRS, tid, (uintptr_t) gprs32, 0, NULL);
	  fill_gprs32 (regcache, gprs32);
	  ptrace32 (PTT_WRITE_GPRS, tid, (uintptr_t) gprs32, 0, NULL);
	}
    }

  /* Floating-point registers.  */

  if (ppc_floating_point_unit_p (gdbarch)
      && (regno == -1
          || (regno >= tdep->ppc_fp0_regnum
              && regno < tdep->ppc_fp0_regnum + ppc_num_fprs)))
    {
      /* Pre-fetch: some regs may not be in the cache.  */
      ptrace32 (PTT_READ_FPRS, tid, (uintptr_t) fprs, 0, NULL);
      fill_fprs (regcache, fprs);
      ptrace32 (PTT_WRITE_FPRS, tid, (uintptr_t) fprs, 0, NULL);
    }

  /* Special-purpose registers.  */

  if (regno == -1 || special_register_p (gdbarch, regno))
    {
      if (arch64)
	{
	  /* Pre-fetch: some registers won't be in the cache.  */
	  ptrace64aix (PTT_READ_SPRS, tid, 
		       (unsigned long) &sprs64, 0, NULL);
	  fill_sprs64 (regcache, &sprs64.pt_iar, &sprs64.pt_msr,
		       &sprs64.pt_cr, &sprs64.pt_lr, &sprs64.pt_ctr,
		       &sprs64.pt_xer, &sprs64.pt_fpscr);
	  ptrace64aix (PTT_WRITE_SPRS, tid, 
		       (unsigned long) &sprs64, 0, NULL);
	}
      else
	{
	  /* The contents of "struct ptspr" were declared as "unsigned
	     long" up to AIX 5.2, but are "unsigned int" since 5.3.
	     Use temporaries to work around this problem.  Also, add an
	     assert here to make sure we fail if the system header files
	     use "unsigned long", and the size of that type is not what
	     the headers expect.  */
	  uint32_t tmp_iar, tmp_msr, tmp_cr, tmp_lr, tmp_ctr, tmp_xer,
		   tmp_fpscr;

	  gdb_assert (sizeof (sprs32.pt_iar) == 4);

	  /* Pre-fetch: some registers won't be in the cache.  */
	  ptrace32 (PTT_READ_SPRS, tid, (uintptr_t) &sprs32, 0, NULL);

	  fill_sprs32 (regcache, &tmp_iar, &tmp_msr, &tmp_cr, &tmp_lr,
		       &tmp_ctr, &tmp_xer, &tmp_fpscr);

	  sprs32.pt_iar = tmp_iar;
	  sprs32.pt_msr = tmp_msr;
	  sprs32.pt_cr = tmp_cr;
	  sprs32.pt_lr = tmp_lr;
	  sprs32.pt_ctr = tmp_ctr;
	  sprs32.pt_xer = tmp_xer;
	  sprs32.pt_fpscr = tmp_fpscr;

	  if (tdep->ppc_mq_regnum >= 0)
	    if (REG_VALID == regcache->get_register_status
			       (tdep->ppc_mq_regnum))
	      regcache->raw_collect (tdep->ppc_mq_regnum, &sprs32.pt_mq);

	  ptrace32 (PTT_WRITE_SPRS, tid, (uintptr_t) &sprs32, 0, NULL);
	}
    }
}

/* Store gdb's current view of the register set into the
   thread/process connected to REGCACHE.  */

void
aix_thread_target::store_registers (struct regcache *regcache, int regno)
{
  struct thread_info *thread;
  pthdb_tid_t tid;

  if (!PD_TID (regcache->ptid ()))
    beneath ()->store_registers (regcache, regno);
  else
    {
      thread = find_thread_ptid (current_inferior (), regcache->ptid ());
      aix_thread_info *priv = get_aix_thread_info (thread);
      tid = priv->tid;

      if (tid == PTHDB_INVALID_TID)
	store_regs_user_thread (regcache, priv->pdtid);
      else
	store_regs_kernel_thread (regcache, regno, tid);
    }
}

/* Implement the to_xfer_partial target_ops method.  */

enum target_xfer_status
aix_thread_target::xfer_partial (enum target_object object,
				 const char *annex, gdb_byte *readbuf,
				 const gdb_byte *writebuf,
				 ULONGEST offset, ULONGEST len,
				 ULONGEST *xfered_len)
{
  scoped_restore save_inferior_ptid = make_scoped_restore (&inferior_ptid);

  inferior_ptid = ptid_t (inferior_ptid.pid ());
  return beneath ()->xfer_partial (object, annex, readbuf,
				   writebuf, offset, len, xfered_len);
}

/* Clean up after the inferior exits.  */

void
aix_thread_target::mourn_inferior ()
{
  target_ops *beneath = this->beneath ();

  pd_deactivate ();
  beneath->mourn_inferior ();
}

/* Return whether thread PID is still valid.  */

bool
aix_thread_target::thread_alive (ptid_t ptid)
{
  if (!PD_TID (ptid))
    return beneath ()->thread_alive (ptid);

  /* We update the thread list every time the child stops, so all
     valid threads should be in the thread list.  */
  process_stratum_target *proc_target
    = current_inferior ()->process_target ();
  return in_thread_list (proc_target, ptid);
}

/* Return a printable representation of composite PID for use in
   "info threads" output.  */

std::string
aix_thread_target::pid_to_str (ptid_t ptid)
{
  if (!PD_TID (ptid))
    return beneath ()->pid_to_str (ptid);

  return string_printf (_("Thread %ld"), ptid.tid ());
}

/* Return a printable representation of extra information about
   THREAD, for use in "info threads" output.  */

const char *
aix_thread_target::extra_thread_info (struct thread_info *thread)
{
  int status;
  pthdb_pthread_t pdtid;
  pthdb_tid_t tid;
  pthdb_state_t state;
  pthdb_suspendstate_t suspendstate;
  pthdb_detachstate_t detachstate;
  int cancelpend;
  static char *ret = NULL;

  if (!PD_TID (thread->ptid))
    return NULL;

  string_file buf;
  aix_thread_info *priv = get_aix_thread_info (thread);

  pdtid = priv->pdtid;
  tid = priv->tid;

  if (tid != PTHDB_INVALID_TID)
    /* i18n: Like "thread-identifier %d, [state] running, suspended" */
    buf.printf (_("tid %d"), (int)tid);

  status = pthdb_pthread_state (pd_session, pdtid, &state);
  if (status != PTHDB_SUCCESS)
    state = PST_NOTSUP;
  buf.printf (", %s", state2str (state));

  status = pthdb_pthread_suspendstate (pd_session, pdtid, 
				       &suspendstate);
  if (status == PTHDB_SUCCESS && suspendstate == PSS_SUSPENDED)
    /* i18n: Like "Thread-Id %d, [state] running, suspended" */
    buf.printf (_(", suspended"));

  status = pthdb_pthread_detachstate (pd_session, pdtid, 
				      &detachstate);
  if (status == PTHDB_SUCCESS && detachstate == PDS_DETACHED)
    /* i18n: Like "Thread-Id %d, [state] running, detached" */
    buf.printf (_(", detached"));

  pthdb_pthread_cancelpend (pd_session, pdtid, &cancelpend);
  if (status == PTHDB_SUCCESS && cancelpend)
    /* i18n: Like "Thread-Id %d, [state] running, cancel pending" */
    buf.printf (_(", cancel pending"));

  buf.write ("", 1);

  xfree (ret);			/* Free old buffer.  */

  ret = xstrdup (buf.c_str ());

  return ret;
}

ptid_t
aix_thread_target::get_ada_task_ptid (long lwp, long thread)
{
  return ptid_t (inferior_ptid.pid (), 0, thread);
}


/* Module startup initialization function, automagically called by
   init.c.  */

void _initialize_aix_thread ();
void
_initialize_aix_thread ()
{
  /* Notice when object files get loaded and unloaded.  */
  gdb::observers::new_objfile.attach (new_objfile);

  /* Add ourselves to inferior_created event chain.
     This is needed to enable the thread target on "attach".  */
  gdb::observers::inferior_created.attach (aix_thread_inferior_created);

  add_setshow_boolean_cmd ("aix-thread", class_maintenance, &debug_aix_thread,
			   _("Set debugging of AIX thread module."),
			   _("Show debugging of AIX thread module."),
			   _("Enables debugging output (used to debug GDB)."),
			   NULL, NULL,
			   /* FIXME: i18n: Debugging of AIX thread
			      module is \"%d\".  */
			   &setdebuglist, &showdebuglist);
}
