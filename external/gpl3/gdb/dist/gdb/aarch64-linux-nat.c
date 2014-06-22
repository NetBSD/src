/* Native-dependent code for GNU/Linux AArch64.

   Copyright (C) 2011-2014 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

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

#include "inferior.h"
#include "gdbcore.h"
#include "regcache.h"
#include "linux-nat.h"
#include "target-descriptions.h"
#include "auxv.h"
#include "gdbcmd.h"
#include "aarch64-tdep.h"
#include "aarch64-linux-tdep.h"
#include "elf/common.h"

#include <sys/ptrace.h>
#include <sys/utsname.h>

#include "gregset.h"

#include "features/aarch64.c"

/* Defines ps_err_e, struct ps_prochandle.  */
#include "gdb_proc_service.h"

#ifndef TRAP_HWBKPT
#define TRAP_HWBKPT 0x0004
#endif

/* On GNU/Linux, threads are implemented as pseudo-processes, in which
   case we may be tracing more than one process at a time.  In that
   case, inferior_ptid will contain the main process ID and the
   individual thread (process) ID.  get_thread_id () is used to get
   the thread id if it's available, and the process id otherwise.  */

static int
get_thread_id (ptid_t ptid)
{
  int tid = ptid_get_lwp (ptid);

  if (0 == tid)
    tid = ptid_get_pid (ptid);
  return tid;
}

/* Macro definitions, data structures, and code for the hardware
   breakpoint and hardware watchpoint support follow.  We use the
   following abbreviations throughout the code:

   hw - hardware
   bp - breakpoint
   wp - watchpoint  */

/* Maximum number of hardware breakpoint and watchpoint registers.
   Neither of these values may exceed the width of dr_changed_t
   measured in bits.  */

#define AARCH64_HBP_MAX_NUM 16
#define AARCH64_HWP_MAX_NUM 16

/* Alignment requirement in bytes for addresses written to
   hardware breakpoint and watchpoint value registers.

   A ptrace call attempting to set an address that does not meet the
   alignment criteria will fail.  Limited support has been provided in
   this port for unaligned watchpoints, such that from a GDB user
   perspective, an unaligned watchpoint may be requested.

   This is achieved by minimally enlarging the watched area to meet the
   alignment requirement, and if necessary, splitting the watchpoint
   over several hardware watchpoint registers.  */

#define AARCH64_HBP_ALIGNMENT 4
#define AARCH64_HWP_ALIGNMENT 8

/* The maximum length of a memory region that can be watched by one
   hardware watchpoint register.  */

#define AARCH64_HWP_MAX_LEN_PER_REG 8

/* ptrace hardware breakpoint resource info is formatted as follows:

   31             24             16               8              0
   +---------------+--------------+---------------+---------------+
   |   RESERVED    |   RESERVED   |   DEBUG_ARCH  |  NUM_SLOTS    |
   +---------------+--------------+---------------+---------------+  */


/* Macros to extract fields from the hardware debug information word.  */
#define AARCH64_DEBUG_NUM_SLOTS(x) ((x) & 0xff)
#define AARCH64_DEBUG_ARCH(x) (((x) >> 8) & 0xff)

/* Macro for the expected version of the ARMv8-A debug architecture.  */
#define AARCH64_DEBUG_ARCH_V8 0x6

/* Number of hardware breakpoints/watchpoints the target supports.
   They are initialized with values obtained via the ptrace calls
   with NT_ARM_HW_BREAK and NT_ARM_HW_WATCH respectively.  */

static int aarch64_num_bp_regs;
static int aarch64_num_wp_regs;

/* Debugging of hardware breakpoint/watchpoint support.  */

static int debug_hw_points;

/* Each bit of a variable of this type is used to indicate whether a
   hardware breakpoint or watchpoint setting has been changed since
   the last update.

   Bit N corresponds to the Nth hardware breakpoint or watchpoint
   setting which is managed in aarch64_debug_reg_state, where N is
   valid between 0 and the total number of the hardware breakpoint or
   watchpoint debug registers minus 1.

   When bit N is 1, the corresponding breakpoint or watchpoint setting
   has changed, and therefore the corresponding hardware debug
   register needs to be updated via the ptrace interface.

   In the per-thread arch-specific data area, we define two such
   variables for per-thread hardware breakpoint and watchpoint
   settings respectively.

   This type is part of the mechanism which helps reduce the number of
   ptrace calls to the kernel, i.e. avoid asking the kernel to write
   to the debug registers with unchanged values.  */

typedef unsigned LONGEST dr_changed_t;

/* Set each of the lower M bits of X to 1; assert X is wide enough.  */

#define DR_MARK_ALL_CHANGED(x, m)					\
  do									\
    {									\
      gdb_assert (sizeof ((x)) * 8 >= (m));				\
      (x) = (((dr_changed_t)1 << (m)) - 1);				\
    } while (0)

#define DR_MARK_N_CHANGED(x, n)						\
  do									\
    {									\
      (x) |= ((dr_changed_t)1 << (n));					\
    } while (0)

#define DR_CLEAR_CHANGED(x)						\
  do									\
    {									\
      (x) = 0;								\
    } while (0)

#define DR_HAS_CHANGED(x) ((x) != 0)
#define DR_N_HAS_CHANGED(x, n) ((x) & ((dr_changed_t)1 << (n)))

/* Structure for managing the hardware breakpoint/watchpoint resources.
   DR_ADDR_* stores the address, DR_CTRL_* stores the control register
   content, and DR_REF_COUNT_* counts the numbers of references to the
   corresponding bp/wp, by which way the limited hardware resources
   are not wasted on duplicated bp/wp settings (though so far gdb has
   done a good job by not sending duplicated bp/wp requests).  */

struct aarch64_debug_reg_state
{
  /* hardware breakpoint */
  CORE_ADDR dr_addr_bp[AARCH64_HBP_MAX_NUM];
  unsigned int dr_ctrl_bp[AARCH64_HBP_MAX_NUM];
  unsigned int dr_ref_count_bp[AARCH64_HBP_MAX_NUM];

  /* hardware watchpoint */
  CORE_ADDR dr_addr_wp[AARCH64_HWP_MAX_NUM];
  unsigned int dr_ctrl_wp[AARCH64_HWP_MAX_NUM];
  unsigned int dr_ref_count_wp[AARCH64_HWP_MAX_NUM];
};

