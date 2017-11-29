/* Native-dependent code for NetBSD/hppa.

   Copyright (C) 2008-2016 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "hppa-tdep.h"
#include "inf-ptrace.h"

#include "nbsd-nat.h"

#ifndef HAVE_GREGSET_T
typedef struct reg gregset_t;
#endif

#ifndef HAVE_FPREGSET_T
typedef struct fpreg fpregset_t;
#endif

#include "gregset.h"

static int
hppanbsd_gregset_supplies_p (int regnum)
{
  return ((regnum >= HPPA_R0_REGNUM && regnum <= HPPA_R31_REGNUM) ||
          (regnum >= HPPA_SAR_REGNUM && regnum <= HPPA_PCSQ_TAIL_REGNUM) ||
          regnum == HPPA_IPSW_REGNUM ||
	  (regnum >= HPPA_SR4_REGNUM && regnum <= HPPA_SR4_REGNUM + 5));
}

static int
hppanbsd_fpregset_supplies_p (int regnum)
{
  return (regnum >= HPPA_FP0_REGNUM && regnum <= HPPA_FP31R_REGNUM);
}

/* Supply the general-purpose registers stored in GREGS to REGCACHE.  */

static void
hppanbsd_supply_gregset (struct regcache *regcache, const void *gregs)
{
  const char *regs = (const char *) gregs;
  int regnum;

  for (regnum = HPPA_R1_REGNUM; regnum <= HPPA_R31_REGNUM; regnum++)
    regcache_raw_supply (regcache, regnum, regs + regnum * 4);

  regcache_raw_supply (regcache, HPPA_SAR_REGNUM, regs + 32 * 4);
  regcache_raw_supply (regcache, HPPA_PCSQ_HEAD_REGNUM, regs + 33 * 4);
  regcache_raw_supply (regcache, HPPA_PCSQ_TAIL_REGNUM, regs + 34 * 4);
  regcache_raw_supply (regcache, HPPA_PCOQ_HEAD_REGNUM, regs + 35 * 4);
  regcache_raw_supply (regcache, HPPA_PCOQ_TAIL_REGNUM, regs + 36 * 4);
  regcache_raw_supply (regcache, HPPA_IPSW_REGNUM, regs);
  regcache_raw_supply (regcache, HPPA_SR4_REGNUM, regs + 41 * 4);
  regcache_raw_supply (regcache, HPPA_SR4_REGNUM + 1, regs + 37 * 4);
  regcache_raw_supply (regcache, HPPA_SR4_REGNUM + 2, regs + 38 * 4);
  regcache_raw_supply (regcache, HPPA_SR4_REGNUM + 3, regs + 39 * 4);
  regcache_raw_supply (regcache, HPPA_SR4_REGNUM + 4, regs + 40 * 4);
}

/* Supply the floating-point registers stored in FPREGS to REGCACHE.  */

