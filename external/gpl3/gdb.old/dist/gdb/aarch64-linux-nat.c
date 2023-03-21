/* Native-dependent code for GNU/Linux AArch64.

   Copyright (C) 2011-2020 Free Software Foundation, Inc.
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
#include "aarch32-tdep.h"
#include "arch/arm.h"
#include "nat/aarch64-linux.h"
#include "nat/aarch64-linux-hw-point.h"
#include "nat/aarch64-sve-linux-ptrace.h"

#include "elf/external.h"
#include "elf/common.h"

#include "nat/gdb_ptrace.h"
#include <sys/utsname.h>
#include <asm/ptrace.h>

#include "gregset.h"
#include "linux-tdep.h"

/* Defines ps_err_e, struct ps_prochandle.  */
#include "gdb_proc_service.h"
#include "arch-utils.h"

#ifndef TRAP_HWBKPT
#define TRAP_HWBKPT 0x0004
#endif

class aarch64_linux_nat_target final : public linux_nat_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  const struct target_desc *read_description () override;

  /* Add our hardware breakpoint and watchpoint implementation.  */
  int can_use_hw_breakpoint (enum bptype, int, int) override;
  int insert_hw_breakpoint (struct gdbarch *, struct bp_target_info *) override;
  int remove_hw_breakpoint (struct gdbarch *, struct bp_target_info *) override;
  int region_ok_for_hw_watchpoint (CORE_ADDR, int) override;
  int insert_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;
  int remove_watchpoint (CORE_ADDR, int, enum target_hw_bp_type,
			 struct expression *) override;
  bool stopped_by_watchpoint () override;
  bool stopped_data_address (CORE_ADDR *) override;
  bool watchpoint_addr_within_range (CORE_ADDR, CORE_ADDR, int) override;

  int can_do_single_step () override;

  /* Override the GNU/Linux inferior startup hook.  */
  void post_startup_inferior (ptid_t) override;

  /* Override the GNU/Linux post attach hook.  */
  void post_attach (int pid) override;

  /* These three defer to common nat/ code.  */
  void low_new_thread (struct lwp_info *lp) override
  { aarch64_linux_new_thread (lp); }
  void low_delete_thread (struct arch_lwp_info *lp) override
  { aarch64_linux_delete_thread (lp); }
  void low_prepare_to_resume (struct lwp_info *lp) override
  { aarch64_linux_prepare_to_resume (lp); }

  void low_new_fork (struct lwp_info *parent, pid_t child_pid) override;
  void low_forget_process (pid_t pid) override;

  /* Add our siginfo layout converter.  */
  bool low_siginfo_fixup (siginfo_t *ptrace, gdb_byte *inf, int direction)
    override;

  struct gdbarch *thread_architecture (ptid_t) override;
};

static aarch64_linux_nat_target the_aarch64_linux_nat_target;

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

