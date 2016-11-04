/* Native-dependent code for GNU/Linux AArch64.

   Copyright (C) 2011-2016 Free Software Foundation, Inc.
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
#include "aarch32-linux-nat.h"
#include "nat/aarch64-linux.h"
#include "nat/aarch64-linux-hw-point.h"

#include "elf/external.h"
#include "elf/common.h"

#include "nat/gdb_ptrace.h"
#include <sys/utsname.h>
#include <asm/ptrace.h>

#include "gregset.h"

/* Defines ps_err_e, struct ps_prochandle.  */
#include "gdb_proc_service.h"

#ifndef TRAP_HWBKPT
#define TRAP_HWBKPT 0x0004
#endif

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

  proc = XCNEW (struct aarch64_process_info);
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

struct aarch64_debug_reg_state *
aarch64_get_debug_reg_state (pid_t pid)
{
  return &aarch64_process_info_get (pid)->state;
}

/* Fill GDB's register array with the general-purpose register values
   from the current thread.  */

static void
fetch_gregs_from_thread (struct regcache *regcache)
{
  int ret, tid;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  elf_gregset_t regs;
  struct iovec iovec;

  /* Make sure REGS can hold all registers contents on both aarch64
     and arm.  */
  gdb_static_assert (sizeof (regs) >= 18 * 4);

  tid = ptid_get_lwp (inferior_ptid);

  iovec.iov_base = &regs;
  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    iovec.iov_len = 18 * 4;
  else
    iovec.iov_len = sizeof (regs);

  ret = ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to fetch general registers."));

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    aarch32_gp_regcache_supply (regcache, (uint32_t *) regs, 1);
  else
    {
      int regno;

      for (regno = AARCH64_X0_REGNUM; regno <= AARCH64_CPSR_REGNUM; regno++)
	regcache_raw_supply (regcache, regno, &regs[regno - AARCH64_X0_REGNUM]);
    }
}

/* Store to the current thread the valid general-purpose register
   values in the GDB's register array.  */

static void
store_gregs_to_thread (const struct regcache *regcache)
{
  int ret, tid;
  elf_gregset_t regs;
  struct iovec iovec;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  /* Make sure REGS can hold all registers contents on both aarch64
     and arm.  */
  gdb_static_assert (sizeof (regs) >= 18 * 4);
  tid = ptid_get_lwp (inferior_ptid);

  iovec.iov_base = &regs;
  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    iovec.iov_len = 18 * 4;
  else
    iovec.iov_len = sizeof (regs);

  ret = ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to fetch general registers."));

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    aarch32_gp_regcache_collect (regcache, (uint32_t *) regs, 1);
  else
    {
      int regno;

      for (regno = AARCH64_X0_REGNUM; regno <= AARCH64_CPSR_REGNUM; regno++)
	if (REG_VALID == regcache_register_status (regcache, regno))
	  regcache_raw_collect (regcache, regno,
				&regs[regno - AARCH64_X0_REGNUM]);
    }

  ret = ptrace (PTRACE_SETREGSET, tid, NT_PRSTATUS, &iovec);
  if (ret < 0)
    perror_with_name (_("Unable to store general registers."));
}

/* Fill GDB's register array with the fp/simd register values
   from the current thread.  */