/* Per-process data.  We don't bind this to a per-inferior registry
   because of targets like x86 GNU/Linux that need to keep track of
   processes that aren't bound to any inferior (e.g., fork children,
   checkpoints).  */

struct aarch64_process_info
{
  /* Linked list.  */
  struct aarch64_process_info *next;

  /* The process identifier.  */
  pid_t pid;

  /* Copy of aarch64 hardware debug registers.  */
  struct aarch64_debug_reg_state state;
};

static struct aarch64_process_info *aarch64_process_list = NULL;

/* Find process data for process PID.  */

static struct aarch64_process_info *
aarch64_find_process_pid (pid_t pid)
{
  struct aarch64_process_info *proc;

  for (proc = aarch64_process_list; proc; proc = proc->next)
    if (proc->pid == pid)
      return proc;

  return NULL;
}

/* Add process data for process PID.  Returns newly allocated info
   object.  */

static struct aarch64_process_info *
aarch64_add_process (pid_t pid)
{
  struct aarch64_process_info *proc;

  proc = xcalloc (1, sizeof (*proc));
  proc->pid = pid;

  proc->next = aarch64_process_list;
  aarch64_process_list = proc;

  return proc;
}

/* Get data specific info for process PID, creating it if necessary.
   Never returns NULL.  */

static struct aarch64_process_info *
aarch64_process_info_get (pid_t pid)
{
  struct aarch64_process_info *proc;

  proc = aarch64_find_process_pid (pid);
  if (proc == NULL)
    proc = aarch64_add_process (pid);

  return proc;
}

/* Called whenever GDB is no longer debugging process PID.  It deletes
   data structures that keep track of debug register state.  */

static void
aarch64_forget_process (pid_t pid)
{
  struct aarch64_process_info *proc, **proc_link;

  proc = aarch64_process_list;
  proc_link = &aarch64_process_list;

  while (proc != NULL)
    {
      if (proc->pid == pid)
	{
	  *proc_link = proc->next;

	  xfree (proc);
	  return;
	}

      proc_link = &proc->next;
      proc = *proc_link;
    }
}

/* Get debug registers state for process PID.  */

static struct aarch64_debug_reg_state *
aarch64_get_debug_reg_state (pid_t pid)
{
  return &aarch64_process_info_get (pid)->state;
}

/* Per-thread arch-specific data we want to keep.  */

struct arch_lwp_info
{
  /* When bit N is 1, it indicates the Nth hardware breakpoint or
     watchpoint register pair needs to be updated when the thread is
     resumed; see aarch64_linux_prepare_to_resume.  */
  dr_changed_t dr_changed_bp;
  dr_changed_t dr_changed_wp;
};

/* Call ptrace to set the thread TID's hardware breakpoint/watchpoint
   registers with data from *STATE.  */

static void
aarch64_linux_set_debug_regs (const struct aarch64_debug_reg_state *state,
			      int tid, int watchpoint)
{
  int i, count;
  struct iovec iov;
  struct user_hwdebug_state regs;
  const CORE_ADDR *addr;
  const unsigned int *ctrl;

  memset (&regs, 0, sizeof (regs));
  iov.iov_base = &regs;
  count = watchpoint ? aarch64_num_wp_regs : aarch64_num_bp_regs;
  addr = watchpoint ? state->dr_addr_wp : state->dr_addr_bp;
  ctrl = watchpoint ? state->dr_ctrl_wp : state->dr_ctrl_bp;
  if (count == 0)
    return;
  iov.iov_len = (offsetof (struct user_hwdebug_state, dbg_regs[count - 1])
		 + sizeof (regs.dbg_regs [count - 1]));

  for (i = 0; i < count; i++)
    {
      regs.dbg_regs[i].addr = addr[i];
      regs.dbg_regs[i].ctrl = ctrl[i];
    }

  if (ptrace (PTRACE_SETREGSET, tid,
	      watchpoint ? NT_ARM_HW_WATCH : NT_ARM_HW_BREAK,
	      (void *) &iov))
    error (_("Unexpected error setting hardware debug registers"));
}

struct aarch64_dr_update_callback_param
{
  int is_watchpoint;
  unsigned int idx;
};

/* Callback for iterate_over_lwps.  Records the
   information about the change of one hardware breakpoint/watchpoint
   setting for the thread LWP.
   The information is passed in via PTR.
   N.B.  The actual updating of hardware debug registers is not
   carried out until the moment the thread is resumed.  */

static int
debug_reg_change_callback (struct lwp_info *lwp, void *ptr)
{
  struct aarch64_dr_update_callback_param *param_p
    = (struct aarch64_dr_update_callback_param *) ptr;
  int pid = get_thread_id (lwp->ptid);
  int idx = param_p->idx;
  int is_watchpoint = param_p->is_watchpoint;
  struct arch_lwp_info *info = lwp->arch_private;
  dr_changed_t *dr_changed_ptr;
  dr_changed_t dr_changed;

  if (info == NULL)
    info = lwp->arch_private = XCNEW (struct arch_lwp_info);

  if (debug_hw_points)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "debug_reg_change_callback: \n\tOn entry:\n");
      fprintf_unfiltered (gdb_stdlog,
			  "\tpid%d, dr_changed_bp=0x%s, "
			  "dr_changed_wp=0x%s\n",
			  pid, phex (info->dr_changed_bp, 8),
			  phex (info->dr_changed_wp, 8));
    }

  dr_changed_ptr = is_watchpoint ? &info->dr_changed_wp
    : &info->dr_changed_bp;
  dr_changed = *dr_changed_ptr;

  gdb_assert (idx >= 0
	      && (idx <= (is_watchpoint ? aarch64_num_wp_regs
			  : aarch64_num_bp_regs)));

  /* The actual update is done later just before resuming the lwp,
     we just mark that one register pair needs updating.  */
  DR_MARK_N_CHANGED (dr_changed, idx);
  *dr_changed_ptr = dr_changed;

  /* If the lwp isn't stopped, force it to momentarily pause, so
     we can update its debug registers.  */
  if (!lwp->stopped)
    linux_stop_lwp (lwp);

  if (debug_hw_points)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "\tOn exit:\n\tpid%d, dr_changed_bp=0x%s, "
			  "dr_changed_wp=0x%s\n",
			  pid, phex (info->dr_changed_bp, 8),
			  phex (info->dr_changed_wp, 8));
    }

  /* Continue the iteration.  */
  return 0;
}

