/* Native-dependent code for OpenBSD/hppa.

   Copyright (C) 2004-2017 Free Software Foundation, Inc.

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
#include "regcache.h"
#include "target.h"

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "hppa-tdep.h"
#include "inf-ptrace.h"

#include "obsd-nat.h"

static int
hppaobsd_gregset_supplies_p (int regnum)
{
  return (regnum >= HPPA_R0_REGNUM && regnum <= HPPA_CR27_REGNUM);
}

static int
hppaobsd_fpregset_supplies_p (int regnum)
{
  return (regnum >= HPPA_FP0_REGNUM && regnum <= HPPA_FP31R_REGNUM);
}

/* Supply the general-purpose registers stored in GREGS to REGCACHE.  */

static void
hppaobsd_supply_gregset (struct regcache *regcache, const void *gregs)
{
  gdb_byte zero[4] = { 0 };
  const char *regs = gregs;
  int regnum;

  regcache_raw_supply (regcache, HPPA_R0_REGNUM, &zero);
  for (regnum = HPPA_R1_REGNUM; regnum <= HPPA_R31_REGNUM; regnum++)
    regcache_raw_supply (regcache, regnum, regs + regnum * 4);

  if (sizeof(struct reg) >= 46 * 4)
    {
      regcache_raw_supply (regcache, HPPA_IPSW_REGNUM, regs);
      regcache_raw_supply (regcache, HPPA_SAR_REGNUM, regs + 32 * 4);
      regcache_raw_supply (regcache, HPPA_PCSQ_HEAD_REGNUM, regs + 33 * 4);
      regcache_raw_supply (regcache, HPPA_PCSQ_TAIL_REGNUM, regs + 34 * 4);
      regcache_raw_supply (regcache, HPPA_PCOQ_HEAD_REGNUM, regs + 35 * 4);
      regcache_raw_supply (regcache, HPPA_PCOQ_TAIL_REGNUM, regs + 36 * 4);
      regcache_raw_supply (regcache, HPPA_SR0_REGNUM, regs + 37 * 4);
      regcache_raw_supply (regcache, HPPA_SR1_REGNUM, regs + 38 * 4);
      regcache_raw_supply (regcache, HPPA_SR2_REGNUM, regs + 39 * 4);
      regcache_raw_supply (regcache, HPPA_SR3_REGNUM, regs + 40 * 4);
      regcache_raw_supply (regcache, HPPA_SR4_REGNUM, regs + 41 * 4);
      regcache_raw_supply (regcache, HPPA_SR5_REGNUM, regs + 42 * 4);
      regcache_raw_supply (regcache, HPPA_SR6_REGNUM, regs + 43 * 4);
      regcache_raw_supply (regcache, HPPA_SR7_REGNUM, regs + 44 * 4);
      regcache_raw_supply (regcache, HPPA_CR26_REGNUM, regs + 45 * 4);
      regcache_raw_supply (regcache, HPPA_CR27_REGNUM, regs + 46 * 4);
    } 
  else
    {
      regcache_raw_supply (regcache, HPPA_SAR_REGNUM, regs);
      regcache_raw_supply (regcache, HPPA_PCOQ_HEAD_REGNUM, regs + 32 * 4);
      regcache_raw_supply (regcache, HPPA_PCOQ_TAIL_REGNUM, regs + 33 * 4);
    }
}

/* Supply the floating-point registers stored in FPREGS to REGCACHE.  */

static void
hppaobsd_supply_fpregset (struct regcache *regcache, const void *fpregs)
{
  const char *regs = fpregs;
  int regnum;

  for (regnum = HPPA_FP0_REGNUM; regnum <= HPPA_FP31R_REGNUM;
       regnum += 2, regs += 8)
    {
      regcache_raw_supply (regcache, regnum, regs);
      regcache_raw_supply (regcache, regnum + 1, regs + 4);
    }
}

/* Collect the general-purpose registers from REGCACHE and store them
   in GREGS.  */