static void
fetch_fpregs_from_thread (struct regcache *regcache)
{
  int ret, tid;
  elf_fpregset_t regs;
  struct iovec iovec;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  /* Make sure REGS can hold all VFP registers contents on both aarch64
     and arm.  */
  gdb_static_assert (sizeof regs >= VFP_REGS_SIZE);

  tid = ptid_get_lwp (inferior_ptid);

  iovec.iov_base = &regs;

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      iovec.iov_len = VFP_REGS_SIZE;

      ret = ptrace (PTRACE_GETREGSET, tid, NT_ARM_VFP, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to fetch VFP registers."));

      aarch32_vfp_regcache_supply (regcache, (gdb_byte *) &regs, 32);
    }
  else
    {
      int regno;

      iovec.iov_len = sizeof (regs);

      ret = ptrace (PTRACE_GETREGSET, tid, NT_FPREGSET, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to fetch vFP/SIMD registers."));

      for (regno = AARCH64_V0_REGNUM; regno <= AARCH64_V31_REGNUM; regno++)
	regcache_raw_supply (regcache, regno,
			     &regs.vregs[regno - AARCH64_V0_REGNUM]);

      regcache_raw_supply (regcache, AARCH64_FPSR_REGNUM, &regs.fpsr);
      regcache_raw_supply (regcache, AARCH64_FPCR_REGNUM, &regs.fpcr);
    }
}

/* Store to the current thread the valid fp/simd register
   values in the GDB's register array.  */

static void
store_fpregs_to_thread (const struct regcache *regcache)
{
  int ret, tid;
  elf_fpregset_t regs;
  struct iovec iovec;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  /* Make sure REGS can hold all VFP registers contents on both aarch64
     and arm.  */
  gdb_static_assert (sizeof regs >= VFP_REGS_SIZE);
  tid = ptid_get_lwp (inferior_ptid);

  iovec.iov_base = &regs;

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      iovec.iov_len = VFP_REGS_SIZE;

      ret = ptrace (PTRACE_GETREGSET, tid, NT_ARM_VFP, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to fetch VFP registers."));

      aarch32_vfp_regcache_collect (regcache, (gdb_byte *) &regs, 32);
    }
  else
    {
      int regno;

      iovec.iov_len = sizeof (regs);

      ret = ptrace (PTRACE_GETREGSET, tid, NT_FPREGSET, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to fetch FP/SIMD registers."));

      for (regno = AARCH64_V0_REGNUM; regno <= AARCH64_V31_REGNUM; regno++)
	if (REG_VALID == regcache_register_status (regcache, regno))
	  regcache_raw_collect (regcache, regno,
				(char *) &regs.vregs[regno - AARCH64_V0_REGNUM]);

      if (REG_VALID == regcache_register_status (regcache, AARCH64_FPSR_REGNUM))
	regcache_raw_collect (regcache, AARCH64_FPSR_REGNUM,
			      (char *) &regs.fpsr);
      if (REG_VALID == regcache_register_status (regcache, AARCH64_FPCR_REGNUM))
	regcache_raw_collect (regcache, AARCH64_FPCR_REGNUM,
			      (char *) &regs.fpcr);
    }

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      ret = ptrace (PTRACE_SETREGSET, tid, NT_ARM_VFP, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to store VFP registers."));
    }
  else
    {
      ret = ptrace (PTRACE_SETREGSET, tid, NT_FPREGSET, &iovec);
      if (ret < 0)
	perror_with_name (_("Unable to store FP/SIMD registers."));
    }
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
  regcache_collect_regset (&aarch64_linux_gregset, regcache,
			   regno, (gdb_byte *) gregsetp,
			   AARCH64_LINUX_SIZEOF_GREGSET);
}

/* Fill GDB's register array with the general-purpose register values
   in *GREGSETP.  */

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregsetp)
{
  regcache_supply_regset (&aarch64_linux_gregset, regcache, -1,
			  (const gdb_byte *) gregsetp,
			  AARCH64_LINUX_SIZEOF_GREGSET);
}

/* Fill register REGNO (if it is a floating-point register) in
   *FPREGSETP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregsetp, int regno)
{
  regcache_collect_regset (&aarch64_linux_fpregset, regcache,
			   regno, (gdb_byte *) fpregsetp,
			   AARCH64_LINUX_SIZEOF_FPREGSET);
}

/* Fill GDB's register array with the floating-point register values
   in *FPREGSETP.  */

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t *fpregsetp)
{
  regcache_supply_regset (&aarch64_linux_fpregset, regcache, -1,
			  (const gdb_byte *) fpregsetp,
			  AARCH64_LINUX_SIZEOF_FPREGSET);
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
ps_get_thread_area (struct ps_prochandle *ph,
		    lwpid_t lwpid, int idx, void **base)
{
  int is_64bit_p
    = (gdbarch_bfd_arch_info (target_gdbarch ())->bits_per_word == 64);

  return aarch64_ps_get_thread_area (ph, lwpid, idx, base, is_64bit_p);
}


static void (*super_post_startup_inferior) (struct target_ops *self,
					    ptid_t ptid);

/* Implement the "to_post_startup_inferior" target_ops method.  */

static void
aarch64_linux_child_post_startup_inferior (struct target_ops *self,
					   ptid_t ptid)
{
  aarch64_forget_process (ptid_get_pid (ptid));
  aarch64_linux_get_debug_reg_capacity (ptid_get_pid (ptid));
  super_post_startup_inferior (self, ptid);
}

extern struct target_desc *tdesc_arm_with_neon;

/* Implement the "to_read_description" target_ops method.  */

static const struct target_desc *
aarch64_linux_read_description (struct target_ops *ops)
{
  int ret, tid;
  gdb_byte regbuf[VFP_REGS_SIZE];
  struct iovec iovec;

  tid = ptid_get_lwp (inferior_ptid);

  iovec.iov_base = regbuf;
  iovec.iov_len = VFP_REGS_SIZE;

  ret = ptrace (PTRACE_GETREGSET, tid, NT_ARM_VFP, &iovec);
  if (ret == 0)
    return tdesc_arm_with_neon;
  else
    return tdesc_aarch64;
}

/* Convert a native/host siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  Returns true if any
   conversion was done; false otherwise.  If DIRECTION is 1, then copy
   from INF to NATIVE.  If DIRECTION is 0, copy from NATIVE to
   INF.  */

static int
aarch64_linux_siginfo_fixup (siginfo_t *native, gdb_byte *inf, int direction)
{
  struct gdbarch *gdbarch = get_frame_arch (get_current_frame ());

  /* Is the inferior 32-bit?  If so, then do fixup the siginfo
     object.  */
  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      if (direction == 0)
	aarch64_compat_siginfo_from_siginfo ((struct compat_siginfo *) inf,
					     native);
      else
	aarch64_siginfo_from_compat_siginfo (native,
					     (struct compat_siginfo *) inf);

      return 1;
    }

  return 0;
}