/* Notify each thread that their IDXth breakpoint/watchpoint register
   pair needs to be updated.  The message will be recorded in each
   thread's arch-specific data area, the actual updating will be done
   when the thread is resumed.  */

static void
aarch64_notify_debug_reg_change (const struct aarch64_debug_reg_state *state,
				 int is_watchpoint, unsigned int idx)
{
  struct aarch64_dr_update_callback_param param;
  ptid_t pid_ptid = pid_to_ptid (ptid_get_pid (inferior_ptid));

  param.is_watchpoint = is_watchpoint;
  param.idx = idx;

  iterate_over_lwps (pid_ptid, debug_reg_change_callback, (void *) &param);
}

/* Print the values of the cached breakpoint/watchpoint registers.  */

static void
aarch64_show_debug_reg_state (struct aarch64_debug_reg_state *state,
			      const char *func, CORE_ADDR addr,
			      int len, int type)
{
  int i;

  fprintf_unfiltered (gdb_stdlog, "%s", func);
  if (addr || len)
    fprintf_unfiltered (gdb_stdlog, " (addr=0x%08lx, len=%d, type=%s)",
			(unsigned long) addr, len,
			type == hw_write ? "hw-write-watchpoint"
			: (type == hw_read ? "hw-read-watchpoint"
			   : (type == hw_access ? "hw-access-watchpoint"
			      : (type == hw_execute ? "hw-breakpoint"
				 : "??unknown??"))));
  fprintf_unfiltered (gdb_stdlog, ":\n");

  fprintf_unfiltered (gdb_stdlog, "\tBREAKPOINTs:\n");
  for (i = 0; i < aarch64_num_bp_regs; i++)
    fprintf_unfiltered (gdb_stdlog,
			"\tBP%d: addr=0x%08lx, ctrl=0x%08x, ref.count=%d\n",
			i, state->dr_addr_bp[i],
			state->dr_ctrl_bp[i], state->dr_ref_count_bp[i]);

  fprintf_unfiltered (gdb_stdlog, "\tWATCHPOINTs:\n");
  for (i = 0; i < aarch64_num_wp_regs; i++)
    fprintf_unfiltered (gdb_stdlog,
			"\tWP%d: addr=0x%08lx, ctrl=0x%08x, ref.count=%d\n",
			i, state->dr_addr_wp[i],
			state->dr_ctrl_wp[i], state->dr_ref_count_wp[i]);
}

/* Fill GDB's register array with the general-purpose register values
   from the current thread.  */

static void
fetch_gregs_from_thread (struct regcache *regcache)
{
  int ret, regno, tid;
  elf_gregset_t regs;
  struct iovec iovec;

  tid = get_thread_id (inferior_ptid);

  iovec.iov_base = &regs;
  iovec.iov_len = sizeof (regs);

  ret = ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to fetch general registers."));

  for (regno = AARCH64_X0_REGNUM; regno <= AARCH64_CPSR_REGNUM; regno++)
    regcache_raw_supply (regcache, regno,
			 (char *) &regs[regno - AARCH64_X0_REGNUM]);
}

/* Store to the current thread the valid general-purpose register
   values in the GDB's register array.  */