void
aarch64_linux_nat_target::low_forget_process (pid_t pid)
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
  struct gdbarch *gdbarch = regcache->arch ();
  elf_gregset_t regs;
  struct iovec iovec;

  /* Make sure REGS can hold all registers contents on both aarch64
     and arm.  */
  gdb_static_assert (sizeof (regs) >= 18 * 4);

  tid = regcache->ptid ().lwp ();

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
	regcache->raw_supply (regno, &regs[regno - AARCH64_X0_REGNUM]);
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
  struct gdbarch *gdbarch = regcache->arch ();

  /* Make sure REGS can hold all registers contents on both aarch64
     and arm.  */
  gdb_static_assert (sizeof (regs) >= 18 * 4);
  tid = regcache->ptid ().lwp ();

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
	if (REG_VALID == regcache->get_register_status (regno))
	  regcache->raw_collect (regno, &regs[regno - AARCH64_X0_REGNUM]);
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
  struct gdbarch *gdbarch = regcache->arch ();

  /* Make sure REGS can hold all VFP registers contents on both aarch64
     and arm.  */
  gdb_static_assert (sizeof regs >= ARM_VFP3_REGS_SIZE);

  tid = regcache->ptid ().lwp ();

  iovec.iov_base = &regs;

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      iovec.iov_len = ARM_VFP3_REGS_SIZE;

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
	regcache->raw_supply (regno, &regs.vregs[regno - AARCH64_V0_REGNUM]);

      regcache->raw_supply (AARCH64_FPSR_REGNUM, &regs.fpsr);
      regcache->raw_supply (AARCH64_FPCR_REGNUM, &regs.fpcr);
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
  struct gdbarch *gdbarch = regcache->arch ();

  /* Make sure REGS can hold all VFP registers contents on both aarch64
     and arm.  */
  gdb_static_assert (sizeof regs >= ARM_VFP3_REGS_SIZE);
  tid = regcache->ptid ().lwp ();

  iovec.iov_base = &regs;

  if (gdbarch_bfd_arch_info (gdbarch)->bits_per_word == 32)
    {
      iovec.iov_len = ARM_VFP3_REGS_SIZE;

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
	if (REG_VALID == regcache->get_register_status (regno))
	  regcache->raw_collect
	    (regno, (char *) &regs.vregs[regno - AARCH64_V0_REGNUM]);

      if (REG_VALID == regcache->get_register_status (AARCH64_FPSR_REGNUM))
	regcache->raw_collect (AARCH64_FPSR_REGNUM, (char *) &regs.fpsr);
      if (REG_VALID == regcache->get_register_status (AARCH64_FPCR_REGNUM))
	regcache->raw_collect (AARCH64_FPCR_REGNUM, (char *) &regs.fpcr);
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

/* Fill GDB's register array with the sve register values
   from the current thread.  */

static void
fetch_sveregs_from_thread (struct regcache *regcache)
{
  std::unique_ptr<gdb_byte[]> base
    = aarch64_sve_get_sveregs (regcache->ptid ().lwp ());
  aarch64_sve_regs_copy_to_reg_buf (regcache, base.get ());
}

/* Store to the current thread the valid sve register
   values in the GDB's register array.  */

static void
store_sveregs_to_thread (struct regcache *regcache)
{
  int ret;
  struct iovec iovec;
  int tid = regcache->ptid ().lwp ();

  /* First store vector length to the thread.  This is done first to ensure the
     ptrace buffers read from the kernel are the correct size.  */
  if (!aarch64_sve_set_vq (tid, regcache))
    perror_with_name (_("Unable to set VG register."));

  /* Obtain a dump of SVE registers from ptrace.  */
  std::unique_ptr<gdb_byte[]> base = aarch64_sve_get_sveregs (tid);

  /* Overwrite with regcache state.  */
  aarch64_sve_regs_copy_from_reg_buf (regcache, base.get ());

  /* Write back to the kernel.  */
  iovec.iov_base = base.get ();
  iovec.iov_len = ((struct user_sve_header *) base.get ())->size;
  ret = ptrace (PTRACE_SETREGSET, tid, NT_ARM_SVE, &iovec);

  if (ret < 0)
    perror_with_name (_("Unable to store sve registers"));
}

/* Fill GDB's register array with the pointer authentication mask values from
   the current thread.  */

static void
fetch_pauth_masks_from_thread (struct regcache *regcache)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (regcache->arch ());
  int ret;
  struct iovec iovec;
  uint64_t pauth_regset[2] = {0, 0};
  int tid = regcache->ptid ().lwp ();

  iovec.iov_base = &pauth_regset;
  iovec.iov_len = sizeof (pauth_regset);

  ret = ptrace (PTRACE_GETREGSET, tid, NT_ARM_PAC_MASK, &iovec);
  if (ret != 0)
    perror_with_name (_("unable to fetch pauth registers."));

  regcache->raw_supply (AARCH64_PAUTH_DMASK_REGNUM (tdep->pauth_reg_base),
			&pauth_regset[0]);
  regcache->raw_supply (AARCH64_PAUTH_CMASK_REGNUM (tdep->pauth_reg_base),
			&pauth_regset[1]);
}

/* Implement the "fetch_registers" target_ops method.  */

void
aarch64_linux_nat_target::fetch_registers (struct regcache *regcache,
					   int regno)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (regcache->arch ());

  if (regno == -1)
    {
      fetch_gregs_from_thread (regcache);
      if (tdep->has_sve ())
	fetch_sveregs_from_thread (regcache);
      else
	fetch_fpregs_from_thread (regcache);

      if (tdep->has_pauth ())
	fetch_pauth_masks_from_thread (regcache);
    }
  else if (regno < AARCH64_V0_REGNUM)
    fetch_gregs_from_thread (regcache);
  else if (tdep->has_sve ())
    fetch_sveregs_from_thread (regcache);
  else
    fetch_fpregs_from_thread (regcache);

  if (tdep->has_pauth ())
    {
      if (regno == AARCH64_PAUTH_DMASK_REGNUM (tdep->pauth_reg_base)
	  || regno == AARCH64_PAUTH_CMASK_REGNUM (tdep->pauth_reg_base))
	fetch_pauth_masks_from_thread (regcache);
    }
}

