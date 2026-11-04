/* GNU/Linux/LoongArch specific low level interface, for the remote server
   for GDB.
   Copyright (C) 2022-2025 Free Software Foundation, Inc.

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

#include "linux-low.h"
#include "nat/loongarch-hw-point.h"
#include "nat/loongarch-linux.h"
#include "nat/loongarch-linux-hw-point.h"
#include "tdesc.h"
#include "elf/common.h"
#include "arch/loongarch.h"

/* Linux target ops definitions for the LoongArch architecture.  */

class loongarch_target : public linux_process_target
{
public:

  const regs_info *get_regs_info () override;

  int breakpoint_kind_from_pc (CORE_ADDR *pcptr) override;

  const gdb_byte *sw_breakpoint_from_kind (int kind, int *size) override;

  bool supports_z_point_type (char z_type) override;

protected:

  void low_arch_setup () override;

  bool low_cannot_fetch_register (int regno) override;

  bool low_cannot_store_register (int regno) override;

  bool low_fetch_register (regcache *regcache, int regno) override;

  bool low_supports_breakpoints () override;

  CORE_ADDR low_get_pc (regcache *regcache) override;

  void low_set_pc (regcache *regcache, CORE_ADDR newpc) override;

  bool low_breakpoint_at (CORE_ADDR pc) override;

  int low_insert_point (raw_bkpt_type type, CORE_ADDR addr,
			int size, raw_breakpoint *bp) override;

  int low_remove_point (raw_bkpt_type type, CORE_ADDR addr,
			int size, raw_breakpoint *bp) override;

  bool low_stopped_by_watchpoint () override;

  CORE_ADDR low_stopped_data_address () override;

  arch_process_info *low_new_process () override;

  void low_delete_process (arch_process_info *info) override;

  void low_new_thread (lwp_info *) override;

  void low_delete_thread (arch_lwp_info *) override;

  void low_new_fork (process_info *parent, process_info *child) override;

  void low_prepare_to_resume (lwp_info *lwp) override;
};

/* The singleton target ops object.  */

static loongarch_target the_loongarch_target;

bool
loongarch_target::low_cannot_fetch_register (int regno)
{
  gdb_assert_not_reached ("linux target op low_cannot_fetch_register "
			  "is not implemented by the target");
}

bool
loongarch_target::low_cannot_store_register (int regno)
{
  gdb_assert_not_reached ("linux target op low_cannot_store_register "
			  "is not implemented by the target");
}

void
loongarch_target::low_prepare_to_resume (lwp_info *lwp)
{
  loongarch_linux_prepare_to_resume (lwp);
}

/* Per-process arch-specific data we want to keep.  */

struct arch_process_info
{
  struct loongarch_debug_reg_state debug_reg_state;
};

/* Implementation of linux target ops method "low_arch_setup".  */

void
loongarch_target::low_arch_setup ()
{
  static const char *expedite_regs[] = { "r3", "pc", NULL };
  loongarch_gdbarch_features features;
  target_desc_up tdesc;

  features.xlen = sizeof (elf_greg_t);
  tdesc = loongarch_create_target_description (features);

  if (tdesc->expedite_regs.empty ())
    {
      init_target_desc (tdesc.get (), expedite_regs, GDB_OSABI_LINUX);
      gdb_assert (!tdesc->expedite_regs.empty ());
    }
  current_process ()->tdesc = tdesc.release ();
  loongarch_linux_get_debug_reg_capacity (current_thread->id.lwp ());
}

/* Collect GPRs from REGCACHE into BUF.  */

static void
loongarch_fill_gregset (struct regcache *regcache, void *buf)
{
  elf_gregset_t *regset = (elf_gregset_t *) buf;
  int i;

  for (i = 1; i < 32; i++)
    collect_register (regcache, i, *regset + i);
  collect_register (regcache, LOONGARCH_ORIG_A0_REGNUM, *regset + LOONGARCH_ORIG_A0_REGNUM);
  collect_register (regcache, LOONGARCH_PC_REGNUM, *regset + LOONGARCH_PC_REGNUM);
  collect_register (regcache, LOONGARCH_BADV_REGNUM, *regset + LOONGARCH_BADV_REGNUM);
}

/* Supply GPRs from BUF into REGCACHE.  */