static void
hppaobsd_collect_gregset (const struct regcache *regcache,
			  void *gregs, int regnum)
{
  char *regs = gregs;
  int i;

  for (i = HPPA_R1_REGNUM; i <= HPPA_R31_REGNUM; i++)
    {
      if (regnum == -1 || regnum == i)
	regcache_raw_collect (regcache, i, regs + i * 4);
    }

  if (sizeof(struct reg) >= 46 * 4)
    {
      if (regnum == -1 || regnum == HPPA_IPSW_REGNUM)
	regcache_raw_collect (regcache, HPPA_IPSW_REGNUM, regs);
      if (regnum == -1 || regnum == HPPA_SAR_REGNUM)
	regcache_raw_collect (regcache, HPPA_SAR_REGNUM, regs + 32 * 4);
      if (regnum == -1 || regnum == HPPA_PCSQ_HEAD_REGNUM)
	regcache_raw_collect (regcache, HPPA_PCSQ_HEAD_REGNUM, regs + 33 * 4);
      if (regnum == -1 || regnum == HPPA_PCSQ_TAIL_REGNUM)
	regcache_raw_collect (regcache, HPPA_PCSQ_TAIL_REGNUM, regs + 34 * 4);
      if (regnum == -1 || regnum == HPPA_PCOQ_HEAD_REGNUM)
	regcache_raw_collect (regcache, HPPA_PCOQ_HEAD_REGNUM, regs + 35 * 4);
      if (regnum == -1 || regnum == HPPA_PCOQ_TAIL_REGNUM)
	regcache_raw_collect (regcache, HPPA_PCOQ_TAIL_REGNUM, regs + 36 * 4);
      if (regnum == -1 || regnum == HPPA_SR0_REGNUM)
	regcache_raw_collect (regcache, HPPA_SR0_REGNUM, regs + 37 * 4);
      if (regnum == -1 || regnum == HPPA_SR1_REGNUM)
	regcache_raw_collect (regcache, HPPA_SR1_REGNUM, regs + 38 * 4);
      if (regnum == -1 || regnum == HPPA_SR2_REGNUM)
	regcache_raw_collect (regcache, HPPA_SR2_REGNUM, regs + 39 * 4);
      if (regnum == -1 || regnum == HPPA_SR3_REGNUM)
	regcache_raw_collect (regcache, HPPA_SR3_REGNUM, regs + 40 * 4);
      if (regnum == -1 || regnum == HPPA_SR4_REGNUM)
	regcache_raw_collect (regcache, HPPA_SR4_REGNUM, regs + 41 * 4);
      if (regnum == -1 || regnum == HPPA_SR5_REGNUM)
	regcache_raw_collect (regcache, HPPA_SR5_REGNUM, regs + 42 * 4);
      if (regnum == -1 || regnum == HPPA_SR6_REGNUM)
	regcache_raw_collect (regcache, HPPA_SR6_REGNUM, regs + 43 * 4);
      if (regnum == -1 || regnum == HPPA_SR7_REGNUM)
	regcache_raw_collect (regcache, HPPA_SR7_REGNUM, regs + 44 * 4);
      if (regnum == -1 || regnum == HPPA_CR26_REGNUM)
	regcache_raw_collect (regcache, HPPA_CR26_REGNUM, regs + 45 * 4);
      if (regnum == -1 || regnum == HPPA_CR27_REGNUM)
	regcache_raw_collect (regcache, HPPA_CR27_REGNUM, regs + 46 * 4);
    }
  else
    {
      if (regnum == -1 || regnum == HPPA_SAR_REGNUM)
	regcache_raw_collect (regcache, HPPA_SAR_REGNUM, regs);
      if (regnum == -1 || regnum == HPPA_PCOQ_HEAD_REGNUM)
	regcache_raw_collect (regcache, HPPA_PCOQ_HEAD_REGNUM, regs + 32 * 4);
      if (regnum == -1 || regnum == HPPA_PCOQ_TAIL_REGNUM)
	regcache_raw_collect (regcache, HPPA_PCOQ_TAIL_REGNUM, regs + 33 * 4);
    }
}

/* Collect the floating-point registers from REGCACHE and store them
   in FPREGS.  */

static void
hppaobsd_collect_fpregset (struct regcache *regcache,
			   void *fpregs, int regnum)
{
  char *regs = fpregs;
  int i;

  for (i = HPPA_FP0_REGNUM; i <= HPPA_FP31R_REGNUM; i += 2, regs += 8)
    {
      if (regnum == -1 || regnum == i || regnum == i + 1)
	{
	  regcache_raw_collect (regcache, i, regs);
	  regcache_raw_collect (regcache, i + 1, regs + 4);
	}
    }
}


/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers (including the floating-point registers).  */

static void
hppaobsd_fetch_registers (struct target_ops *ops,
			  struct regcache *regcache, int regnum)
{
  pid_t pid = ptid_get_pid (regcache_get_ptid (regcache));

  if (regnum == -1 || hppaobsd_gregset_supplies_p (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
	perror_with_name (_("Couldn't get registers"));

      hppaobsd_supply_gregset (regcache, &regs);
    }

  if (regnum == -1 || hppaobsd_fpregset_supplies_p (regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      hppaobsd_supply_fpregset (regcache, &fpregs);
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers (including the floating-point registers).  */

static void
hppaobsd_store_registers (struct target_ops *ops,
			  struct regcache *regcache, int regnum)
{
  if (regnum == -1 || hppaobsd_gregset_supplies_p (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
        perror_with_name (_("Couldn't get registers"));

      hppaobsd_collect_gregset (regcache, &regs, regnum);

      if (ptrace (PT_SETREGS, pid, (PTRACE_TYPE_ARG3) &regs, 0) == -1)
        perror_with_name (_("Couldn't write registers"));
    }

  if (regnum == -1 || hppaobsd_fpregset_supplies_p (regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      hppaobsd_collect_fpregset (regcache, &fpregs, regnum);

      if (ptrace (PT_SETFPREGS, pid, (PTRACE_TYPE_ARG3) &fpregs, 0) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_hppaobsd_nat (void);

void
_initialize_hppaobsd_nat (void)
{
  struct target_ops *t;

  t = inf_ptrace_target ();
  t->to_fetch_registers = hppaobsd_fetch_registers;
  t->to_store_registers = hppaobsd_store_registers;
  obsd_add_target (t);
}
