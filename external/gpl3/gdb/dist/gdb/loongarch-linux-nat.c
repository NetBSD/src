/* Native-dependent code for GNU/Linux on LoongArch processors.

   Copyright (C) 2022-2025 Free Software Foundation, Inc.
   Contributed by Loongson Ltd.

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

#include "cli/cli-cmds.h"
#include "elf/common.h"
#include "gregset.h"
#include "inferior.h"
#include "linux-nat-trad.h"
#include "loongarch-tdep.h"
#include "nat/gdb_ptrace.h"
#include "nat/loongarch-hw-point.h"
#include "nat/loongarch-linux.h"
#include "nat/loongarch-linux-hw-point.h"
#include "target-descriptions.h"

#include <asm/ptrace.h>

/* Hash table storing per-process data.  We don't bind this to a
   per-inferior registry because of targets like x86 GNU/Linux that
   need to keep track of processes that aren't bound to any inferior
   (e.g., fork children, checkpoints).  */

static std::unordered_map<pid_t, loongarch_debug_reg_state>
loongarch_debug_process_state;

/* See nat/loongarch-linux-hw-point.h.  */

struct loongarch_debug_reg_state *
loongarch_get_debug_reg_state (pid_t pid)
{
  return &loongarch_debug_process_state[pid];
}

/* Remove any existing per-process debug state for process PID.  */

static void
loongarch_remove_debug_reg_state (pid_t pid)
{
  loongarch_debug_process_state.erase (pid);
}

/* LoongArch Linux native additions to the default Linux support.  */

class loongarch_linux_nat_target final : public linux_nat_trad_target
{
public:
  /* Add our register access methods.  */
  void fetch_registers (struct regcache *, int) override;
  void store_registers (struct regcache *, int) override;

  int can_use_hw_breakpoint (enum bptype type, int cnt, int othertype) override;
  int region_ok_for_hw_watchpoint (CORE_ADDR addr, int len) override;

  int insert_watchpoint (CORE_ADDR addr, int len, enum target_hw_bp_type type,
			 struct expression *cond) override;
  int remove_watchpoint (CORE_ADDR addr, int len, enum target_hw_bp_type type,
			 struct expression *cond) override;

  /* Add our hardware breakpoint and watchpoint implementation.  */
  bool stopped_by_watchpoint () override;
  bool stopped_data_address (CORE_ADDR *) override;

  int insert_hw_breakpoint (struct gdbarch *gdbarch,
			    struct bp_target_info *bp_tgt) override;
  int remove_hw_breakpoint (struct gdbarch *gdbarch,
			    struct bp_target_info *bp_tgt) override;

  /* Override the GNU/Linux inferior startup hook.  */
  void post_startup_inferior (ptid_t) override;

  /* Override the GNU/Linux post attach hook.  */
  void post_attach (int pid) override;

  /* These three defer to common nat/ code.  */
  void low_new_thread (struct lwp_info *lp) override
  { loongarch_linux_new_thread (lp); }
  void low_delete_thread (struct arch_lwp_info *lp) override
  { loongarch_linux_delete_thread (lp); }
  void low_prepare_to_resume (struct lwp_info *lp) override
  { loongarch_linux_prepare_to_resume (lp); }

  void low_new_fork (struct lwp_info *parent, pid_t child_pid) override;
  void low_forget_process (pid_t pid) override;

protected:
  /* Override linux_nat_trad_target methods.  */
  CORE_ADDR register_u_offset (struct gdbarch *gdbarch, int regnum,
			       int store_p) override;
};

/* Fill GDB's register array with the general-purpose, orig_a0, pc and badv
   register values from the current thread.  */

static void
fetch_gregs_from_thread (struct regcache *regcache, int regnum, pid_t tid)
{
  elf_gregset_t regset;

  if (regnum == -1 || (regnum >= 0 && regnum < 32)
      || regnum == LOONGARCH_ORIG_A0_REGNUM
      || regnum == LOONGARCH_PC_REGNUM
      || regnum == LOONGARCH_BADV_REGNUM)
  {
    struct iovec iov;

    iov.iov_base = &regset;
    iov.iov_len = sizeof (regset);

    if (ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, (long) &iov) < 0)
      perror_with_name (_("Couldn't get NT_PRSTATUS registers"));
    else
      loongarch_gregset.supply_regset (nullptr, regcache, -1,
				       &regset, sizeof (regset));
  }
}