static void
loongarch_store_gregset (struct regcache *regcache, const void *buf)
{
  const elf_gregset_t *regset = (const elf_gregset_t *) buf;
  int i;

  supply_register_zeroed (regcache, 0);
  for (i = 1; i < 32; i++)
    supply_register (regcache, i, *regset + i);
  supply_register (regcache, LOONGARCH_ORIG_A0_REGNUM, *regset + LOONGARCH_ORIG_A0_REGNUM);
  supply_register (regcache, LOONGARCH_PC_REGNUM, *regset + LOONGARCH_PC_REGNUM);
  supply_register (regcache, LOONGARCH_BADV_REGNUM, *regset + LOONGARCH_BADV_REGNUM);
}

/* Collect FPRs from REGCACHE into BUF.  */

static void
loongarch_fill_fpregset (struct regcache *regcache, void *buf)
{
  gdb_byte *regbuf = nullptr;
  int fprsize = register_size (regcache->tdesc, LOONGARCH_FIRST_FP_REGNUM);
  int fccsize = register_size (regcache->tdesc, LOONGARCH_FIRST_FCC_REGNUM);

  for (int i = 0; i < LOONGARCH_LINUX_NUM_FPREGSET; i++)
    {
      regbuf = (gdb_byte *)buf + fprsize * i;
      collect_register (regcache, LOONGARCH_FIRST_FP_REGNUM + i, regbuf);
    }

  for (int i = 0; i < LOONGARCH_LINUX_NUM_FCC; i++)
    {
      regbuf = (gdb_byte *)buf + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * i;
      collect_register (regcache, LOONGARCH_FIRST_FCC_REGNUM + i, regbuf);
    }

  regbuf = (gdb_byte *)buf + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
    fccsize * LOONGARCH_LINUX_NUM_FCC;
  collect_register (regcache, LOONGARCH_FCSR_REGNUM, regbuf);
}

/* Supply FPRs from BUF into REGCACHE.  */

static void
loongarch_store_fpregset (struct regcache *regcache, const void *buf)
{
  const gdb_byte *regbuf = nullptr;
  int fprsize = register_size (regcache->tdesc, LOONGARCH_FIRST_FP_REGNUM);
  int fccsize = register_size (regcache->tdesc, LOONGARCH_FIRST_FCC_REGNUM);

  for (int i = 0; i < LOONGARCH_LINUX_NUM_FPREGSET; i++)
    {
      regbuf = (const gdb_byte *)buf + fprsize * i;
      supply_register (regcache, LOONGARCH_FIRST_FP_REGNUM + i, regbuf);
    }

  for (int i = 0; i < LOONGARCH_LINUX_NUM_FCC; i++)
    {
      regbuf = (const gdb_byte *)buf + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
	fccsize * i;
      supply_register (regcache, LOONGARCH_FIRST_FCC_REGNUM + i, regbuf);
    }

  regbuf = (const gdb_byte *)buf + fprsize * LOONGARCH_LINUX_NUM_FPREGSET +
    fccsize * LOONGARCH_LINUX_NUM_FCC;
  supply_register (regcache, LOONGARCH_FCSR_REGNUM, regbuf);
}

/* Collect lsx regs from REGCACHE into BUF.  */

static void
loongarch_fill_lsxregset (struct regcache *regcache, void *buf)
{
  elf_lsxregset_t *regset = (elf_lsxregset_t *) buf;
  int i;

  for (i = 0; i < LOONGARCH_LINUX_NUM_LSXREGSET; i++)
    collect_register (regcache, LOONGARCH_FIRST_LSX_REGNUM + i, *regset + i);
}

/* Supply lsx regs from BUF into REGCACHE.  */

static void
loongarch_store_lsxregset (struct regcache *regcache, const void *buf)
{
  const elf_lsxregset_t *regset = (const elf_lsxregset_t *) buf;
  int i;

  for (i = 0; i < LOONGARCH_LINUX_NUM_LSXREGSET; i++)
    supply_register (regcache, LOONGARCH_FIRST_LSX_REGNUM + i, *regset + i);
}

/* Collect lasx regs from REGCACHE into BUF.  */

static void
loongarch_fill_lasxregset (struct regcache *regcache, void *buf)
{
  elf_lasxregset_t *regset = (elf_lasxregset_t *) buf;
  int i;

  for (i = 0; i < LOONGARCH_LINUX_NUM_LASXREGSET; i++)
    collect_register (regcache, LOONGARCH_FIRST_LASX_REGNUM + i, *regset + i);
}