/* Implement the "store_registers" target_ops method.  */

void
aarch64_linux_nat_target::store_registers (struct regcache *regcache,
					   int regno)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (regcache->arch ());

  if (regno == -1)
    {
      store_gregs_to_thread (regcache);
      if (tdep->has_sve ())
	store_sveregs_to_thread (regcache);
      else
	store_fpregs_to_thread (regcache);
    }
  else if (regno < AARCH64_V0_REGNUM)
    store_gregs_to_thread (regcache);
  else if (tdep->has_sve ())
    store_sveregs_to_thread (regcache);
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

void
aarch64_linux_nat_target::low_new_fork (struct lwp_info *parent,
					pid_t child_pid)
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

  parent_pid = parent->ptid.pid ();
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


/* Implement the "post_startup_inferior" target_ops method.  */

void
aarch64_linux_nat_target::post_startup_inferior (ptid_t ptid)
{
  low_forget_process (ptid.pid ());
  aarch64_linux_get_debug_reg_capacity (ptid.pid ());
  linux_nat_target::post_startup_inferior (ptid);
}

/* Implement the "post_attach" target_ops method.  */

void
aarch64_linux_nat_target::post_attach (int pid)
{
  low_forget_process (pid);
  /* Set the hardware debug register capacity.  If
     aarch64_linux_get_debug_reg_capacity is not called
     (as it is in aarch64_linux_child_post_startup_inferior) then
     software watchpoints will be used instead of hardware
     watchpoints when attaching to a target.  */
  aarch64_linux_get_debug_reg_capacity (pid);
  linux_nat_target::post_attach (pid);
}

/* Implement the "read_description" target_ops method.  */

const struct target_desc *
aarch64_linux_nat_target::read_description ()
{
  int ret, tid;
  gdb_byte regbuf[ARM_VFP3_REGS_SIZE];
  struct iovec iovec;

  tid = inferior_ptid.lwp ();

  iovec.iov_base = regbuf;
  iovec.iov_len = ARM_VFP3_REGS_SIZE;

  ret = ptrace (PTRACE_GETREGSET, tid, NT_ARM_VFP, &iovec);
  if (ret == 0)
    return aarch32_read_description ();

  CORE_ADDR hwcap = linux_get_hwcap (this);

  return aarch64_read_description (aarch64_sve_get_vq (tid),
				   hwcap & AARCH64_HWCAP_PACA);
}

/* Convert a native/host siginfo object, into/from the siginfo in the
   layout of the inferiors' architecture.  Returns true if any
   conversion was done; false otherwise.  If DIRECTION is 1, then copy
   from INF to NATIVE.  If DIRECTION is 0, copy from NATIVE to
   INF.  */

bool
aarch64_linux_nat_target::low_siginfo_fixup (siginfo_t *native, gdb_byte *inf,
					     int direction)
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

      return true;
    }

  return false;
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