/* Store to the current thread the valid general-purpose, orig_a0, pc and badv
   register values in the GDB's register array.  */

static void
store_gregs_to_thread (struct regcache *regcache, int regnum, pid_t tid)
{
  elf_gregset_t regset;

  if (regnum == -1 || (regnum >= 0 && regnum < 32)
      || regnum == LOONGARCH_ORIG_A0_REGNUM
      || regnum == LOONGARCH_PC_REGNUM
      || regnum == LOONGARCH_BADV_REGNUM)
  {
    struct iovec iov;

    iov.iov_base = &regset;
    iov.iov_len = sizeof (regset);

    if (ptrace (PTRACE_GETREGSET, tid, NT_PRSTATUS, (long) &iov) < 0)
      perror_with_name (_("Couldn't get NT_PRSTATUS registers"));
    else
      {
	loongarch_gregset.collect_regset (nullptr, regcache, regnum,
					  &regset, sizeof (regset));
	if (ptrace (PTRACE_SETREGSET, tid, NT_PRSTATUS, (long) &iov) < 0)
	  perror_with_name (_("Couldn't set NT_PRSTATUS registers"));
      }
  }
}

/* Fill GDB's register array with the fp, fcc and fcsr
   register values from the current thread.  */

static void
fetch_fpregs_from_thread (struct regcache *regcache, int regnum, pid_t tid)
{
  elf_fpregset_t regset;

  if ((regnum == -1)
      || (regnum >= LOONGARCH_FIRST_FP_REGNUM && regnum <= LOONGARCH_FCSR_REGNUM))
    {
      struct iovec iovec = { .iov_base = &regset, .iov_len = sizeof (regset) };

      if (ptrace (PTRACE_GETREGSET, tid, NT_FPREGSET, (long) &iovec) < 0)
	perror_with_name (_("Couldn't get NT_FPREGSET registers"));
      else
	loongarch_fpregset.supply_regset (nullptr, regcache, -1,
					  &regset, sizeof (regset));
    }
}

/* Store to the current thread the valid fp, fcc and fcsr
   register values in the GDB's register array.  */

static void
store_fpregs_to_thread (struct regcache *regcache, int regnum, pid_t tid)
{
  elf_fpregset_t regset;

  if ((regnum == -1)
      || (regnum >= LOONGARCH_FIRST_FP_REGNUM && regnum <= LOONGARCH_FCSR_REGNUM))
    {
      struct iovec iovec = { .iov_base = &regset, .iov_len = sizeof (regset) };

      if (ptrace (PTRACE_GETREGSET, tid, NT_FPREGSET, (long) &iovec) < 0)
	perror_with_name (_("Couldn't get NT_FPREGSET registers"));
      else
	{
	  loongarch_fpregset.collect_regset (nullptr, regcache, regnum,
					     &regset, sizeof (regset));
	  if (ptrace (PTRACE_SETREGSET, tid, NT_FPREGSET, (long) &iovec) < 0)
	    perror_with_name (_("Couldn't set NT_FPREGSET registers"));
	}
    }
}

/* Fill GDB's register array with the Loongson SIMD Extension
   register values from the current thread.  */

static void
fetch_lsxregs_from_thread (struct regcache *regcache, int regnum, pid_t tid)
{
  elf_lsxregset_t regset;

  if ((regnum == -1)
      || (regnum >= LOONGARCH_FIRST_LSX_REGNUM && regnum < LOONGARCH_FIRST_LASX_REGNUM))
    {
      struct iovec iovec = { .iov_base = &regset, .iov_len = sizeof (regset) };

      if (ptrace (PTRACE_GETREGSET, tid, NT_LARCH_LSX, (long) &iovec) < 0)
	{
	  /* If kernel dose not support lsx, just return. */
	  if (errno == EINVAL)
	    return;

	  perror_with_name (_("Couldn't get NT_LARCH_LSX registers"));
	}
      else
	loongarch_lsxregset.supply_regset (nullptr, regcache, -1,
					   &regset, sizeof (regset));
    }
}