static void
store_gregs_to_thread (const struct regcache *regcache)
{
  int ret, regno, tid;
  elf_gregset_t regs;
  struct iovec iovec;

  tid = get_thread_id (inferior_ptid);

  iovec.iov_base = &regs;
  iovec.iov_len = sizeof (regs);

  ret = ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to fetch general registers."));

  for (regno = AARCH64_X0_REGNUM; regno <= AARCH64_CPSR_REGNUM; regno++)
    if (REG_VALID == regcache_register_status (regcache, regno))
      regcache_raw_collect (regcache, regno,
			    (char *) &regs[regno - AARCH64_X0_REGNUM]);

  ret = ptrace (PTRACE_SETREGSET, tid, NT_PRSTATUS, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to store general registers."));
}

/* Fill GDB's register array with the fp/simd register values
   from the current thread.  */

static void
fetch_fpregs_from_thread (struct regcache *regcache)
{
  int ret, regno, tid;
  elf_fpregset_t regs;
  struct iovec iovec;

  tid = get_thread_id (inferior_ptid);

  iovec.iov_base = &regs;
  iovec.iov_len = sizeof (regs);

  ret = ptrace (PTRACE_GETREGSET, tid, NT_FPREGSET, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to fetch FP/SIMD registers."));

  for (regno = AARCH64_V0_REGNUM; regno <= AARCH64_V31_REGNUM; regno++)
    regcache_raw_supply (regcache, regno,
			 (char *) &regs.vregs[regno - AARCH64_V0_REGNUM]);

  regcache_raw_supply (regcache, AARCH64_FPSR_REGNUM, (char *) &regs.fpsr);
  regcache_raw_supply (regcache, AARCH64_FPCR_REGNUM, (char *) &regs.fpcr);
}

/* Store to the current thread the valid fp/simd register
   values in the GDB's register array.  */

static void
store_fpregs_to_thread (const struct regcache *regcache)
{
  int ret, regno, tid;
  elf_fpregset_t regs;
  struct iovec iovec;

  tid = get_thread_id (inferior_ptid);

  iovec.iov_base = &regs;
  iovec.iov_len = sizeof (regs);

  ret = ptrace (PTRACE_GETREGSET, tid, NT_FPREGSET, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to fetch FP/SIMD registers."));

  for (regno = AARCH64_V0_REGNUM; regno <= AARCH64_V31_REGNUM; regno++)
    if (REG_VALID == regcache_register_status (regcache, regno))
      regcache_raw_collect (regcache, regno,
			    (char *) &regs.vregs[regno - AARCH64_V0_REGNUM]);

  if (REG_VALID == regcache_register_status (regcache, AARCH64_FPSR_REGNUM))
    regcache_raw_collect (regcache, AARCH64_FPSR_REGNUM, (char *) &regs.fpsr);
  if (REG_VALID == regcache_register_status (regcache, AARCH64_FPCR_REGNUM))
    regcache_raw_collect (regcache, AARCH64_FPCR_REGNUM, (char *) &regs.fpcr);

  ret = ptrace (PTRACE_SETREGSET, tid, NT_FPREGSET, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to store FP/SIMD registers."));
}

/* Implement the "to_fetch_register" target_ops method.  */

static void
aarch64_linux_fetch_inferior_registers (struct target_ops *ops,
					struct regcache *regcache,
					int regno)
{
  if (regno == -1)
    {
      fetch_gregs_from_thread (regcache);
      fetch_fpregs_from_thread (regcache);
    }
  else if (regno < AARCH64_V0_REGNUM)
    fetch_gregs_from_thread (regcache);
  else
    fetch_fpregs_from_thread (regcache);
}

/* Implement the "to_store_register" target_ops method.  */

static void
aarch64_linux_store_inferior_registers (struct target_ops *ops,
					struct regcache *regcache,
					int regno)
{
  if (regno == -1)
    {
      store_gregs_to_thread (regcache);
      store_fpregs_to_thread (regcache);
    }
  else if (regno < AARCH64_V0_REGNUM)
    store_gregs_to_thread (regcache);
  else
    store_fpregs_to_thread (regcache);
}

/* Fill register REGNO (if it is a general-purpose register) in
   *GREGSETPS with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_gregset (const struct regcache *regcache,
	      gdb_gregset_t *gregsetp, int regno)
{
  gdb_byte *gregs_buf = (gdb_byte *) gregsetp;
  int i;

  for (i = AARCH64_X0_REGNUM; i <= AARCH64_CPSR_REGNUM; i++)
    if (regno == -1 || regno == i)
      regcache_raw_collect (regcache, i,
			    gregs_buf + X_REGISTER_SIZE
			    * (i - AARCH64_X0_REGNUM));
}

/* Fill GDB's register array with the general-purpose register values
   in *GREGSETP.  */

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregsetp)
{
  aarch64_linux_supply_gregset (regcache, (const gdb_byte *) gregsetp);
}

/* Fill register REGNO (if it is a floating-point register) in
   *FPREGSETP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregsetp, int regno)
{
  gdb_byte *fpregs_buf = (gdb_byte *) fpregsetp;
  int i;

  for (i = AARCH64_V0_REGNUM; i <= AARCH64_V31_REGNUM; i++)
    if (regno == -1 || regno == i)
      regcache_raw_collect (regcache, i,
			    fpregs_buf + V_REGISTER_SIZE
			    * (i - AARCH64_V0_REGNUM));

  if (regno == -1 || regno == AARCH64_FPSR_REGNUM)
    regcache_raw_collect (regcache, AARCH64_FPSR_REGNUM,
			  fpregs_buf + V_REGISTER_SIZE * 32);

  if (regno == -1 || regno == AARCH64_FPCR_REGNUM)
    regcache_raw_collect (regcache, AARCH64_FPCR_REGNUM,
			  fpregs_buf + V_REGISTER_SIZE * 32 + 4);
}

/* Fill GDB's register array with the floating-point register values
   in *FPREGSETP.  */

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t *fpregsetp)
{
  aarch64_linux_supply_fpregset (regcache, (const gdb_byte *) fpregsetp);
}

/* Called when resuming a thread.
   The hardware debug registers are updated when there is any change.  */

static void
aarch64_linux_prepare_to_resume (struct lwp_info *lwp)
{
  struct arch_lwp_info *info = lwp->arch_private;

  /* NULL means this is the main thread still going through the shell,
     or, no watchpoint has been set yet.  In that case, there's
     nothing to do.  */
  if (info == NULL)
    return;

  if (DR_HAS_CHANGED (info->dr_changed_bp)
      || DR_HAS_CHANGED (info->dr_changed_wp))
    {
      int tid = ptid_get_lwp (lwp->ptid);
      struct aarch64_debug_reg_state *state
	= aarch64_get_debug_reg_state (ptid_get_pid (lwp->ptid));

      if (debug_hw_points)
	fprintf_unfiltered (gdb_stdlog, "prepare_to_resume thread %d\n", tid);

      /* Watchpoints.  */
      if (DR_HAS_CHANGED (info->dr_changed_wp))
	{
	  aarch64_linux_set_debug_regs (state, tid, 1);
	  DR_CLEAR_CHANGED (info->dr_changed_wp);
	}

      /* Breakpoints.  */
      if (DR_HAS_CHANGED (info->dr_changed_bp))
	{
	  aarch64_linux_set_debug_regs (state, tid, 0);
	  DR_CLEAR_CHANGED (info->dr_changed_bp);
	}
    }
}

static void
aarch64_linux_new_thread (struct lwp_info *lp)
{
  struct arch_lwp_info *info = XCNEW (struct arch_lwp_info);

  /* Mark that all the hardware breakpoint/watchpoint register pairs
     for this thread need to be initialized.  */
  DR_MARK_ALL_CHANGED (info->dr_changed_bp, aarch64_num_bp_regs);
  DR_MARK_ALL_CHANGED (info->dr_changed_wp, aarch64_num_wp_regs);

  lp->arch_private = info;
}

/* linux_nat_new_fork hook.   */

static void
aarch64_linux_new_fork (struct lwp_info *parent, pid_t child_pid)
{
  pid_t parent_pid;
  struct aarch64_debug_reg_state *parent_state;
  struct aarch64_debug_reg_state *child_state;

  /* NULL means no watchpoint has ever been set in the parent.  In
     that case, there's nothing to do.  */
  if (parent->arch_private == NULL)
    return;

  /* GDB core assumes the child inherits the watchpoints/hw
     breakpoints of the parent, and will remove them all from the
     forked off process.  Copy the debug registers mirrors into the
     new process so that all breakpoints and watchpoints can be
     removed together.  */

  parent_pid = ptid_get_pid (parent->ptid);
  parent_state = aarch64_get_debug_reg_state (parent_pid);
  child_state = aarch64_get_debug_reg_state (child_pid);
  *child_state = *parent_state;
}


/* Called by libthread_db.  Returns a pointer to the thread local
   storage (or its descriptor).  */

ps_err_e
ps_get_thread_area (const struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  struct iovec iovec;
  uint64_t reg;

  iovec.iov_base = &reg;
  iovec.iov_len = sizeof (reg);

  if (ptrace (PTRACE_GETREGSET, lwpid, NT_ARM_TLS, &iovec) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) (reg - idx);

  return PS_OK;
}


/* Get the hardware debug register capacity information.  */

static void
aarch64_linux_get_debug_reg_capacity (void)
{
  int tid;
  struct iovec iov;
  struct user_hwdebug_state dreg_state;

  tid = get_thread_id (inferior_ptid);
  iov.iov_base = &dreg_state;
  iov.iov_len = sizeof (dreg_state);

  /* Get hardware watchpoint register info.  */
  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_HW_WATCH, &iov) == 0
      && AARCH64_DEBUG_ARCH (dreg_state.dbg_info) == AARCH64_DEBUG_ARCH_V8)
    {
      aarch64_num_wp_regs = AARCH64_DEBUG_NUM_SLOTS (dreg_state.dbg_info);
      if (aarch64_num_wp_regs > AARCH64_HWP_MAX_NUM)
	{
	  warning (_("Unexpected number of hardware watchpoint registers"
		     " reported by ptrace, got %d, expected %d."),
		   aarch64_num_wp_regs, AARCH64_HWP_MAX_NUM);
	  aarch64_num_wp_regs = AARCH64_HWP_MAX_NUM;
	}
    }
  else
    {
      warning (_("Unable to determine the number of hardware watchpoints"
		 " available."));
      aarch64_num_wp_regs = 0;
    }

  /* Get hardware breakpoint register info.  */
  if (ptrace (PTRACE_GETREGSET, tid, NT_ARM_HW_BREAK, &iov) == 0
      && AARCH64_DEBUG_ARCH (dreg_state.dbg_info) == AARCH64_DEBUG_ARCH_V8)
    {
      aarch64_num_bp_regs = AARCH64_DEBUG_NUM_SLOTS (dreg_state.dbg_info);
      if (aarch64_num_bp_regs > AARCH64_HBP_MAX_NUM)
	{
	  warning (_("Unexpected number of hardware breakpoint registers"
		     " reported by ptrace, got %d, expected %d."),
		   aarch64_num_bp_regs, AARCH64_HBP_MAX_NUM);
	  aarch64_num_bp_regs = AARCH64_HBP_MAX_NUM;
	}
    }
  else
    {
      warning (_("Unable to determine the number of hardware breakpoints"
		 " available."));
      aarch64_num_bp_regs = 0;
    }
}

