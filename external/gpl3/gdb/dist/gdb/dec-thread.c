/* Copyright (C) 2008-2013 Free Software Foundation, Inc.

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
#include "command.h"
#include "gdbcmd.h"
#include "target.h"
#include "observer.h"
#include <sys/procfs.h>
#include "gregset.h"
#include "regcache.h"
#include "inferior.h"
#include "gdbthread.h"

#include <pthread_debug.h>

/* Print debugging traces if set to non-zero.  */
static int debug_dec_thread = 0;

/* Non-zero if the dec-thread layer is active.  */
static int dec_thread_active = 0;

/* The pthread_debug context.  */
pthreadDebugContext_t debug_context;

/* The dec-thread target_ops structure.  */
static struct target_ops dec_thread_ops;

/* Print a debug trace if DEBUG_DEC_THREAD is set (its value is adjusted
   by the user using "set debug dec-thread ...").  */

static void
debug (char *format, ...)
{
  if (debug_dec_thread)
    {
      va_list args;

      va_start (args, format);
      printf_unfiltered ("DEC Threads: ");
      vprintf_unfiltered (format, args);
      printf_unfiltered ("\n");
      va_end (args);
    }
}

/* pthread debug callbacks.  */

static int
suspend_clbk (void *caller_context)
{
  return ESUCCESS;
}

static int
resume_clbk (void *caller_context)
{
  return ESUCCESS;
} 

static int
hold_clbk (void *caller_context, pthreadDebugKId_t kernel_tid)
{ 
  return ESUCCESS;
}

static int
unhold_clbk (void *caller_context, pthreadDebugKId_t kernel_tid)
{
  return ESUCCESS;
}

static int
read_clbk (void *caller_context, void *address, void *buffer,
           unsigned long size)
{
  int status = target_read_memory ((CORE_ADDR) address, buffer, size);

  if (status != 0)
    return EINVAL;

  return ESUCCESS;
}

static int
write_clbk (void *caller_context, void *address, void *buffer,
            unsigned long size)
{
  int status = target_write_memory ((CORE_ADDR) address, buffer, size);
  
  if (status != 0)
    return EINVAL;

  return ESUCCESS;
}

/* Get integer regs.  */

static int
get_reg_clbk(void *caller_context, pthreadDebugGetRegRtn_t regs,
             pthreadDebugKId_t kernel_tid)
{
  debug ("get_reg_clbk");

  /* Not sure that we actually need to do anything in this callback.  */
  return ESUCCESS;
}

/* Set integer regs.  */

static int
set_reg_clbk(void *caller_context, const pthreadDebugRegs_t *regs,
             pthreadDebugKId_t kernel_tid)
{
  debug ("set_reg_clbk");

  /* Not sure that we actually need to do anything in this callback.  */
  return ESUCCESS;
}

static int
output_clbk (void *caller_context, char *line)
{
  printf_filtered ("%s\n", line);
  return ESUCCESS;
}

static int
error_clbk (void *caller_context, char *line)
{
  fprintf_filtered (gdb_stderr, "%s\n", line);
  return ESUCCESS;
}

/* Get floating-point regs.  */

static int
get_fpreg_clbk (void *caller_context, pthreadDebugFregs_p fregs,
                pthreadDebugKId_t kernel_tid)
{
  debug ("get_fpreg_clbk");

  /* Not sure that we actually need to do anything in this callback.  */
  return ESUCCESS;
}

/* Set floating-point regs.  */

static int
set_fpreg_clbk (void *caller_context, const pthreadDebugFregs_t *fregs,
                pthreadDebugKId_t kernel_tid)
{
  debug ("set_fpreg_clbk");

  /* Not sure that we actually need to do anything in this callback.  */
  return ESUCCESS;
}

static void *
malloc_clbk (void *caller_context, size_t size)
{
  return xmalloc (size);
}

static void
free_clbk (void *caller_context, void *address)
{
  xfree (address);
}

static int
kthdinfo_clbk (pthreadDebugClient_t caller_context,
               pthreadDebugKId_t kernel_tid,
               pthreadDebugKThreadInfo_p thread_info)
{
  return ENOTSUP;
}

static int
speckthd_clbk (pthreadDebugClient_t caller_context,
               pthreadDebugSpecialType_t type,
               pthreadDebugKId_t *kernel_tid)
{
  return ENOTSUP;
}