/* Store to the current thread the valid Loongson SIMD Extension
   register values in the GDB's register array.  */

static void
store_lsxregs_to_thread (struct regcache *regcache, int regnum, pid_t tid)
{
  elf_lsxregset_t regset;

  if ((regnum == -1)
      || (regnum >= LOONGARCH_FIRST_LSX_REGNUM && regnum < LOONGARCH_FIRST_LASX_REGNUM))
    {
      struct iovec iovec = { .iov_base = &regset, .iov_len = sizeof (regset) };

      if (ptrace (PTRACE_GETREGSET, tid, NT_LARCH_LSX, (long) &iovec) < 0)
	{
	  /* If kernel dose not support lsx, just return. */
	  if (errno == EINVAL)
	    return;

	  perror_with_name (_("Couldn't get NT_LARCH_LSX registers"));
	}
      else
	{
	  loongarch_lsxregset.collect_regset (nullptr, regcache, regnum,
					     &regset, sizeof (regset));
	  if (ptrace (PTRACE_SETREGSET, tid, NT_LARCH_LSX, (long) &iovec) < 0)
	    perror_with_name (_("Couldn't set NT_LARCH_LSX registers"));
	}
    }
}

/* Fill GDB's register array with the Loongson Advanced SIMD Extension
   register values from the current thread.  */

static void
fetch_lasxregs_from_thread (struct regcache *regcache, int regnum, pid_t tid)
{
  elf_lasxregset_t regset;

  if ((regnum == -1)
      || (regnum >= LOONGARCH_FIRST_LASX_REGNUM
	  && regnum < LOONGARCH_FIRST_LASX_REGNUM + LOONGARCH_LINUX_NUM_LASXREGSET))
    {
      struct iovec iovec = { .iov_base = &regset, .iov_len = sizeof (regset) };

      if (ptrace (PTRACE_GETREGSET, tid, NT_LARCH_LASX, (long) &iovec) < 0)
	{
	  /* If kernel dose not support lasx, just return. */
	  if (errno == EINVAL)
	    return;

	  perror_with_name (_("Couldn't get NT_LARCH_LSX registers"));
	}
      else
	loongarch_lasxregset.supply_regset (nullptr, regcache, -1,
					    &regset, sizeof (regset));
    }
}

/* Store to the current thread the valid Loongson Advanced SIMD Extension
   register values in the GDB's register array.  */

static void
store_lasxregs_to_thread (struct regcache *regcache, int regnum, pid_t tid)
{
  elf_lasxregset_t regset;

  if ((regnum == -1)
      || (regnum >= LOONGARCH_FIRST_LASX_REGNUM
	  && regnum < LOONGARCH_FIRST_LASX_REGNUM + LOONGARCH_LINUX_NUM_LASXREGSET))
    {
      struct iovec iovec = { .iov_base = &regset, .iov_len = sizeof (regset) };

      if (ptrace (PTRACE_GETREGSET, tid, NT_LARCH_LASX, (long) &iovec) < 0)
	{
	  /* If kernel dose not support lasx, just return. */
	  if (errno == EINVAL)
	    return;

	  perror_with_name (_("Couldn't get NT_LARCH_LSX registers"));
	}
      else
	{
	  loongarch_lasxregset.collect_regset (nullptr, regcache, regnum,
					      &regset, sizeof (regset));
	  if (ptrace (PTRACE_SETREGSET, tid, NT_LARCH_LASX, (long) &iovec) < 0)
	    perror_with_name (_("Couldn't set NT_LARCH_LASX registers"));
	}
    }
}


/* Fill GDB's register array with the lbt register values
   from the current thread.  */