static void (*super_post_startup_inferior) (ptid_t ptid);

/* Implement the "to_post_startup_inferior" target_ops method.  */

static void
aarch64_linux_child_post_startup_inferior (ptid_t ptid)
{
  aarch64_forget_process (ptid_get_pid (ptid));
  aarch64_linux_get_debug_reg_capacity ();
  super_post_startup_inferior (ptid);
}

/* Implement the "to_read_description" target_ops method.  */

static const struct target_desc *
aarch64_linux_read_description (struct target_ops *ops)
{
  initialize_tdesc_aarch64 ();
  return tdesc_aarch64;
}

/* Given the (potentially unaligned) watchpoint address in ADDR and
   length in LEN, return the aligned address and aligned length in
   *ALIGNED_ADDR_P and *ALIGNED_LEN_P, respectively.  The returned
   aligned address and length will be valid values to write to the
   hardware watchpoint value and control registers.

   The given watchpoint may get truncated if more than one hardware
   register is needed to cover the watched region.  *NEXT_ADDR_P
   and *NEXT_LEN_P, if non-NULL, will return the address and length
   of the remaining part of the watchpoint (which can be processed
   by calling this routine again to generate another aligned address
   and length pair.

   See the comment above the function of the same name in
   gdbserver/linux-aarch64-low.c for more information.  */

static void
aarch64_align_watchpoint (CORE_ADDR addr, int len, CORE_ADDR *aligned_addr_p,
			  int *aligned_len_p, CORE_ADDR *next_addr_p,
			  int *next_len_p)
{
  int aligned_len;
  unsigned int offset;
  CORE_ADDR aligned_addr;
  const unsigned int alignment = AARCH64_HWP_ALIGNMENT;
  const unsigned int max_wp_len = AARCH64_HWP_MAX_LEN_PER_REG;

  /* As assumed by the algorithm.  */
  gdb_assert (alignment == max_wp_len);

  if (len <= 0)
    return;

  /* Address to be put into the hardware watchpoint value register
     must be aligned.  */
  offset = addr & (alignment - 1);
  aligned_addr = addr - offset;

  gdb_assert (offset >= 0 && offset < alignment);
  gdb_assert (aligned_addr >= 0 && aligned_addr <= addr);
  gdb_assert (offset + len > 0);

  if (offset + len >= max_wp_len)
    {
      /* Need more than one watchpoint registers; truncate it at the
         alignment boundary.  */
      aligned_len = max_wp_len;
      len -= (max_wp_len - offset);
      addr += (max_wp_len - offset);
      gdb_assert ((addr & (alignment - 1)) == 0);
    }
  else
    {
      /* Find the smallest valid length that is large enough to
	 accommodate this watchpoint.  */
      static const unsigned char
	aligned_len_array[AARCH64_HWP_MAX_LEN_PER_REG] =
	{ 1, 2, 4, 4, 8, 8, 8, 8 };

      aligned_len = aligned_len_array[offset + len - 1];
      addr += len;
      len = 0;
    }

  if (aligned_addr_p)
    *aligned_addr_p = aligned_addr;
  if (aligned_len_p)
    *aligned_len_p = aligned_len;
  if (next_addr_p)
    *next_addr_p = addr;
  if (next_len_p)
    *next_len_p = len;
}

/* Returns the number of hardware watchpoints of type TYPE that we can
   set.  Value is positive if we can set CNT watchpoints, zero if
   setting watchpoints of type TYPE is not supported, and negative if
   CNT is more than the maximum number of watchpoints of type TYPE
   that we can support.  TYPE is one of bp_hardware_watchpoint,
   bp_read_watchpoint, bp_write_watchpoint, or bp_hardware_breakpoint.
   CNT is the number of such watchpoints used so far (including this
   one).  OTHERTYPE is non-zero if other types of watchpoints are
   currently enabled.

   We always return 1 here because we don't have enough information
   about possible overlap of addresses that they want to watch.  As an
   extreme example, consider the case where all the watchpoints watch
   the same address and the same region length: then we can handle a
   virtually unlimited number of watchpoints, due to debug register
   sharing implemented via reference counts.  */

static int
aarch64_linux_can_use_hw_breakpoint (int type, int cnt, int othertype)
{
  return 1;
}