static pthreadDebugCallbacks_t debug_callbacks =
{
  PTHREAD_DEBUG_VERSION,
  (pthreadDebugGetMemRtn_t) read_clbk,
  (pthreadDebugSetMemRtn_t) write_clbk,
  suspend_clbk,
  resume_clbk,
  kthdinfo_clbk,
  hold_clbk,
  unhold_clbk,
  (pthreadDebugGetFregRtn_t) get_fpreg_clbk,
  (pthreadDebugSetFregRtn_t) set_fpreg_clbk,
  (pthreadDebugGetRegRtn_t) get_reg_clbk,
  (pthreadDebugSetRegRtn_t) set_reg_clbk,
  (pthreadDebugOutputRtn_t) output_clbk,
  (pthreadDebugOutputRtn_t) error_clbk,
  malloc_clbk,
  free_clbk,
  speckthd_clbk
};

/* Activate thread support if appropriate.  Do nothing if thread
   support is already active.  */

static void
enable_dec_thread (void)
{
  struct minimal_symbol *msym;
  void* caller_context;
  int status;

  /* If already active, nothing more to do.  */
  if (dec_thread_active)
    return;

  msym = lookup_minimal_symbol ("__pthread_dbg_symtable", NULL, NULL);
  if (msym == NULL)
    {
      debug ("enable_dec_thread: No __pthread_dbg_symtable");
      return;
    }

  status = pthreadDebugContextInit (&caller_context, &debug_callbacks,
                                    (void *) SYMBOL_VALUE_ADDRESS (msym),
                                    &debug_context);
  if (status != ESUCCESS)
    {
      debug ("enable_dec_thread: pthreadDebugContextInit -> %d",
             status);
      return;
    }

  push_target (&dec_thread_ops);
  dec_thread_active = 1;

  debug ("enable_dec_thread: Thread support enabled.");
}

/* Deactivate thread support.  Do nothing if thread support is
   already inactive.  */

static void
disable_dec_thread (void)
{
  if (!dec_thread_active)
    return;

  pthreadDebugContextDestroy (debug_context);
  unpush_target (&dec_thread_ops);
  dec_thread_active = 0;
}

/* A structure that contains a thread ID and is associated
   pthreadDebugThreadInfo_t data.  */

struct dec_thread_info
{
  pthreadDebugId_t thread;
  pthreadDebugThreadInfo_t info;
};
typedef struct dec_thread_info dec_thread_info_s;

/* The list of user threads.  */

DEF_VEC_O (dec_thread_info_s);
VEC(dec_thread_info_s) *dec_thread_list;

/* Release the memory used by the given VECP thread list pointer.
   Then set *VECP to NULL.  */

static void
free_dec_thread_info_vec (VEC(dec_thread_info_s) **vecp)
{
  int i;
  struct dec_thread_info *item;
  VEC(dec_thread_info_s) *vec = *vecp;

  for (i = 0; VEC_iterate (dec_thread_info_s, vec, i, item); i++)
     xfree (item);
  VEC_free (dec_thread_info_s, vec);
  *vecp = NULL;
}

/* Return a thread's ptid given its associated INFO.  */

static ptid_t
ptid_build_from_info (struct dec_thread_info info)
{
  int pid = ptid_get_pid (inferior_ptid);

  return ptid_build (pid, 0, (long) info.thread);
}

/* Return non-zero if PTID is still alive.

   Assumes that DEC_THREAD_LIST is up to date.  */
static int
dec_thread_ptid_is_alive (ptid_t ptid)
{
  pthreadDebugId_t tid = ptid_get_tid (ptid);
  int i;
  struct dec_thread_info *info;

  if (tid == 0)
    /* This is the thread corresponding to the process.  This ptid
       is always alive until the program exits.  */
    return 1;

  /* Search whether an entry with the same tid exists in the dec-thread
     list of threads.  If it does, then the thread is still alive.
     No match found means that the thread must be dead, now.  */
  for (i = 0; VEC_iterate (dec_thread_info_s, dec_thread_list, i, info); i++)
    if (info->thread == tid)
      return 1;
  return 0;
}

/* Recompute the list of user threads and store the result in
   DEC_THREAD_LIST.  */