static void
fetch_lbt_from_thread (struct regcache *regcache, int regnum, pid_t tid)
{
  gdb_byte regset[LOONGARCH_LBT_REGS_SIZE];

  if (regnum == -1
      || (regnum >= LOONGARCH_FIRST_SCR_REGNUM
	  && regnum <= LOONGARCH_FTOP_REGNUM))
    {
      struct iovec iov;

      iov.iov_base = regset;
      iov.iov_len = LOONGARCH_LBT_REGS_SIZE;

      if (ptrace (PTRACE_GETREGSET, tid, NT_LARCH_LBT, (long) &iov) < 0)
	{
	  /* If kernel dose not support lbt, just return. */
	  if (errno == EINVAL)
	    return;
	  perror_with_name (_("Couldn't get NT_LARCH_LBT registers"));
	}
      else
	loongarch_lbtregset.supply_regset (nullptr, regcache, -1,
					   regset, LOONGARCH_LBT_REGS_SIZE);
    }
}

/* Store to the current thread the valid lbt register values
   in the GDB's register array.  */

static void
store_lbt_to_thread (struct regcache *regcache, int regnum, pid_t tid)
{
  gdb_byte regset[LOONGARCH_LBT_REGS_SIZE];

  if (regnum == -1
      || (regnum >= LOONGARCH_FIRST_SCR_REGNUM
	  && regnum <= LOONGARCH_FTOP_REGNUM))
    {
      struct iovec iov;

      iov.iov_base = regset;
      iov.iov_len = LOONGARCH_LBT_REGS_SIZE;

      if (ptrace (PTRACE_GETREGSET, tid, NT_LARCH_LBT, (long) &iov) < 0)
	{
	  /* If kernel dose not support lbt, just return. */
	  if (errno == EINVAL)
	    return;
	  perror_with_name (_("Couldn't get NT_LARCH_LBT registers"));
	}
      else
	{
	  loongarch_lbtregset.collect_regset (nullptr, regcache, regnum,
					    regset, LOONGARCH_LBT_REGS_SIZE);
	  if (ptrace (PTRACE_SETREGSET, tid, NT_LARCH_LBT, (long) &iov) < 0)
	    perror_with_name (_("Couldn't set NT_LARCH_LBT registers"));
	}
    }
}

/* Implement the "fetch_registers" target_ops method.  */

void
loongarch_linux_nat_target::fetch_registers (struct regcache *regcache,
					     int regnum)
{
  pid_t tid = get_ptrace_pid (regcache->ptid ());

  fetch_gregs_from_thread(regcache, regnum, tid);
  fetch_fpregs_from_thread(regcache, regnum, tid);
  fetch_lsxregs_from_thread(regcache, regnum, tid);
  fetch_lasxregs_from_thread(regcache, regnum, tid);
  fetch_lbt_from_thread (regcache, regnum, tid);
}

/* Implement the "store_registers" target_ops method.  */

void
loongarch_linux_nat_target::store_registers (struct regcache *regcache,
					     int regnum)
{
  pid_t tid = get_ptrace_pid (regcache->ptid ());

  store_gregs_to_thread (regcache, regnum, tid);
  store_fpregs_to_thread(regcache, regnum, tid);
  store_lsxregs_to_thread(regcache, regnum, tid);
  store_lasxregs_to_thread(regcache, regnum, tid);
  store_lbt_to_thread (regcache, regnum, tid);
}

/* Return the address in the core dump or inferior of register REGNO.  */

CORE_ADDR
loongarch_linux_nat_target::register_u_offset (struct gdbarch *gdbarch,
					       int regnum, int store_p)
{
  if (regnum >= 0 && regnum < 32)
    return regnum;
  else if (regnum == LOONGARCH_PC_REGNUM)
    return LOONGARCH_PC_REGNUM;
  else
    return -1;
}

static loongarch_linux_nat_target the_loongarch_linux_nat_target;

/* Wrapper functions.  These are only used by libthread_db.  */

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregset)
{
  loongarch_gregset.supply_regset (nullptr, regcache, -1, gregset,
				   sizeof (gdb_gregset_t));
}

void
fill_gregset (const struct regcache *regcache, gdb_gregset_t *gregset,
	      int regnum)
{
  loongarch_gregset.collect_regset (nullptr, regcache, regnum, gregset,
				    sizeof (gdb_gregset_t));
}

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t *fpregset)
{
  loongarch_fpregset.supply_regset (nullptr, regcache, -1, fpregset,
				    sizeof (gdb_fpregset_t));
}