/* ptrace expects control registers to be formatted as follows:

   31                             13          5      3      1     0
   +--------------------------------+----------+------+------+----+
   |         RESERVED (SBZ)         |  LENGTH  | TYPE | PRIV | EN |
   +--------------------------------+----------+------+------+----+

   The TYPE field is ignored for breakpoints.  */

#define DR_CONTROL_ENABLED(ctrl)	(((ctrl) & 0x1) == 1)
#define DR_CONTROL_LENGTH(ctrl)		(((ctrl) >> 5) & 0xff)

/* Utility function that returns the length in bytes of a watchpoint
   according to the content of a hardware debug control register CTRL.
   Note that the kernel currently only supports the following Byte
   Address Select (BAS) values: 0x1, 0x3, 0xf and 0xff, which means
   that for a hardware watchpoint, its valid length can only be 1
   byte, 2 bytes, 4 bytes or 8 bytes.  */

static inline unsigned int
aarch64_watchpoint_length (unsigned int ctrl)
{
  switch (DR_CONTROL_LENGTH (ctrl))
    {
    case 0x01:
      return 1;
    case 0x03:
      return 2;
    case 0x0f:
      return 4;
    case 0xff:
      return 8;
    default:
      return 0;
    }
}

/* Given the hardware breakpoint or watchpoint type TYPE and its
   length LEN, return the expected encoding for a hardware
   breakpoint/watchpoint control register.  */

static unsigned int
aarch64_point_encode_ctrl_reg (int type, int len)
{
  unsigned int ctrl, ttype;

  /* type */
  switch (type)
    {
    case hw_write:
      ttype = 2;
      break;
    case hw_read:
      ttype = 1;
      break;
    case hw_access:
      ttype = 3;
      break;
    case hw_execute:
      ttype = 0;
      break;
    default:
      perror_with_name (_("Unrecognized breakpoint/watchpoint type"));
    }
  ctrl = ttype << 3;

  /* length bitmask */
  ctrl |= ((1 << len) - 1) << 5;
  /* enabled at el0 */
  ctrl |= (2 << 1) | 1;

  return ctrl;
}

/* Addresses to be written to the hardware breakpoint and watchpoint
   value registers need to be aligned; the alignment is 4-byte and
   8-type respectively.  Linux kernel rejects any non-aligned address
   it receives from the related ptrace call.  Furthermore, the kernel
   currently only supports the following Byte Address Select (BAS)
   values: 0x1, 0x3, 0xf and 0xff, which means that for a hardware
   watchpoint to be accepted by the kernel (via ptrace call), its
   valid length can only be 1 byte, 2 bytes, 4 bytes or 8 bytes.
   Despite these limitations, the unaligned watchpoint is supported in
   this port.

   Return 0 for any non-compliant ADDR and/or LEN; return 1 otherwise.  */

static int
aarch64_point_is_aligned (int is_watchpoint, CORE_ADDR addr, int len)
{
  unsigned int alignment = is_watchpoint ? AARCH64_HWP_ALIGNMENT
    : AARCH64_HBP_ALIGNMENT;

  if (addr & (alignment - 1))
    return 0;

  if (len != 8 && len != 4 && len != 2 && len != 1)
    return 0;

  return 1;
}

/* Record the insertion of one breakpoint/watchpoint, as represented
   by ADDR and CTRL, in the cached debug register state area *STATE.  */

static int
aarch64_dr_state_insert_one_point (struct aarch64_debug_reg_state *state,
				   int type, CORE_ADDR addr, int len)
{
  int i, idx, num_regs, is_watchpoint;
  unsigned int ctrl, *dr_ctrl_p, *dr_ref_count;
  CORE_ADDR *dr_addr_p;

  /* Set up state pointers.  */
  is_watchpoint = (type != hw_execute);
  gdb_assert (aarch64_point_is_aligned (is_watchpoint, addr, len));
  if (is_watchpoint)
    {
      num_regs = aarch64_num_wp_regs;
      dr_addr_p = state->dr_addr_wp;
      dr_ctrl_p = state->dr_ctrl_wp;
      dr_ref_count = state->dr_ref_count_wp;
    }
  else
    {
      num_regs = aarch64_num_bp_regs;
      dr_addr_p = state->dr_addr_bp;
      dr_ctrl_p = state->dr_ctrl_bp;
      dr_ref_count = state->dr_ref_count_bp;
    }

  ctrl = aarch64_point_encode_ctrl_reg (type, len);

  /* Find an existing or free register in our cache.  */
  idx = -1;
  for (i = 0; i < num_regs; ++i)
    {
      if ((dr_ctrl_p[i] & 1) == 0)
	{
	  gdb_assert (dr_ref_count[i] == 0);
	  idx = i;
	  /* no break; continue hunting for an existing one.  */
	}
      else if (dr_addr_p[i] == addr && dr_ctrl_p[i] == ctrl)
	{
	  gdb_assert (dr_ref_count[i] != 0);
	  idx = i;
	  break;
	}
    }

  /* No space.  */
  if (idx == -1)
    return -1;

  /* Update our cache.  */
  if ((dr_ctrl_p[idx] & 1) == 0)
    {
      /* new entry */
      dr_addr_p[idx] = addr;
      dr_ctrl_p[idx] = ctrl;
      dr_ref_count[idx] = 1;
      /* Notify the change.  */
      aarch64_notify_debug_reg_change (state, is_watchpoint, idx);
    }
  else
    {
      /* existing entry */
      dr_ref_count[idx]++;
    }

  return 0;
}

/* Record the removal of one breakpoint/watchpoint, as represented by
   ADDR and CTRL, in the cached debug register state area *STATE.  */

