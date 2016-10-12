/* Xtensa GNU/Linux native support.

   Copyright (C) 2007-2015 Free Software Foundation, Inc.

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
#include "frame.h"
#include "inferior.h"
#include "gdbcore.h"
#include "regcache.h"
#include "target.h"
#include "linux-nat.h"
#include <sys/types.h>
#include <signal.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include "gdb_wait.h"
#include <fcntl.h>
#include <sys/procfs.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>

#include "gregset.h"
#include "xtensa-tdep.h"

/* Extended register set depends on hardware configs.
   Keeping these definitions separately allows to introduce
   hardware-specific overlays.  */
#include "xtensa-xtregs.c"

static int
get_thread_id (ptid_t ptid)
{
  int tid = ptid_get_lwp (ptid);
  if (0 == tid)
    tid = ptid_get_pid (ptid);
  return tid;
}
#define GET_THREAD_ID(PTID)	get_thread_id (PTID)

void
fill_gregset (const struct regcache *regcache,
	      gdb_gregset_t *gregsetp, int regnum)
{
  int i;
  xtensa_elf_gregset_t *regs = (xtensa_elf_gregset_t *) gregsetp;
  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  if (regnum == gdbarch_pc_regnum (gdbarch) || regnum == -1)
    regcache_raw_collect (regcache, gdbarch_pc_regnum (gdbarch), &regs->pc);
  if (regnum == gdbarch_ps_regnum (gdbarch) || regnum == -1)
    regcache_raw_collect (regcache, gdbarch_ps_regnum (gdbarch), &regs->ps);

  if (regnum == gdbarch_tdep (gdbarch)->wb_regnum || regnum == -1)
    regcache_raw_collect (regcache,
			  gdbarch_tdep (gdbarch)->wb_regnum,
			  &regs->windowbase);
  if (regnum == gdbarch_tdep (gdbarch)->ws_regnum || regnum == -1)
    regcache_raw_collect (regcache,
			  gdbarch_tdep (gdbarch)->ws_regnum,
			  &regs->windowstart);
  if (regnum == gdbarch_tdep (gdbarch)->lbeg_regnum || regnum == -1)
    regcache_raw_collect (regcache,
			  gdbarch_tdep (gdbarch)->lbeg_regnum,
			  &regs->lbeg);
  if (regnum == gdbarch_tdep (gdbarch)->lend_regnum || regnum == -1)
    regcache_raw_collect (regcache,
			  gdbarch_tdep (gdbarch)->lend_regnum,
			  &regs->lend);
  if (regnum == gdbarch_tdep (gdbarch)->lcount_regnum || regnum == -1)
    regcache_raw_collect (regcache,
			  gdbarch_tdep (gdbarch)->lcount_regnum,
			  &regs->lcount);
  if (regnum == gdbarch_tdep (gdbarch)->sar_regnum || regnum == -1)
    regcache_raw_collect (regcache,
			  gdbarch_tdep (gdbarch)->sar_regnum,
			  &regs->sar);
  if (regnum >=gdbarch_tdep (gdbarch)->ar_base
      && regnum < gdbarch_tdep (gdbarch)->ar_base
		    + gdbarch_tdep (gdbarch)->num_aregs)
    regcache_raw_collect (regcache,regnum,
			  &regs->ar[regnum - gdbarch_tdep (gdbarch)->ar_base]);
  else if (regnum == -1)
    {
      for (i = 0; i < gdbarch_tdep (gdbarch)->num_aregs; ++i)
	regcache_raw_collect (regcache,
			      gdbarch_tdep (gdbarch)->ar_base + i,
			      &regs->ar[i]);
    }
}

void
supply_gregset_reg (struct regcache *regcache,
		    const gdb_gregset_t *gregsetp, int regnum)
{
  int i;
  xtensa_elf_gregset_t *regs = (xtensa_elf_gregset_t *) gregsetp;

  struct gdbarch *gdbarch = get_regcache_arch (regcache);

  if (regnum == gdbarch_pc_regnum (gdbarch) || regnum == -1)
    regcache_raw_supply (regcache, gdbarch_pc_regnum (gdbarch), &regs->pc);
  if (regnum == gdbarch_ps_regnum (gdbarch) || regnum == -1)
    regcache_raw_supply (regcache, gdbarch_ps_regnum (gdbarch), &regs->ps);

  if (regnum == gdbarch_tdep (gdbarch)->wb_regnum || regnum == -1)
    regcache_raw_supply (regcache,
			  gdbarch_tdep (gdbarch)->wb_regnum,
			  &regs->windowbase);
  if (regnum == gdbarch_tdep (gdbarch)->ws_regnum || regnum == -1)
    regcache_raw_supply (regcache,
			  gdbarch_tdep (gdbarch)->ws_regnum,
			  &regs->windowstart);
  if (regnum == gdbarch_tdep (gdbarch)->lbeg_regnum || regnum == -1)
    regcache_raw_supply (regcache,
			  gdbarch_tdep (gdbarch)->lbeg_regnum,
			  &regs->lbeg);
  if (regnum == gdbarch_tdep (gdbarch)->lend_regnum || regnum == -1)
    regcache_raw_supply (regcache,
			  gdbarch_tdep (gdbarch)->lend_regnum,
			  &regs->lend);
  if (regnum == gdbarch_tdep (gdbarch)->lcount_regnum || regnum == -1)
    regcache_raw_supply (regcache,
			  gdbarch_tdep (gdbarch)->lcount_regnum,
			  &regs->lcount);
  if (regnum == gdbarch_tdep (gdbarch)->sar_regnum || regnum == -1)
    regcache_raw_supply (regcache,
			  gdbarch_tdep (gdbarch)->sar_regnum,
			  &regs->sar);
  if (regnum >=gdbarch_tdep (gdbarch)->ar_base
      && regnum < gdbarch_tdep (gdbarch)->ar_base
		    + gdbarch_tdep (gdbarch)->num_aregs)
    regcache_raw_supply (regcache,regnum,
			  &regs->ar[regnum - gdbarch_tdep (gdbarch)->ar_base]);
  else if (regnum == -1)
    {
      for (i = 0; i < gdbarch_tdep (gdbarch)->num_aregs; ++i)
	regcache_raw_supply (regcache,
			      gdbarch_tdep (gdbarch)->ar_base + i,
			      &regs->ar[i]);
    }
}

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregsetp)
{
  supply_gregset_reg (regcache, gregsetp, -1);
}

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregsetp, int regnum)
{
  return;
}