/* Supply lasx regs from BUF into REGCACHE.  */

static void
loongarch_store_lasxregset (struct regcache *regcache, const void *buf)
{
  const elf_lasxregset_t *regset = (const elf_lasxregset_t *) buf;
  int i;

  for (i = 0; i < LOONGARCH_LINUX_NUM_LASXREGSET; i++)
    supply_register (regcache, LOONGARCH_FIRST_LASX_REGNUM + i, *regset + i);
}

/* Collect lbt regs from REGCACHE into BUF.  */

static void
loongarch_fill_lbtregset (struct regcache *regcache, void *buf)
{
  gdb_byte *regbuf = (gdb_byte*)buf;
  int scrsize = register_size (regcache->tdesc, LOONGARCH_FIRST_SCR_REGNUM);
  int eflagssize = register_size (regcache->tdesc, LOONGARCH_EFLAGS_REGNUM);
  int i;

  for (i = 0; i < LOONGARCH_LINUX_NUM_SCR; i++)
    collect_register (regcache, LOONGARCH_FIRST_SCR_REGNUM + i, regbuf + scrsize * i);

  collect_register (regcache, LOONGARCH_EFLAGS_REGNUM,
		    regbuf + LOONGARCH_LINUX_NUM_SCR * scrsize);
  collect_register (regcache, LOONGARCH_FTOP_REGNUM,
		    regbuf + LOONGARCH_LINUX_NUM_SCR * scrsize + eflagssize);

}

/* Supply lbt regs from BUF into REGCACHE.  */

static void
loongarch_store_lbtregset (struct regcache *regcache, const void *buf)
{

  gdb_byte *regbuf = (gdb_byte*)buf;
  int scrsize = register_size (regcache->tdesc, LOONGARCH_FIRST_SCR_REGNUM);
  int eflagssize = register_size (regcache->tdesc, LOONGARCH_EFLAGS_REGNUM);
  int i;

  for (i = 0; i < LOONGARCH_LINUX_NUM_SCR; i++)
    supply_register (regcache, LOONGARCH_FIRST_SCR_REGNUM + i, regbuf + scrsize * i);

  supply_register (regcache, LOONGARCH_EFLAGS_REGNUM,
		    regbuf + LOONGARCH_LINUX_NUM_SCR * scrsize);
  supply_register (regcache, LOONGARCH_FTOP_REGNUM,
		    regbuf + LOONGARCH_LINUX_NUM_SCR * scrsize + eflagssize);

}

/* LoongArch/Linux regsets.  */
static struct regset_info loongarch_regsets[] = {
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_PRSTATUS, sizeof (elf_gregset_t),
    GENERAL_REGS, loongarch_fill_gregset, loongarch_store_gregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_FPREGSET, sizeof (elf_fpregset_t),
    FP_REGS, loongarch_fill_fpregset, loongarch_store_fpregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_LARCH_LSX, sizeof (elf_lsxregset_t),
    OPTIONAL_REGS, loongarch_fill_lsxregset, loongarch_store_lsxregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_LARCH_LASX, sizeof (elf_lasxregset_t),
    OPTIONAL_REGS, loongarch_fill_lasxregset, loongarch_store_lasxregset },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_LARCH_LBT, LOONGARCH_LBT_REGS_SIZE,
    OPTIONAL_REGS, loongarch_fill_lbtregset, loongarch_store_lbtregset },
  NULL_REGSET
};

/* LoongArch/Linux regset information.  */
static struct regsets_info loongarch_regsets_info =
  {
    loongarch_regsets, /* regsets */
    0, /* num_regsets */
    NULL, /* disabled_regsets */
  };

/* Definition of linux_target_ops data member "regs_info".  */
static struct regs_info loongarch_regs =
  {
    NULL, /* regset_bitmap */
    NULL, /* usrregs */
    &loongarch_regsets_info,
  };

/* Implementation of linux target ops method "get_regs_info".  */

const regs_info *
loongarch_target::get_regs_info ()
{
  return &loongarch_regs;
}

/* Implementation of linux target ops method "low_fetch_register".  */

bool
loongarch_target::low_fetch_register (regcache *regcache, int regno)
{
  if (regno != 0)
    return false;
  supply_register_zeroed (regcache, 0);
  return true;
}

bool
loongarch_target::low_supports_breakpoints ()
{
  return true;
}

/* Implementation of linux target ops method "low_get_pc".  */