static void
update_dec_thread_list (void)
{
  pthreadDebugId_t thread;
  pthreadDebugThreadInfo_t info;
  int res;

  free_dec_thread_info_vec (&dec_thread_list);
  res = pthreadDebugThdSeqInit (debug_context, &thread);
  while (res == ESUCCESS)
    {

      res = pthreadDebugThdGetInfo (debug_context, thread, &info);
      if (res != ESUCCESS)
        warning (_("unable to get thread info, ignoring thread %ld"),
                   thread);
      else if (info.kind == PTHREAD_DEBUG_THD_KIND_INITIAL
               || info.kind == PTHREAD_DEBUG_THD_KIND_NORMAL)
        {
          struct dec_thread_info *item = 
            xmalloc (sizeof (struct dec_thread_info));

          item->thread = thread;
          item->info = info;
          VEC_safe_push (dec_thread_info_s, dec_thread_list, item);
        }
      res = pthreadDebugThdSeqNext (debug_context, &thread);
    }
  pthreadDebugThdSeqDestroy (debug_context);
}

/* A callback to count the number of threads known to GDB.  */

static int
dec_thread_count_gdb_threads (struct thread_info *ignored, void *context)
{
  int *count = (int *) context;

  *count = *count + 1;
  return 0;
}

/* A callback that saves the given thread INFO at the end of an
   array.  The end of the array is given in the CONTEXT and is
   incremented once the info has been added.  */

static int
dec_thread_add_gdb_thread (struct thread_info *info, void *context)
{
  struct thread_info ***listp = (struct thread_info ***) context;
  
  **listp = info;
  *listp = *listp + 1;
  return 0;
}

/* Implement the find_new_thread target_ops method.  */

static void
dec_thread_find_new_threads (struct target_ops *ops)
{
  int i;
  struct dec_thread_info *info;

  update_dec_thread_list ();
  for (i = 0; VEC_iterate (dec_thread_info_s, dec_thread_list, i, info); i++)
    {
      ptid_t ptid = ptid_build_from_info (*info);

      if (!in_thread_list (ptid))
        add_thread (ptid);
    }
}

/* Resynchronize the list of threads known by GDB with the actual
   list of threads reported by libpthread_debug.  */

static void
resync_thread_list (struct target_ops *ops)
{
  int i;
  int num_gdb_threads = 0;
  struct thread_info **gdb_thread_list;
  struct thread_info **next_thread_info;

  /* Add new threads.  */
  dec_thread_find_new_threads (ops);

  /* Remove threads that no longer exist.  To help with the search,
     we build an array of GDB threads, and then iterate over this
     array.  */

  iterate_over_threads (dec_thread_count_gdb_threads,
                        (void *) &num_gdb_threads);
  gdb_thread_list = alloca (num_gdb_threads * sizeof (struct thread_info *));
  next_thread_info = gdb_thread_list;
  iterate_over_threads (dec_thread_add_gdb_thread, (void *) &next_thread_info);

  for (i = 0; i < num_gdb_threads; i++)
    if (!dec_thread_ptid_is_alive (gdb_thread_list[i]->ptid))
      delete_thread (gdb_thread_list[i]->ptid);
}

/* The "to_detach" method of the dec_thread_ops.  */

static void
dec_thread_detach (struct target_ops *ops, char *args, int from_tty)
{   
  struct target_ops *beneath = find_target_beneath (ops);

  debug ("dec_thread_detach");

  disable_dec_thread ();
  beneath->to_detach (beneath, args, from_tty);
}

/* Return the ptid of the thread that is currently active.  */

static ptid_t
get_active_ptid (void)
{
  int i;
  struct dec_thread_info *info;

  for (i = 0; VEC_iterate (dec_thread_info_s, dec_thread_list, i, info);
       i++)
    if (info->info.state == PTHREAD_DEBUG_STATE_RUNNING)
      return ptid_build_from_info (*info);

  /* No active thread found.  This can happen when the program
     has just exited.  */
  return null_ptid;
}

/* The "to_wait" method of the dec_thread_ops.  */