int
aarch64_linux_nat_target::can_use_hw_breakpoint (enum bptype type,
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

int
aarch64_linux_nat_target::insert_hw_breakpoint (struct gdbarch *gdbarch,
						struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address = bp_tgt->reqstd_address;
  int len;
  const enum target_hw_bp_type type = hw_execute;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

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

int
aarch64_linux_nat_target::remove_hw_breakpoint (struct gdbarch *gdbarch,
						struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address;
  int len = 4;
  const enum target_hw_bp_type type = hw_execute;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

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

/* Implement the "insert_watchpoint" target_ops method.

   Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */

int
aarch64_linux_nat_target::insert_watchpoint (CORE_ADDR addr, int len,
					     enum target_hw_bp_type type,
					     struct expression *cond)
{
  int ret;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

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

/* Implement the "remove_watchpoint" target_ops method.
   Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE.  Return 0 on success, -1 on failure.  */

int
aarch64_linux_nat_target::remove_watchpoint (CORE_ADDR addr, int len,
					     enum target_hw_bp_type type,
					     struct expression *cond)
{
  int ret;
  struct aarch64_debug_reg_state *state
    = aarch64_get_debug_reg_state (inferior_ptid.pid ());

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

/* Implement the "region_ok_for_hw_watchpoint" target_ops method.  */

int
aarch64_linux_nat_target::region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  return aarch64_linux_region_ok_for_watchpoint (addr, len);
}

/* Implement the "stopped_data_address" target_ops method.  */

bool
aarch64_linux_nat_target::stopped_data_address (CORE_ADDR *addr_p)
{
  siginfo_t siginfo;
  int i;
  struct aarch64_debug_reg_state *state;

  if (!linux_nat_get_siginfo (inferior_ptid, &siginfo))
    return false;

  /* This must be a hardware breakpoint.  */
  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != TRAP_HWBKPT)
    return false;

  /* Check if the address matches any watched address.  */
  state = aarch64_get_debug_reg_state (inferior_ptid.pid ());
  for (i = aarch64_num_wp_regs - 1; i >= 0; --i)
    {
      const unsigned int offset
	= aarch64_watchpoint_offset (state->dr_ctrl_wp[i]);
      const unsigned int len = aarch64_watchpoint_length (state->dr_ctrl_wp[i]);
      const CORE_ADDR addr_trap = (CORE_ADDR) siginfo.si_addr;
      const CORE_ADDR addr_watch = state->dr_addr_wp[i] + offset;
      const CORE_ADDR addr_watch_aligned = align_down (state->dr_addr_wp[i], 8);
      const CORE_ADDR addr_orig = state->dr_addr_orig_wp[i];

      if (state->dr_ref_count_wp[i]
	  && DR_CONTROL_ENABLED (state->dr_ctrl_wp[i])
	  && addr_trap >= addr_watch_aligned
	  && addr_trap < addr_watch + len)
	{
	  /* ADDR_TRAP reports the first address of the memory range
	     accessed by the CPU, regardless of what was the memory
	     range watched.  Thus, a large CPU access that straddles
	     the ADDR_WATCH..ADDR_WATCH+LEN range may result in an
	     ADDR_TRAP that is lower than the
	     ADDR_WATCH..ADDR_WATCH+LEN range.  E.g.:

	     addr: |   4   |   5   |   6   |   7   |   8   |
				   |---- range watched ----|
		   |----------- range accessed ------------|

	     In this case, ADDR_TRAP will be 4.

	     To match a watchpoint known to GDB core, we must never
	     report *ADDR_P outside of any ADDR_WATCH..ADDR_WATCH+LEN
	     range.  ADDR_WATCH <= ADDR_TRAP < ADDR_ORIG is a false
	     positive on kernels older than 4.10.  See PR
	     external/20207.  */
	  *addr_p = addr_orig;
	  return true;
	}
    }

  return false;
}

/* Implement the "stopped_by_watchpoint" target_ops method.  */

bool
aarch64_linux_nat_target::stopped_by_watchpoint ()
{
  CORE_ADDR addr;

  return stopped_data_address (&addr);
}

/* Implement the "watchpoint_addr_within_range" target_ops method.  */

bool
aarch64_linux_nat_target::watchpoint_addr_within_range (CORE_ADDR addr,
							CORE_ADDR start, int length)
{
  return start <= addr && start + length - 1 >= addr;
}

/* Implement the "can_do_single_step" target_ops method.  */

int
aarch64_linux_nat_target::can_do_single_step ()
{
  return 1;
}

/* Implement the "thread_architecture" target_ops method.  */

struct gdbarch *
aarch64_linux_nat_target::thread_architecture (ptid_t ptid)
{
  /* Return the gdbarch for the current thread.  If the vector length has
     changed since the last time this was called, then do a further lookup.  */

  uint64_t vq = aarch64_sve_get_vq (ptid.lwp ());

  /* Find the current gdbarch the same way as process_stratum_target.  Only
     return it if the current vector length matches the one in the tdep.  */
  inferior *inf = find_inferior_ptid (this, ptid);
  gdb_assert (inf != NULL);
  if (vq == gdbarch_tdep (inf->gdbarch)->vq)
    return inf->gdbarch;

  /* We reach here if the vector length for the thread is different from its
     value at process start.  Lookup gdbarch via info (potentially creating a
     new one), stashing the vector length inside id.  Use -1 for when SVE
     unavailable, to distinguish from an unset value of 0.  */
  struct gdbarch_info info;
  gdbarch_info_init (&info);
  info.bfd_arch_info = bfd_lookup_arch (bfd_arch_aarch64, bfd_mach_aarch64);
  info.id = (int *) (vq == 0 ? -1 : vq);
  return gdbarch_find_by_info (info);
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

void _initialize_aarch64_linux_nat ();
void
_initialize_aarch64_linux_nat ()
{
  add_show_debug_regs_command ();

  /* Register the target.  */
  linux_target = &the_aarch64_linux_nat_target;
  add_inf_child_target (&the_aarch64_linux_nat_target);
}