/* Returns the number of hardware watchpoints of type TYPE that we can
   set.  Value is positive if we can set CNT watchpoints, zero if
   setting watchpoints of type TYPE is not supported, and negative if
   CNT is more than the maximum number of watchpoints of type TYPE
   that we can support.  TYPE is one of bp_hardware_watchpoint,
   bp_read_watchpoint, bp_write_watchpoint, or bp_hardware_breakpoint.
   CNT is the number of such watchpoints used so far (including this
   one).  OTHERTYPE is non-zero if other types of watchpoints are
   currently enabled.  */

static int
aarch64_linux_can_use_hw_breakpoint (struct target_ops *self,
				     enum bptype type,
				     int cnt, int othertype)
{
  if (type == bp_hardware_watchpoint || type == bp_read_watchpoint
      || type == bp_access_watchpoint || type == bp_watchpoint)
    {
      if (aarch64_num_wp_regs == 0)
	return 0;
    }
  else if (type == bp_hardware_breakpoint)
    {
      if (aarch64_num_bp_regs == 0)
	return 0;
    }
  else
    gdb_assert_not_reached ("unexpected breakpoint type");

  /* We always return 1 here because we don't have enough information
     about possible overlap of addresses that they want to watch.  As an
     extreme example, consider the case where all the watchpoints watch
     the same address and the same region length: then we can handle a
     virtually unlimited number of watchpoints, due to debug register
     sharing implemented via reference counts.  */
  return 1;
}

/* Insert a hardware-assisted breakpoint at BP_TGT->reqstd_address.
   Return 0 on success, -1 on failure.  */

static int
aarch64_linux_insert_hw_breakpoint (struct target_ops *self,
				    struct gdbarch *gdbarch,
				    struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address = bp_tgt->reqstd_address;
  int len;
  const enum target_hw_bp_type type = hw_execute;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));

  gdbarch_breakpoint_from_pc (gdbarch, &addr, &len);

  if (show_debug_regs)
    fprintf_unfiltered
      (gdb_stdlog,
       "insert_hw_breakpoint on entry (addr=0x%08lx, len=%d))\n",
       (unsigned long) addr, len);

  ret = aarch64_handle_breakpoint (type, addr, len, 1 /* is_insert */, state);

  if (show_debug_regs)
    {
      aarch64_show_debug_reg_state (state,
				    "insert_hw_breakpoint", addr, len, type);
    }

  return ret;
}

/* Remove a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, -1 on failure.  */