void
fill_fpregset (const struct regcache *regcache, gdb_fpregset_t *fpregset,
	       int regnum)
{
  loongarch_fpregset.collect_regset (nullptr, regcache, regnum, fpregset,
				     sizeof (gdb_fpregset_t));
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
loongarch_linux_nat_target::can_use_hw_breakpoint (enum bptype type, int cnt,
						   int othertype)
{
  if (type == bp_hardware_watchpoint || type == bp_read_watchpoint
      || type == bp_access_watchpoint || type == bp_watchpoint)
    {
      if (loongarch_num_wp_regs == 0)
	return 0;
    }
  else if (type == bp_hardware_breakpoint)
    {
      if (loongarch_num_bp_regs == 0)
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

int
loongarch_linux_nat_target::region_ok_for_hw_watchpoint (CORE_ADDR addr,
							int len)
{
  return loongarch_region_ok_for_watchpoint (addr, len);
}

/* Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */

int
loongarch_linux_nat_target::insert_watchpoint (CORE_ADDR addr, int len,
					      enum target_hw_bp_type type,
					      struct expression *cond)
{
  int ret;
  struct loongarch_debug_reg_state *state
	= loongarch_get_debug_reg_state (inferior_ptid.pid ());

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"insert_watchpoint on entry (addr=0x%08lx, len=%d)\n",
		(unsigned long) addr, len);

  gdb_assert (type != hw_execute);

  ret = loongarch_handle_watchpoint (type, addr, len, 1 /* is_insert */,
				     inferior_ptid, state);

  if (show_debug_regs)
    {
      loongarch_show_debug_reg_state (state,
				      "insert_watchpoint", addr, len, type);
    }

  return ret;

}

/* Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE.  Return 0 on success, -1 on failure.  */

int
loongarch_linux_nat_target::remove_watchpoint (CORE_ADDR addr, int len,
					      enum target_hw_bp_type type,
					      struct expression *cond)
{
  int ret;
  struct loongarch_debug_reg_state *state
	= loongarch_get_debug_reg_state (inferior_ptid.pid ());

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"remove_watchpoint on entry (addr=0x%08lx, len=%d)\n",
		(unsigned long) addr, len);

  gdb_assert (type != hw_execute);

  ret = loongarch_handle_watchpoint (type, addr, len, 0 /* is_insert */,
				     inferior_ptid, state);

  if (show_debug_regs)
    {
      loongarch_show_debug_reg_state (state,
				      "remove_watchpoint", addr, len, type);
    }

  return ret;

}

/* Implement the "stopped_data_address" target_ops method.  */

bool
loongarch_linux_nat_target::stopped_data_address (CORE_ADDR *addr_p)
{
  siginfo_t siginfo;
  struct loongarch_debug_reg_state *state;

  if (!linux_nat_get_siginfo (inferior_ptid, &siginfo))
    return false;

  /* This must be a hardware breakpoint.  */
  if (siginfo.si_signo != SIGTRAP || (siginfo.si_code & 0xffff) != TRAP_HWBKPT)
    return false;

  /* Check if the address matches any watched address.  */
  state = loongarch_get_debug_reg_state (inferior_ptid.pid ());

  return
    loongarch_stopped_data_address (state, (CORE_ADDR) siginfo.si_addr, addr_p);
}

/* Implement the "stopped_by_watchpoint" target_ops method.  */

bool
loongarch_linux_nat_target::stopped_by_watchpoint ()
{
  CORE_ADDR addr;

  return stopped_data_address (&addr);
}

/* Insert a hardware-assisted breakpoint at BP_TGT->reqstd_address.
   Return 0 on success, -1 on failure.  */

int
loongarch_linux_nat_target::insert_hw_breakpoint (struct gdbarch *gdbarch,
						  struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address = bp_tgt->reqstd_address;
  int len;
  const enum target_hw_bp_type type = hw_execute;
  struct loongarch_debug_reg_state *state
	= loongarch_get_debug_reg_state (inferior_ptid.pid ());

  gdbarch_breakpoint_from_pc (gdbarch, &addr, &len);

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"insert_hw_breakpoint on entry (addr=0x%08lx, len=%d))\n",
		(unsigned long) addr, len);

  ret = loongarch_handle_breakpoint (type, addr, len, 1 /* is_insert */,
				     inferior_ptid, state);

  if (show_debug_regs)
    {
      loongarch_show_debug_reg_state (state,
				      "insert_hw_breakpoint", addr, len, type);
    }

  return ret;
}