CORE_ADDR
loongarch_target::low_get_pc (regcache *regcache)
{
  if (register_size (regcache->tdesc, 0) == 8)
    return linux_get_pc_64bit (regcache);
  else
    return linux_get_pc_32bit (regcache);
}

/* Implementation of linux target ops method "low_set_pc".  */

void
loongarch_target::low_set_pc (regcache *regcache, CORE_ADDR newpc)
{
  if (register_size (regcache->tdesc, 0) == 8)
    linux_set_pc_64bit (regcache, newpc);
  else
    linux_set_pc_32bit (regcache, newpc);
}

#define loongarch_breakpoint_len 4

/* LoongArch BRK software debug mode instruction.
   This instruction needs to match gdb/loongarch-tdep.c
   (loongarch_default_breakpoint).  */
static const gdb_byte loongarch_breakpoint[] = {0x05, 0x00, 0x2a, 0x00};

/* Implementation of target ops method "breakpoint_kind_from_pc".  */

int
loongarch_target::breakpoint_kind_from_pc (CORE_ADDR *pcptr)
{
  return loongarch_breakpoint_len;
}

/* Implementation of target ops method "sw_breakpoint_from_kind".  */

const gdb_byte *
loongarch_target::sw_breakpoint_from_kind (int kind, int *size)
{
  *size = loongarch_breakpoint_len;
  return (const gdb_byte *) &loongarch_breakpoint;
}

/* Implementation of linux target ops method "low_breakpoint_at".  */

bool
loongarch_target::low_breakpoint_at (CORE_ADDR pc)
{
  gdb_byte insn[loongarch_breakpoint_len];

  read_memory (pc, (unsigned char *) &insn, loongarch_breakpoint_len);
  if (memcmp (insn, loongarch_breakpoint, loongarch_breakpoint_len) == 0)
    return true;

  return false;
}

static void
loongarch_init_debug_reg_state (struct loongarch_debug_reg_state *state)
{
  int i;

  for (i = 0; i < LOONGARCH_HBP_MAX_NUM; ++i)
    {
      state->dr_addr_bp[i] = 0;
      state->dr_ctrl_bp[i] = 0;
      state->dr_ref_count_bp[i] = 0;
    }

  for (i = 0; i < LOONGARCH_HWP_MAX_NUM; ++i)
    {
      state->dr_addr_wp[i] = 0;
      state->dr_ctrl_wp[i] = 0;
      state->dr_ref_count_wp[i] = 0;
    }
}

/* See nat/loongarch-linux-hw-point.h.  */

struct loongarch_debug_reg_state *
loongarch_get_debug_reg_state (pid_t pid)
{
  struct process_info *proc = find_process_pid (pid);

  return &proc->priv->arch_private->debug_reg_state;
}

/* Implementation of target ops method "supports_z_point_type".  */

bool
loongarch_target::supports_z_point_type (char z_type)
{
  switch (z_type)
    {
    case Z_PACKET_SW_BP:
    case Z_PACKET_HW_BP:
    case Z_PACKET_WRITE_WP:
    case Z_PACKET_READ_WP:
    case Z_PACKET_ACCESS_WP:
      return true;
    default:
      return false;
    }
}

/* Implementation of linux target ops method "low_insert_point".

   It actually only records the info of the to-be-inserted bp/wp;
   the actual insertion will happen when threads are resumed.  */

int
loongarch_target::low_insert_point (raw_bkpt_type type, CORE_ADDR addr,
				    int len, raw_breakpoint *bp)
{
  int ret;
  enum target_hw_bp_type targ_type;
  struct loongarch_debug_reg_state *state
    = loongarch_get_debug_reg_state (current_thread->id.pid ());

  if (show_debug_regs)
    fprintf (stderr, "insert_point on entry (addr=0x%08lx, len=%d)\n",
	     (unsigned long) addr, len);

  /* Determine the type from the raw breakpoint type.  */
  targ_type = raw_bkpt_type_to_target_hw_bp_type (type);

  if (targ_type != hw_execute)
    {
      if (loongarch_region_ok_for_watchpoint (addr, len))
	ret = loongarch_handle_watchpoint (targ_type, addr, len,
					   1 /* is_insert */,
					   current_lwp_ptid (), state);
      else
	ret = -1;
    }
  else
    {
      ret = loongarch_handle_breakpoint (targ_type, addr, len,
					 1 /* is_insert */, current_lwp_ptid (),
					 state);
    }

  if (show_debug_regs)
    loongarch_show_debug_reg_state (state, "insert_point", addr, len,
				    targ_type);

  return ret;
}