static int
aarch64_dr_state_remove_one_point (struct aarch64_debug_reg_state *state,
				   int type, CORE_ADDR addr, int len)
{
  int i, num_regs, is_watchpoint;
  unsigned int ctrl, *dr_ctrl_p, *dr_ref_count;
  CORE_ADDR *dr_addr_p;

  /* Set up state pointers.  */
  is_watchpoint = (type != hw_execute);
  gdb_assert (aarch64_point_is_aligned (is_watchpoint, addr, len));
  if (is_watchpoint)
    {
      num_regs = aarch64_num_wp_regs;
      dr_addr_p = state->dr_addr_wp;
      dr_ctrl_p = state->dr_ctrl_wp;
      dr_ref_count = state->dr_ref_count_wp;
    }
  else
    {
      num_regs = aarch64_num_bp_regs;
      dr_addr_p = state->dr_addr_bp;
      dr_ctrl_p = state->dr_ctrl_bp;
      dr_ref_count = state->dr_ref_count_bp;
    }

  ctrl = aarch64_point_encode_ctrl_reg (type, len);

  /* Find the entry that matches the ADDR and CTRL.  */
  for (i = 0; i < num_regs; ++i)
    if (dr_addr_p[i] == addr && dr_ctrl_p[i] == ctrl)
      {
	gdb_assert (dr_ref_count[i] != 0);
	break;
      }

  /* Not found.  */
  if (i == num_regs)
    return -1;

  /* Clear our cache.  */
  if (--dr_ref_count[i] == 0)
    {
      /* Clear the enable bit.  */
      ctrl &= ~1;
      dr_addr_p[i] = 0;
      dr_ctrl_p[i] = ctrl;
      /* Notify the change.  */
      aarch64_notify_debug_reg_change (state, is_watchpoint, i);
    }

  return 0;
}

/* Implement insertion and removal of a single breakpoint.  */

static int
aarch64_handle_breakpoint (int type, CORE_ADDR addr, int len, int is_insert)
{
  struct aarch64_debug_reg_state *state;

  /* The hardware breakpoint on AArch64 should always be 4-byte
     aligned.  */
  if (!aarch64_point_is_aligned (0 /* is_watchpoint */ , addr, len))
    return -1;

  state = aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));

  if (is_insert)
    return aarch64_dr_state_insert_one_point (state, type, addr, len);
  else
    return aarch64_dr_state_remove_one_point (state, type, addr, len);
}

/* Insert a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, -1 on failure.  */

static int
aarch64_linux_insert_hw_breakpoint (struct gdbarch *gdbarch,
				    struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address;
  const int len = 4;
  const int type = hw_execute;

  if (debug_hw_points)
    fprintf_unfiltered
      (gdb_stdlog,
       "insert_hw_breakpoint on entry (addr=0x%08lx, len=%d))\n",
       (unsigned long) addr, len);

  ret = aarch64_handle_breakpoint (type, addr, len, 1 /* is_insert */);

  if (debug_hw_points > 1)
    {
      struct aarch64_debug_reg_state *state
	= aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));

      aarch64_show_debug_reg_state (state,
				    "insert_hw_watchpoint", addr, len, type);
    }

  return ret;
}

/* Remove a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, -1 on failure.  */

static int
aarch64_linux_remove_hw_breakpoint (struct gdbarch *gdbarch,
				    struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address;
  const int len = 4;
  const int type = hw_execute;

  if (debug_hw_points)
    fprintf_unfiltered
      (gdb_stdlog, "remove_hw_breakpoint on entry (addr=0x%08lx, len=%d))\n",
       (unsigned long) addr, len);

  ret = aarch64_handle_breakpoint (type, addr, len, 0 /* is_insert */);

  if (debug_hw_points > 1)
    {
      struct aarch64_debug_reg_state *state
	= aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));

      aarch64_show_debug_reg_state (state,
				    "remove_hw_watchpoint", addr, len, type);
    }

  return ret;
}

/* This is essentially the same as aarch64_handle_breakpoint, apart
   from that it is an aligned watchpoint to be handled.  */

static int
aarch64_handle_aligned_watchpoint (int type, CORE_ADDR addr, int len,
				   int is_insert)
{
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));

  if (is_insert)
    return aarch64_dr_state_insert_one_point (state, type, addr, len);
  else
    return aarch64_dr_state_remove_one_point (state, type, addr, len);
}

/* Insert/remove unaligned watchpoint by calling
   aarch64_align_watchpoint repeatedly until the whole watched region,
   as represented by ADDR and LEN, has been properly aligned and ready
   to be written to one or more hardware watchpoint registers.
   IS_INSERT indicates whether this is an insertion or a deletion.
   Return 0 if succeed.  */

static int
aarch64_handle_unaligned_watchpoint (int type, CORE_ADDR addr, int len,
				     int is_insert)
{
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));

  while (len > 0)
    {
      CORE_ADDR aligned_addr;
      int aligned_len, ret;

      aarch64_align_watchpoint (addr, len, &aligned_addr, &aligned_len,
				&addr, &len);

      if (is_insert)
	ret = aarch64_dr_state_insert_one_point (state, type, aligned_addr,
						 aligned_len);
      else
	ret = aarch64_dr_state_remove_one_point (state, type, aligned_addr,
						 aligned_len);

      if (debug_hw_points)
	fprintf_unfiltered (gdb_stdlog,
"handle_unaligned_watchpoint: is_insert: %d\n"
"                             aligned_addr: 0x%08lx, aligned_len: %d\n"
"                                next_addr: 0x%08lx,    next_len: %d\n",
		 is_insert, aligned_addr, aligned_len, addr, len);

      if (ret != 0)
	return ret;
    }

  return 0;
}

/* Implements insertion and removal of a single watchpoint.  */

static int
aarch64_handle_watchpoint (int type, CORE_ADDR addr, int len, int is_insert)
{
  if (aarch64_point_is_aligned (1 /* is_watchpoint */ , addr, len))
    return aarch64_handle_aligned_watchpoint (type, addr, len, is_insert);
  else
    return aarch64_handle_unaligned_watchpoint (type, addr, len, is_insert);
}

/* Implement the "to_insert_watchpoint" target_ops method.

   Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */

static int
aarch64_linux_insert_watchpoint (CORE_ADDR addr, int len, int type,
				 struct expression *cond)
{
  int ret;

  if (debug_hw_points)
    fprintf_unfiltered (gdb_stdlog,
			"insert_watchpoint on entry (addr=0x%08lx, len=%d)\n",
			(unsigned long) addr, len);

  gdb_assert (type != hw_execute);

  ret = aarch64_handle_watchpoint (type, addr, len, 1 /* is_insert */);

  if (debug_hw_points > 1)
    {
      struct aarch64_debug_reg_state *state
	= aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));

      aarch64_show_debug_reg_state (state,
				    "insert_watchpoint", addr, len, type);
    }

  return ret;
}

/* Implement the "to_remove_watchpoint" target_ops method.
   Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE.  Return 0 on success, -1 on failure.  */

static int
aarch64_linux_remove_watchpoint (CORE_ADDR addr, int len, int type,
				 struct expression *cond)
{
  int ret;