static void
hppanbsd_supply_fpregset (struct regcache *regcache, const void *fpregs)
{
  const char *regs = (const char *) fpregs;
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
hppanbsd_collect_gregset (const struct regcache *regcache,
			  void *gregs, int regnum)
{
  char *regs = (char *) gregs;
  int i;

  for (i = HPPA_R1_REGNUM; i <= HPPA_R31_REGNUM; i++)
    {
      if (regnum == -1 || regnum == i)
	regcache_raw_collect (regcache, i, regs + i * 4);
    }

  if (regnum == -1 || regnum == HPPA_IPSW_REGNUM)
    regcache_raw_collect (regcache, HPPA_IPSW_REGNUM, regs);
  if (regnum == -1 || regnum == HPPA_PCOQ_HEAD_REGNUM)
    regcache_raw_collect (regcache, HPPA_PCOQ_HEAD_REGNUM, regs + 35 * 4);
  if (regnum == -1 || regnum == HPPA_PCOQ_TAIL_REGNUM)
    regcache_raw_collect (regcache, HPPA_PCOQ_TAIL_REGNUM, regs + 36 * 4);

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
  if (regnum == -1 || regnum == HPPA_IPSW_REGNUM)
    regcache_raw_collect (regcache, HPPA_IPSW_REGNUM, regs);
  if (regnum == -1 || regnum == HPPA_SR4_REGNUM)
    regcache_raw_collect (regcache, HPPA_SR4_REGNUM, regs + 41 * 4);
  if (regnum == -1 || regnum == HPPA_SR4_REGNUM + 1)
    regcache_raw_collect (regcache, HPPA_SR4_REGNUM + 1, regs + 37 * 4);
  if (regnum == -1 || regnum == HPPA_SR4_REGNUM + 2)
    regcache_raw_collect (regcache, HPPA_SR4_REGNUM + 2, regs + 38 * 4);
  if (regnum == -1 || regnum == HPPA_SR4_REGNUM + 3)
    regcache_raw_collect (regcache, HPPA_SR4_REGNUM + 3, regs + 39 * 4);
  if (regnum == -1 || regnum == HPPA_SR4_REGNUM + 4)
    regcache_raw_collect (regcache, HPPA_SR4_REGNUM + 4, regs + 40 * 4);
}

/* Collect the floating-point registers from REGCACHE and store them
   in FPREGS.  */

static void
hppanbsd_collect_fpregset (const struct regcache *regcache,
			  void *fpregs, int regnum)
{
  char *regs = (char *) fpregs;
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
hppanbsd_fetch_registers (struct target_ops *ops,
			  struct regcache *regcache, int regnum)

{
  if (regnum == -1 || hppanbsd_gregset_supplies_p (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &regs, ptid_get_lwp (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get registers"));

      hppanbsd_supply_gregset (regcache, &regs);
    }

  if (regnum == -1 || hppanbsd_fpregset_supplies_p (regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, ptid_get_lwp (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      hppanbsd_supply_fpregset (regcache, &fpregs);
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers (including the floating-point registers).  */

static void
hppanbsd_store_registers (struct target_ops *ops,
			  struct regcache *regcache, int regnum)
{
  if (regnum == -1 || hppanbsd_gregset_supplies_p (regnum))
    {
      struct reg regs;

      if (ptrace (PT_GETREGS, ptid_get_pid (inferior_ptid),
                  (PTRACE_TYPE_ARG3) &regs, ptid_get_lwp (inferior_ptid)) == -1)
        perror_with_name (_("Couldn't get registers"));

      hppanbsd_collect_gregset (regcache, &regs, regnum);

      if (ptrace (PT_SETREGS, ptid_get_pid (inferior_ptid),
	          (PTRACE_TYPE_ARG3) &regs, ptid_get_lwp (inferior_ptid)) == -1)
        perror_with_name (_("Couldn't write registers"));
    }

  if (regnum == -1 || hppanbsd_fpregset_supplies_p (regnum))
    {
      struct fpreg fpregs;

      if (ptrace (PT_GETFPREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, ptid_get_lwp (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't get floating point status"));

      hppanbsd_collect_fpregset (regcache, &fpregs, regnum);

      if (ptrace (PT_SETFPREGS, ptid_get_pid (inferior_ptid),
		  (PTRACE_TYPE_ARG3) &fpregs, ptid_get_lwp (inferior_ptid)) == -1)
	perror_with_name (_("Couldn't write floating point status"));
    }
}

void
supply_gregset (struct regcache *regcache, const gregset_t *gregsetp)
{
  hppanbsd_supply_gregset (regcache, gregsetp);
}

/* Fill register REGNUM (if it is a general-purpose register) in
   *GREGSETP with the value in GDB's register cache.  If REGNUM is -1,
   do this for all registers.  */

void
fill_gregset (const struct regcache *regcache,
              gregset_t *gregsetp, int regnum)
{
  hppanbsd_collect_gregset (regcache, gregsetp, regnum);
}

/* Transfering floating-point registers between GDB, inferiors and cores.  */

/* Fill GDB's register cache with the floating-point and SSE register
   values in *FPREGSETP.  */

void
supply_fpregset (struct regcache *regcache, const fpregset_t *fpregsetp)
{
  hppanbsd_supply_fpregset (regcache, fpregsetp);
}

/* Fill register REGNUM (if it is a floating-point or SSE register) in
   *FPREGSETP with the value in GDB's register cache.  If REGNUM is
   -1, do this for all registers.  */

void
fill_fpregset (const struct regcache *regcache,
               fpregset_t *fpregsetp, int regnum)
{
  hppanbsd_collect_fpregset (regcache, fpregsetp, regnum);
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_hppanbsd_nat (void);

void
_initialize_hppanbsd_nat (void)
{
  struct target_ops *t;

  /* Add some extra features to the ptrace target.  */
  t = inf_ptrace_target ();

  t->to_fetch_registers = hppanbsd_fetch_registers;
  t->to_store_registers = hppanbsd_store_registers;

  t->to_pid_to_exec_file = nbsd_pid_to_exec_file;

  add_target (t);
}