void 
supply_fpregset (struct regcache *regcache,
		 const gdb_fpregset_t *fpregsetp)
{
  return;
}

/* Fetch greg-register(s) from process/thread TID
   and store value(s) in GDB's register array.  */

static void
fetch_gregs (struct regcache *regcache, int regnum)
{
  int tid = GET_THREAD_ID (inferior_ptid);
  const gdb_gregset_t regs;
  int areg;
  
  if (ptrace (PTRACE_GETREGS, tid, 0, (long) &regs) < 0)
    {
      perror_with_name (_("Couldn't get registers"));
      return;
    }
 
  supply_gregset_reg (regcache, &regs, regnum);
}

/* Store greg-register(s) in GDB's register 
   array into the process/thread specified by TID.  */

static void
store_gregs (struct regcache *regcache, int regnum)
{
  int tid = GET_THREAD_ID (inferior_ptid);
  gdb_gregset_t regs;
  int areg;

  if (ptrace (PTRACE_GETREGS, tid, 0, (long) &regs) < 0)
    {
      perror_with_name (_("Couldn't get registers"));
      return;
    }

  fill_gregset (regcache, &regs, regnum);

  if (ptrace (PTRACE_SETREGS, tid, 0, (long) &regs) < 0)
    {
      perror_with_name (_("Couldn't write registers"));
      return;
    }
}

static int xtreg_lo;
static int xtreg_high;

/* Fetch/Store Xtensa TIE registers.  Xtensa GNU/Linux PTRACE
   interface provides special requests for this.  */

static void
fetch_xtregs (struct regcache *regcache, int regnum)
{
  int tid = GET_THREAD_ID (inferior_ptid);
  const xtensa_regtable_t *ptr;
  char xtregs [XTENSA_ELF_XTREG_SIZE];

  if (ptrace (PTRACE_GETXTREGS, tid, 0, (long)&xtregs) < 0)
    perror_with_name (_("Couldn't get extended registers"));

  for (ptr = xtensa_regmap_table; ptr->name; ptr++)
    if (regnum == ptr->gdb_regnum || regnum == -1)
      regcache_raw_supply (regcache, ptr->gdb_regnum,
			   xtregs + ptr->ptrace_offset);
}

static void
store_xtregs (struct regcache *regcache, int regnum)
{
  int tid = GET_THREAD_ID (inferior_ptid);
  const xtensa_regtable_t *ptr;
  char xtregs [XTENSA_ELF_XTREG_SIZE];

  if (ptrace (PTRACE_GETXTREGS, tid, 0, (long)&xtregs) < 0)
    perror_with_name (_("Couldn't get extended registers"));

  for (ptr = xtensa_regmap_table; ptr->name; ptr++)
    if (regnum == ptr->gdb_regnum || regnum == -1)
      regcache_raw_collect (regcache, ptr->gdb_regnum,
			    xtregs + ptr->ptrace_offset);

  if (ptrace (PTRACE_SETXTREGS, tid, 0, (long)&xtregs) < 0)
    perror_with_name (_("Couldn't write extended registers"));
}

void
xtensa_linux_fetch_inferior_registers (struct target_ops *ops,
				       struct regcache *regcache, int regnum)
{
  if (regnum == -1)
    {
      fetch_gregs (regcache, regnum);
      fetch_xtregs (regcache, regnum);
    }
  else if ((regnum < xtreg_lo) || (regnum > xtreg_high))
    fetch_gregs (regcache, regnum);
  else
    fetch_xtregs (regcache, regnum);
}

void
xtensa_linux_store_inferior_registers (struct target_ops *ops,
				       struct regcache *regcache, int regnum)
{
  if (regnum == -1)
    {
      store_gregs (regcache, regnum);
      store_xtregs (regcache, regnum);
    }
  else if ((regnum < xtreg_lo) || (regnum > xtreg_high))
    store_gregs (regcache, regnum);
  else
    store_xtregs (regcache, regnum);
}

void _initialize_xtensa_linux_nat (void);

void
_initialize_xtensa_linux_nat (void)
{
  struct target_ops *t;
  const xtensa_regtable_t *ptr;

  /* Calculate the number range for extended registers.  */
  xtreg_lo = 1000000000;
  xtreg_high = -1;
  for (ptr = xtensa_regmap_table; ptr->name; ptr++)
    {
      if (ptr->gdb_regnum < xtreg_lo)
	xtreg_lo = ptr->gdb_regnum;
      if (ptr->gdb_regnum > xtreg_high)
	xtreg_high = ptr->gdb_regnum;
    }

  /* Fill in the generic GNU/Linux methods.  */
  t = linux_target ();

  /* Add our register access methods.  */
  t->to_fetch_registers = xtensa_linux_fetch_inferior_registers;
  t->to_store_registers = xtensa_linux_store_inferior_registers;

  linux_nat_add_target (t);
}