static ptid_t
dec_thread_wait (struct target_ops *ops,
		 ptid_t ptid, struct target_waitstatus *status, int options)
{
  ptid_t active_ptid;
  struct target_ops *beneath = find_target_beneath (ops);

  debug ("dec_thread_wait");

  ptid = beneath->to_wait (beneath, ptid, status, options);

  /* The ptid returned by the target beneath us is the ptid of the process.
     We need to find which thread is currently active and return its ptid.  */
  resync_thread_list (ops);
  active_ptid = get_active_ptid ();
  if (ptid_equal (active_ptid, null_ptid))
    return ptid;
  return active_ptid;
}

/* Fetch the general purpose and floating point registers for the given
   thread TID, and store the result in GREGSET and FPREGSET.  Return
   zero if successful.  */

static int
dec_thread_get_regsets (pthreadDebugId_t tid, gdb_gregset_t *gregset,
                        gdb_fpregset_t *fpregset)
{
  int res;
  pthreadDebugRegs_t regs;
  pthreadDebugFregs_t fregs;

  res = pthreadDebugThdGetReg (debug_context, tid, &regs);
  if (res != ESUCCESS)
    {
      debug ("dec_thread_get_regsets: pthreadDebugThdGetReg -> %d", res);
      return -1;
    }
  memcpy (gregset->regs, &regs, sizeof (regs));

  res = pthreadDebugThdGetFreg (debug_context, tid, &fregs);
  if (res != ESUCCESS)
    {
      debug ("dec_thread_get_regsets: pthreadDebugThdGetFreg -> %d", res);
      return -1;
    }
  memcpy (fpregset->regs, &fregs, sizeof (fregs));

  return 0;
}

/* The "to_fetch_registers" method of the dec_thread_ops.

   Because the dec-thread debug API doesn't allow us to fetch
   only one register, we simply ignore regno and fetch+supply all
   registers.  */

static void
dec_thread_fetch_registers (struct target_ops *ops,
                            struct regcache *regcache, int regno)
{
  pthreadDebugId_t tid = ptid_get_tid (inferior_ptid);
  gregset_t gregset;
  fpregset_t fpregset;
  int res;

  debug ("dec_thread_fetch_registers (tid=%ld, regno=%d)", tid, regno);


  if (tid == 0 || ptid_equal (inferior_ptid, get_active_ptid ()))
    {
      struct target_ops *beneath = find_target_beneath (ops);

      beneath->to_fetch_registers (beneath, regcache, regno);
      return;
    }

  res = dec_thread_get_regsets (tid, &gregset, &fpregset);
  if (res != 0)
    return;

  supply_gregset (regcache, &gregset);
  supply_fpregset (regcache, &fpregset);
}

/* Store the registers given in GREGSET and FPREGSET into the associated
   general purpose and floating point registers of thread TID.  Return
   zero if successful.  */

static int
dec_thread_set_regsets (pthreadDebugId_t tid, gdb_gregset_t gregset,
                        gdb_fpregset_t fpregset)
{
  int res;
  pthreadDebugRegs_t regs;
  pthreadDebugFregs_t fregs;

  memcpy (&regs, gregset.regs, sizeof (regs));
  res = pthreadDebugThdSetReg (debug_context, tid, &regs);
  if (res != ESUCCESS)
    {
      debug ("dec_thread_set_regsets: pthreadDebugThdSetReg -> %d", res);
      return -1;
    }

  memcpy (&fregs, fpregset.regs, sizeof (fregs));
  res = pthreadDebugThdSetFreg (debug_context, tid, &fregs);
  if (res != ESUCCESS)
    {
      debug ("dec_thread_set_regsets: pthreadDebugThdSetFreg -> %d", res);
      return -1;
    }

  return 0;
}

/* The "to_store_registers" method of the dec_thread_ops.

   Because the dec-thread debug API doesn't allow us to store
   just one register, we store all the registers.  */

static void
dec_thread_store_registers (struct target_ops *ops,
                            struct regcache *regcache, int regno)
{
  pthreadDebugId_t tid = ptid_get_tid (inferior_ptid);
  gregset_t gregset;
  fpregset_t fpregset;
  int res;

  debug ("dec_thread_store_registers (tid=%ld, regno=%d)", tid, regno);

  if (tid == 0 || ptid_equal (inferior_ptid, get_active_ptid ()))
    {
      struct target_ops *beneath = find_target_beneath (ops);

      beneath->to_store_registers (beneath, regcache, regno);
      return;
    }