/* Implementation of linux target ops method "low_remove_point".

   It actually only records the info of the to-be-removed bp/wp,
   the actual removal will be done when threads are resumed.  */

int
loongarch_target::low_remove_point (raw_bkpt_type type, CORE_ADDR addr,
				    int len, raw_breakpoint *bp)
{
  int ret;
  enum target_hw_bp_type targ_type;
  struct loongarch_debug_reg_state *state
    = loongarch_get_debug_reg_state (current_thread->id.pid ());

  if (show_debug_regs)
    fprintf (stderr, "remove_point on entry (addr=0x%08lx, len=%d)\n",
	     (unsigned long) addr, len);

  /* Determine the type from the raw breakpoint type.  */
  targ_type = raw_bkpt_type_to_target_hw_bp_type (type);

  /* Set up state pointers.  */
  if (targ_type != hw_execute)
    ret =
      loongarch_handle_watchpoint (targ_type, addr, len, 0 /* is_insert */,
				   current_lwp_ptid (), state);
  else
    {
      ret = loongarch_handle_breakpoint (targ_type, addr, len,
					 0 /* is_insert */,  current_lwp_ptid (),
					 state);
    }

  if (show_debug_regs)
    loongarch_show_debug_reg_state (state, "remove_point", addr, len,
				    targ_type);

  return ret;
}


/* Implementation of linux target ops method "low_stopped_data_address".  */

CORE_ADDR
loongarch_target::low_stopped_data_address ()
{
  siginfo_t siginfo;
  struct loongarch_debug_reg_state *state;
  int pid = current_thread->id.lwp ();

  /* Get the siginfo.  */
  if (ptrace (PTRACE_GETSIGINFO, pid, NULL, &siginfo) != 0)
    return (CORE_ADDR) 0;

  /* Need to be a hardware breakpoint/watchpoint trap.  */
  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != 0x0004 /* TRAP_HWBKPT */)
    return (CORE_ADDR) 0;

  /* Check if the address matches any watched address.  */
  state = loongarch_get_debug_reg_state (current_thread->id.pid ());
  CORE_ADDR result;
  if (loongarch_stopped_data_address (state, (CORE_ADDR) siginfo.si_addr, &result))
    return result;

  return (CORE_ADDR) 0;
}

/* Implementation of linux target ops method "low_stopped_by_watchpoint".  */

bool
loongarch_target::low_stopped_by_watchpoint ()
{
  return (low_stopped_data_address () != 0);
}

/* Implementation of linux target ops method "low_new_process".  */

arch_process_info *
loongarch_target::low_new_process ()
{
  struct arch_process_info *info = XCNEW (struct arch_process_info);

  loongarch_init_debug_reg_state (&info->debug_reg_state);

  return info;
}

/* Implementation of linux target ops method "low_delete_process".  */

void
loongarch_target::low_delete_process (arch_process_info *info)
{
  xfree (info);
}

void
loongarch_target::low_new_thread (lwp_info *lwp)
{
  loongarch_linux_new_thread (lwp);
}

void
loongarch_target::low_delete_thread (arch_lwp_info *arch_lwp)
{
  loongarch_linux_delete_thread (arch_lwp);
}

/* Implementation of linux target ops method "low_new_fork".  */

void
loongarch_target::low_new_fork (process_info *parent,
			      process_info *child)
{
  /* These are allocated by linux_add_process.  */
  gdb_assert (parent->priv != NULL
	      && parent->priv->arch_private != NULL);
  gdb_assert (child->priv != NULL
	      && child->priv->arch_private != NULL);

  /* GDB core assumes the child inherits the watchpoints/hw
     breakpoints of the parent, and will remove them all from the
     forked off process. Copy the debug registers mirrors into the
     new process so that all breakpoints and watchpoints can be
     removed together.  The debug registers mirror will become zeroed
     in the end before detaching the forked off process, thus making
     this compatible with older Linux kernels too.  */

  *child->priv->arch_private = *parent->priv->arch_private;
}

/* The linux target ops object.  */

linux_process_target *the_linux_target = &the_loongarch_target;

/* Initialize the LoongArch/Linux target.  */

void
initialize_low_arch ()
{
  initialize_regsets_info (&loongarch_regsets_info);
}