/* Remove a hardware-assisted breakpoint at BP_TGT->placed_address.
   Return 0 on success, -1 on failure.  */

int
loongarch_linux_nat_target::remove_hw_breakpoint (struct gdbarch *gdbarch,
						  struct bp_target_info *bp_tgt)
{
  int ret;
  CORE_ADDR addr = bp_tgt->placed_address;
  int len = 4;
  const enum target_hw_bp_type type = hw_execute;
  struct loongarch_debug_reg_state *state
    = loongarch_get_debug_reg_state (inferior_ptid.pid ());

  gdbarch_breakpoint_from_pc (gdbarch, &addr, &len);

  if (show_debug_regs)
    gdb_printf (gdb_stdlog,
		"remove_hw_breakpoint on entry (addr=0x%08lx, len=%d))\n",
		(unsigned long) addr, len);

  ret = loongarch_handle_breakpoint (type, addr, len, 0 /* is_insert */,
				     inferior_ptid, state);

  if (show_debug_regs)
    {
      loongarch_show_debug_reg_state (state,
				      "remove_hw_watchpoint", addr, len, type);
    }

  return ret;
}

/* Implement the virtual inf_ptrace_target::post_startup_inferior method.  */

void
loongarch_linux_nat_target::post_startup_inferior (ptid_t ptid)
{
  low_forget_process (ptid.pid ());
  loongarch_linux_get_debug_reg_capacity (ptid.pid ());
  linux_nat_target::post_startup_inferior (ptid);
}

/* Implement the "post_attach" target_ops method.  */

void
loongarch_linux_nat_target::post_attach (int pid)
{
  low_forget_process (pid);
  /* Get the hardware debug register capacity. If
     loongarch_linux_get_debug_reg_capacity is not called
     (as it is in loongarch_linux_child_post_startup_inferior) then
     software watchpoints will be used instead of hardware
     watchpoints when attaching to a target.  */
  loongarch_linux_get_debug_reg_capacity (pid);
  linux_nat_target::post_attach (pid);
}

/* linux_nat_new_fork hook.   */

void
loongarch_linux_nat_target::low_new_fork (struct lwp_info *parent,
					  pid_t child_pid)
{
  pid_t parent_pid;
  struct loongarch_debug_reg_state *parent_state;
  struct loongarch_debug_reg_state *child_state;

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
  parent_state = loongarch_get_debug_reg_state (parent_pid);
  child_state = loongarch_get_debug_reg_state (child_pid);
  *child_state = *parent_state;
}

/* Called whenever GDB is no longer debugging process PID.  It deletes
   data structures that keep track of debug register state.  */

void
loongarch_linux_nat_target::low_forget_process (pid_t pid)
{
  loongarch_remove_debug_reg_state (pid);
}

/* Initialize LoongArch Linux native support.  */

INIT_GDB_FILE (loongarch_linux_nat)
{
  linux_target = &the_loongarch_linux_nat_target;
  add_inf_child_target (&the_loongarch_linux_nat_target);

  /* Add a maintenance command to enable printing the LoongArch internal
     debug registers mirror variables.  */
  add_setshow_boolean_cmd ("show-debug-regs", class_maintenance,
			   &show_debug_regs, _("\
Set whether to show the LoongArch debug registers state."), _("\
Show whether to show the LoongArch debug registers state."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, the debug registers values are shown when GDB inserts\n\
or removes a hardware breakpoint or watchpoint, and when the inferior\n\
triggers a breakpoint or watchpoint."),
			   NULL,
			   NULL,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);
}