  /* FIXME: brobecker/2008-05-28: I wonder if we could simply check
     in which register set the register is and then only store the
     registers for that register set, instead of storing both register
     sets.  */
  fill_gregset (regcache, &gregset, -1);
  fill_fpregset (regcache, &fpregset, -1);
  
  res = dec_thread_set_regsets (tid, gregset, fpregset);
  if (res != 0)
    warning (_("failed to store registers."));
}

/* The "to_mourn_inferior" method of the dec_thread_ops.  */

static void
dec_thread_mourn_inferior (struct target_ops *ops)
{
  struct target_ops *beneath = find_target_beneath (ops);

  debug ("dec_thread_mourn_inferior");

  disable_dec_thread ();
  beneath->to_mourn_inferior (beneath);
}

/* The "to_thread_alive" method of the dec_thread_ops.  */
static int
dec_thread_thread_alive (struct target_ops *ops, ptid_t ptid)
{
  debug ("dec_thread_thread_alive (tid=%ld)", ptid_get_tid (ptid));

  /* The thread list maintained by GDB is up to date, since we update
     it everytime we stop.   So check this list.  */
  return in_thread_list (ptid);
}

/* The "to_pid_to_str" method of the dec_thread_ops.  */

static char *
dec_thread_pid_to_str (struct target_ops *ops, ptid_t ptid)
{
  static char *ret = NULL;

  if (ptid_get_tid (ptid) == 0)
    {
      struct target_ops *beneath = find_target_beneath (ops);

      return beneath->to_pid_to_str (beneath, ptid);
    }

  /* Free previous return value; a new one will be allocated by
     xstrprintf().  */
  xfree (ret);

  ret = xstrprintf (_("Thread %ld"), ptid_get_tid (ptid));
  return ret;
}

/* A "new-objfile" observer.  Used to activate/deactivate dec-thread
   support.  */

static void
dec_thread_new_objfile_observer (struct objfile *objfile)
{
  if (objfile != NULL)
     enable_dec_thread ();
  else
     disable_dec_thread ();
}

/* The "to_get_ada_task_ptid" method of the dec_thread_ops.  */

static ptid_t
dec_thread_get_ada_task_ptid (long lwp, long thread)
{
  int i;
  struct dec_thread_info *info;

  debug ("dec_thread_get_ada_task_ptid (lwp=0x%lx, thread=0x%lx)",
         lwp, thread);

  for (i = 0; VEC_iterate (dec_thread_info_s, dec_thread_list, i, info);
       i++)
    if (info->info.teb == (pthread_t) thread)
      return ptid_build_from_info (*info);
  
  warning (_("Could not find thread id from THREAD = 0x%lx"), thread);
  return inferior_ptid;
}

static void
init_dec_thread_ops (void)
{
  dec_thread_ops.to_shortname          = "dec-threads";
  dec_thread_ops.to_longname           = _("DEC threads support");
  dec_thread_ops.to_doc                = _("DEC threads support");
  dec_thread_ops.to_detach             = dec_thread_detach;
  dec_thread_ops.to_wait               = dec_thread_wait;
  dec_thread_ops.to_fetch_registers    = dec_thread_fetch_registers;
  dec_thread_ops.to_store_registers    = dec_thread_store_registers;
  dec_thread_ops.to_mourn_inferior     = dec_thread_mourn_inferior;
  dec_thread_ops.to_thread_alive       = dec_thread_thread_alive;
  dec_thread_ops.to_find_new_threads   = dec_thread_find_new_threads;
  dec_thread_ops.to_pid_to_str         = dec_thread_pid_to_str;
  dec_thread_ops.to_stratum            = thread_stratum;
  dec_thread_ops.to_get_ada_task_ptid  = dec_thread_get_ada_task_ptid;
  dec_thread_ops.to_magic              = OPS_MAGIC;
}

void
_initialize_dec_thread (void)
{
  init_dec_thread_ops ();
  add_target (&dec_thread_ops);

  observer_attach_new_objfile (dec_thread_new_objfile_observer);

  add_setshow_boolean_cmd ("dec-thread", class_maintenance, &debug_dec_thread,
                            _("Set debugging of DEC threads module."),
                            _("Show debugging of DEC threads module."),
                            _("Enables debugging output (used to debug GDB)."),
                            NULL, NULL,
                            &setdebuglist, &showdebuglist);
}