static int
aarch64_linux_remove_hw_breakpoint (struct target_ops *self,
				    struct gdbarch *gdbarch,
				    struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address;
  int len = 4;
  const enum target_hw_bp_type type = hw_execute;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));

  gdbarch_breakpoint_from_pc (gdbarch, &addr, &len);

  if (show_debug_regs)
    fprintf_unfiltered
      (gdb_stdlog, "remove_hw_breakpoint on entry (addr=0x%08lx, len=%d))\n",
       (unsigned long) addr, len);

  ret = aarch64_handle_breakpoint (type, addr, len, 0 /* is_insert */, state);

  if (show_debug_regs)
    {
      aarch64_show_debug_reg_state (state,
				    "remove_hw_watchpoint", addr, len, type);
    }

  return ret;
}

/* Implement the "to_insert_watchpoint" target_ops method.

   Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */

static int
aarch64_linux_insert_watchpoint (struct target_ops *self,
				 CORE_ADDR addr, int len,
				 enum target_hw_bp_type type,
				 struct expression *cond)
{
  int ret;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));

  if (show_debug_regs)
    fprintf_unfiltered (gdb_stdlog,
			"insert_watchpoint on entry (addr=0x%08lx, len=%d)\n",
			(unsigned long) addr, len);

  gdb_assert (type != hw_execute);

  ret = aarch64_handle_watchpoint (type, addr, len, 1 /* is_insert */, state);

  if (show_debug_regs)
    {
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
aarch64_linux_remove_watchpoint (struct target_ops *self,
				 CORE_ADDR addr, int len,
				 enum target_hw_bp_type type,
				 struct expression *cond)
{
  int ret;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (ptid_get_pid (inferior_ptid));

  if (show_debug_regs)
    fprintf_unfiltered (gdb_stdlog,
			"remove_watchpoint on entry (addr=0x%08lx, len=%d)\n",
			(unsigned long) addr, len);

  gdb_assert (type != hw_execute);

  ret = aarch64_handle_watchpoint (type, addr, len, 0 /* is_insert */, state);

  if (show_debug_regs)
    {
      aarch64_show_debug_reg_state (state,
				    "remove_watchpoint", addr, len, type);
    }

  return ret;
}

/* Implement the "to_region_ok_for_hw_watchpoint" target_ops method.  */

static int
aarch64_linux_region_ok_for_hw_watchpoint (struct target_ops *self,
					   CORE_ADDR addr, int len)
{
  return aarch64_linux_region_ok_for_watchpoint (addr, len);
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
aarch64_linux_stopped_by_watchpoint (struct target_ops *ops)
{
  CORE_ADDR addr;

  return aarch64_linux_stopped_data_address (ops, &addr);
}

/* Implement the "to_watchpoint_addr_within_range" target_ops method.  */

static int
aarch64_linux_watchpoint_addr_within_range (struct target_ops *target,
					    CORE_ADDR addr,
					    CORE_ADDR start, int length)
{
  return start <= addr && start + length - 1 >= addr;
}

/* Implement the "to_can_do_single_step" target_ops method.  */

static int
aarch64_linux_can_do_single_step (struct target_ops *target)
{
  return 1;
}

/* Define AArch64 maintenance commands.  */

static void
add_show_debug_regs_command (void)
{
  /* A maintenance command to enable printing the internal DRi mirror
     variables.  */
  add_setshow_boolean_cmd ("show-debug-regs", class_maintenance,
			   &show_debug_regs, _("\
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
  t->to_can_do_single_step = aarch64_linux_can_do_single_step;

  /* Override the GNU/Linux inferior startup hook.  */
  super_post_startup_inferior = t->to_post_startup_inferior;
  t->to_post_startup_inferior = aarch64_linux_child_post_startup_inferior;

  /* Register the target.  */
  linux_nat_add_target (t);
  linux_nat_set_new_thread (t, aarch64_linux_new_thread);
  linux_nat_set_new_fork (t, aarch64_linux_new_fork);
  linux_nat_set_forget_process (t, aarch64_forget_process);
  linux_nat_set_prepare_to_resume (t, aarch64_linux_prepare_to_resume);

  /* Add our siginfo layout converter.  */
  linux_nat_set_siginfo_fixup (t, aarch64_linux_siginfo_fixup);
}