  if (debug_hw_points)
    fprintf_unfiltered (gdb_stdlog,
			"remove_watchpoint on entry (addr=0x%08lx, len=%d)\n",
			(unsigned long) addr, len);

  gdb_assert (type != hw_execute);

  ret = aarch64_handle_watchpoint (type, addr, len, 0 /* is_insert */);

  if (debug_hw_points > 1)
    {
      struct aarch64_debug_reg_state *state
	= aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));

      aarch64_show_debug_reg_state (state,
				    "remove_watchpoint", addr, len, type);
    }

  return ret;
}

/* Implement the "to_region_ok_for_hw_watchpoint" target_ops method.  */

static int
aarch64_linux_region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  CORE_ADDR aligned_addr;

  /* Can not set watchpoints for zero or negative lengths.  */
  if (len <= 0)
    return 0;

  /* Must have hardware watchpoint debug register(s).  */
  if (aarch64_num_wp_regs == 0)
    return 0;

  /* We support unaligned watchpoint address and arbitrary length,
     as long as the size of the whole watched area after alignment
     doesn't exceed size of the total area that all watchpoint debug
     registers can watch cooperatively.

     This is a very relaxed rule, but unfortunately there are
     limitations, e.g. false-positive hits, due to limited support of
     hardware debug registers in the kernel.  See comment above
     aarch64_align_watchpoint for more information.  */

  aligned_addr = addr & ~(AARCH64_HWP_MAX_LEN_PER_REG - 1);
  if (aligned_addr + aarch64_num_wp_regs * AARCH64_HWP_MAX_LEN_PER_REG
      < addr + len)
    return 0;

  /* All tests passed so we are likely to be able to set the watchpoint.
     The reason that it is 'likely' rather than 'must' is because
     we don't check the current usage of the watchpoint registers, and
     there may not be enough registers available for this watchpoint.
     Ideally we should check the cached debug register state, however
     the checking is costly.  */
  return 1;
}

/* Implement the "to_stopped_data_address" target_ops method.  */

static int
aarch64_linux_stopped_data_address (struct target_ops *target,
				    CORE_ADDR *addr_p)
{
  siginfo_t siginfo;
  int i, tid;
  struct aarch64_debug_reg_state *state;

  if (!linux_nat_get_siginfo (inferior_ptid, &siginfo))
    return 0;

  /* This must be a hardware breakpoint.  */
  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != TRAP_HWBKPT)
    return 0;

  /* Check if the address matches any watched address.  */
  state = aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));
  for (i = aarch64_num_wp_regs - 1; i >= 0; --i)
    {
      const unsigned int len = aarch64_watchpoint_length (state->dr_ctrl_wp[i]);
      const CORE_ADDR addr_trap = (CORE_ADDR) siginfo.si_addr;
      const CORE_ADDR addr_watch = state->dr_addr_wp[i];

      if (state->dr_ref_count_wp[i]
	  && DR_CONTROL_ENABLED (state->dr_ctrl_wp[i])
	  && addr_trap >= addr_watch
	  && addr_trap < addr_watch + len)
	{
	  *addr_p = addr_trap;
	  return 1;
	}
    }

  return 0;
}

/* Implement the "to_stopped_by_watchpoint" target_ops method.  */

static int
aarch64_linux_stopped_by_watchpoint (void)
{
  CORE_ADDR addr;

  return aarch64_linux_stopped_data_address (&current_target, &addr);
}

/* Implement the "to_watchpoint_addr_within_range" target_ops method.  */

static int
aarch64_linux_watchpoint_addr_within_range (struct target_ops *target,
					    CORE_ADDR addr,
					    CORE_ADDR start, int length)
{
  return start <= addr && start + length - 1 >= addr;
}

/* Define AArch64 maintenance commands.  */

static void
add_show_debug_regs_command (void)
{
  /* A maintenance command to enable printing the internal DRi mirror
     variables.  */
  add_setshow_boolean_cmd ("show-debug-regs", class_maintenance,
			   &debug_hw_points, _("\
Set whether to show variables that mirror the AArch64 debug registers."), _("\
Show whether to show variables that mirror the AArch64 debug registers."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, the debug registers values are shown when GDB inserts\n\
or removes a hardware breakpoint or watchpoint, and when the inferior\n\
triggers a breakpoint or watchpoint."),
			   NULL,
			   NULL,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);
}

/* -Wmissing-prototypes.  */
void _initialize_aarch64_linux_nat (void);

void
_initialize_aarch64_linux_nat (void)
{
  struct target_ops *t;

  /* Fill in the generic GNU/Linux methods.  */
  t = linux_target ();

  add_show_debug_regs_command ();

  /* Add our register access methods.  */
  t->to_fetch_registers = aarch64_linux_fetch_inferior_registers;
  t->to_store_registers = aarch64_linux_store_inferior_registers;

  t->to_read_description = aarch64_linux_read_description;

  t->to_can_use_hw_breakpoint = aarch64_linux_can_use_hw_breakpoint;
  t->to_insert_hw_breakpoint = aarch64_linux_insert_hw_breakpoint;
  t->to_remove_hw_breakpoint = aarch64_linux_remove_hw_breakpoint;
  t->to_region_ok_for_hw_watchpoint =
    aarch64_linux_region_ok_for_hw_watchpoint;
  t->to_insert_watchpoint = aarch64_linux_insert_watchpoint;
  t->to_remove_watchpoint = aarch64_linux_remove_watchpoint;
  t->to_stopped_by_watchpoint = aarch64_linux_stopped_by_watchpoint;
  t->to_stopped_data_address = aarch64_linux_stopped_data_address;
  t->to_watchpoint_addr_within_range =
    aarch64_linux_watchpoint_addr_within_range;

  /* Override the GNU/Linux inferior startup hook.  */
  super_post_startup_inferior = t->to_post_startup_inferior;
  t->to_post_startup_inferior = aarch64_linux_child_post_startup_inferior;

  /* Register the target.  */
  linux_nat_add_target (t);
  linux_nat_set_new_thread (t, aarch64_linux_new_thread);
  linux_nat_set_new_fork (t, aarch64_linux_new_fork);
  linux_nat_set_forget_process (t, aarch64_forget_process);
  linux_nat_set_prepare_to_resume (t, aarch64_linux_prepare_to_resume);
}
