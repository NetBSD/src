/* S390 native-dependent code for GDB, the GNU debugger.
   Copyright 2001 Free Software Foundation, Inc
   Contributed by D.J. Barrow (djbarrow@de.ibm.com,barrow_dj@yahoo.com)
   for IBM Deutschland Entwicklung GmbH, IBM Corporation.
   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include "defs.h"
#include "tm.h"
#include "regcache.h"
#include <asm/ptrace.h>
#include <sys/ptrace.h>
#include <asm/processor.h>
#include <sys/procfs.h>
#include <sys/user.h>
#include <value.h>
#include <sys/ucontext.h>
#ifndef offsetof
#define offsetof(type,member) ((size_t) &((type *)0)->member)
#endif


int
s390_register_u_addr (int blockend, int regnum)
{
  int retval;

  if (regnum >= S390_GP0_REGNUM && regnum <= S390_GP_LAST_REGNUM)
    retval = PT_GPR0 + ((regnum - S390_GP0_REGNUM) * S390_GPR_SIZE);
  else if (regnum >= S390_PSWM_REGNUM && regnum <= S390_PC_REGNUM)
    retval = PT_PSWMASK + ((regnum - S390_PSWM_REGNUM) * S390_PSW_MASK_SIZE);
  else if (regnum == S390_FPC_REGNUM)
    retval = PT_FPC;
  else if (regnum >= S390_FP0_REGNUM && regnum <= S390_FPLAST_REGNUM)
    retval =
#if CONFIG_ARCH_S390X
      PT_FPR0
#else
      PT_FPR0_HI
#endif
      + ((regnum - S390_FP0_REGNUM) * S390_FPR_SIZE);
  else if (regnum >= S390_FIRST_ACR && regnum <= S390_LAST_ACR)
    retval = PT_ACR0 + ((regnum - S390_FIRST_ACR) * S390_ACR_SIZE);
  else if (regnum >= (S390_FIRST_CR + 9) && regnum <= (S390_FIRST_CR + 11))
    retval = PT_CR_9 + ((regnum - (S390_FIRST_CR + 9)) * S390_CR_SIZE);
  else
    {
#ifdef GDBSERVER
      error ("s390_register_u_addr invalid regnum %s %d regnum=%d",
             __FILE__, (int) __LINE__, regnum);
#else
      internal_error (__FILE__, __LINE__,
                      "s390_register_u_addr invalid regnum regnum=%d",
                      regnum);
#endif
      retval = 0;
    }
  return retval + blockend;
}

#ifndef GDBSERVER
/* watch_areas are required if you put 2 or more watchpoints on the same 
   address or overlapping areas gdb will call us to delete the watchpoint 
   more than once when we try to delete them.
   attempted reference counting to reduce the number of areas unfortunately
   they didn't shrink when areas had to be split overlapping occurs. */
struct watch_area;
typedef struct watch_area watch_area;
struct watch_area
{
  watch_area *next;
  CORE_ADDR lo_addr;
  CORE_ADDR hi_addr;
};

static watch_area *watch_base = NULL;
int watch_area_cnt = 0;
static CORE_ADDR watch_lo_addr = 0, watch_hi_addr = 0;



CORE_ADDR
s390_stopped_by_watchpoint (int pid)
{
  per_lowcore_bits per_lowcore;
  ptrace_area parea;

  parea.len = sizeof (per_lowcore);
  parea.process_addr = (addr_t) & per_lowcore;
  parea.kernel_addr = offsetof (struct user_regs_struct, per_info.lowcore);
  ptrace (PTRACE_PEEKUSR_AREA, pid, &parea);
  return ((per_lowcore.perc_storage_alteration == 1) &&
	  (per_lowcore.perc_store_real_address == 0));
}


void
s390_fix_watch_points (int pid)
{
  per_struct per_info;
  ptrace_area parea;

  parea.len = sizeof (per_info);
  parea.process_addr = (addr_t) & per_info;
  parea.kernel_addr = PT_CR_9;
  ptrace (PTRACE_PEEKUSR_AREA, pid, &parea);
  /* The kernel automatically sets the psw for per depending */
  /* on whether the per control registers are set for event recording */
  /* & sets cr9 & cr10 appropriately also */
  if (watch_area_cnt)
    {
      per_info.control_regs.bits.em_storage_alteration = 1;
      per_info.control_regs.bits.storage_alt_space_ctl = 1;
    }
  else
    {
      per_info.control_regs.bits.em_storage_alteration = 0;
      per_info.control_regs.bits.storage_alt_space_ctl = 0;
    }
  per_info.starting_addr = watch_lo_addr;
  per_info.ending_addr = watch_hi_addr;
  ptrace (PTRACE_POKEUSR_AREA, pid, &parea);
}

int
s390_insert_watchpoint (int pid, CORE_ADDR addr, int len, int rw)
{
  CORE_ADDR hi_addr = addr + len - 1;
  watch_area *newarea = (watch_area *) xmalloc (sizeof (watch_area));


  if (newarea)
    {
      newarea->next = watch_base;
      watch_base = newarea;
      watch_lo_addr = min (watch_lo_addr, addr);
      watch_hi_addr = max (watch_hi_addr, hi_addr);
      newarea->lo_addr = addr;
      newarea->hi_addr = hi_addr;
      if (watch_area_cnt == 0)
	{
	  watch_lo_addr = newarea->lo_addr;
	  watch_hi_addr = newarea->hi_addr;
	}
      watch_area_cnt++;
      s390_fix_watch_points (pid);
    }
  return newarea ? 0 : -1;
}


int
s390_remove_watchpoint (int pid, CORE_ADDR addr, int len)
{
  watch_area *curr = watch_base, *prev, *matchCurr;
  CORE_ADDR hi_addr = addr + len - 1;
  CORE_ADDR watch_second_lo_addr = 0xffffffffUL, watch_second_hi_addr = 0;
  int lo_addr_ref_cnt, hi_addr_ref_cnt;
  prev = matchCurr = NULL;
  lo_addr_ref_cnt = (addr == watch_lo_addr);
  hi_addr_ref_cnt = (addr == watch_hi_addr);
  while (curr)
    {
      if (matchCurr == NULL)
	{
	  if (curr->lo_addr == addr && curr->hi_addr == hi_addr)
	    {
	      matchCurr = curr;
	      if (prev)
		prev->next = curr->next;
	      else
		watch_base = curr->next;
	    }
	  prev = curr;
	}
      if (lo_addr_ref_cnt)
	{
	  if (watch_lo_addr == curr->lo_addr)
	    lo_addr_ref_cnt++;
	  if (curr->lo_addr > watch_lo_addr &&
	      curr->lo_addr < watch_second_lo_addr)
	    watch_second_lo_addr = curr->lo_addr;
	}
      if (hi_addr_ref_cnt)
	{
	  if (watch_hi_addr == curr->hi_addr)
	    hi_addr_ref_cnt++;
	  if (curr->hi_addr < watch_hi_addr &&
	      curr->hi_addr > watch_second_hi_addr)
	    watch_second_hi_addr = curr->hi_addr;
	}
      curr = curr->next;
    }
  if (matchCurr)
    {
      xfree (matchCurr);
      watch_area_cnt--;
      if (watch_area_cnt)
	{
	  if (lo_addr_ref_cnt == 2)
	    watch_lo_addr = watch_second_lo_addr;
	  if (hi_addr_ref_cnt == 2)
	    watch_hi_addr = watch_second_hi_addr;
	}
      else
	{
	  watch_lo_addr = watch_hi_addr = 0;
	}
      s390_fix_watch_points (pid);
      return 0;
    }
  else
    {
      fprintf_unfiltered (gdb_stderr,
			  "Attempt to remove nonexistent watchpoint in s390_remove_watchpoint\n");
      return -1;
    }
}

int
kernel_u_size (void)
{
  return sizeof (struct user);
}


#if  (defined (S390_FP0_REGNUM) && defined (HAVE_FPREGSET_T) && defined(HAVE_SYS_PROCFS_H) && defined (HAVE_GREGSET_T))
void
supply_gregset (gregset_t * gregsetp)
{
  int regi;
  greg_t *gregp = (greg_t *) gregsetp;

  supply_register (S390_PSWM_REGNUM, (char *) &gregp[S390_PSWM_REGNUM]);
  supply_register (S390_PC_REGNUM, (char *) &gregp[S390_PC_REGNUM]);
  for (regi = 0; regi < S390_NUM_GPRS; regi++)
    supply_register (S390_GP0_REGNUM + regi,
		     (char *) &gregp[S390_GP0_REGNUM + regi]);
  for (regi = 0; regi < S390_NUM_ACRS; regi++)
    supply_register (S390_FIRST_ACR + regi,
		     (char *) &gregp[S390_FIRST_ACR + regi]);
  /* unfortunately this isn't in gregsetp */
  for (regi = 0; regi < S390_NUM_CRS; regi++)
    supply_register (S390_FIRST_CR + regi, NULL);
}


void
supply_fpregset (fpregset_t * fpregsetp)
{
  int regi;

  supply_register (S390_FPC_REGNUM, (char *) &fpregsetp->fpc);
  for (regi = 0; regi < S390_NUM_FPRS; regi++)
    supply_register (S390_FP0_REGNUM + regi, (char *) &fpregsetp->fprs[regi]);

}

void
fill_gregset (gregset_t * gregsetp, int regno)
{
  int regi;
  greg_t *gregp = (greg_t *) gregsetp;

  if (regno < 0) 
    {
      regcache_collect (S390_PSWM_REGNUM, &gregp[S390_PSWM_REGNUM]);
      regcache_collect (S390_PC_REGNUM, &gregp[S390_PC_REGNUM]);
      for (regi = 0; regi < S390_NUM_GPRS; regi++)
        regcache_collect (S390_GP0_REGNUM + regi,
			  &gregp[S390_GP0_REGNUM + regi]);
      for (regi = 0; regi < S390_NUM_ACRS; regi++)
        regcache_collect (S390_FIRST_ACR + regi,
			  &gregp[S390_FIRST_ACR + regi]);
    }
  else if (regno >= S390_PSWM_REGNUM && regno <= S390_LAST_ACR)
    regcache_collect (regno, &gregp[regno]);
}

/*  Given a pointer to a floating point register set in /proc format
   (fpregset_t *), update the register specified by REGNO from gdb's idea
   of the current floating point register set.  If REGNO is -1, update
   them all. */

void
fill_fpregset (fpregset_t * fpregsetp, int regno)
{
  int regi;

  if (regno < 0) 
    {
      regcache_collect (S390_FPC_REGNUM, &fpregsetp->fpc);
      for (regi = 0; regi < S390_NUM_FPRS; regi++)
        regcache_collect (S390_FP0_REGNUM + regi, &fpregsetp->fprs[regi]);
    }
  else if (regno == S390_FPC_REGNUM)
    regcache_collect (S390_FPC_REGNUM, &fpregsetp->fpc);
  else if (regno >= S390_FP0_REGNUM && regno <= S390_FPLAST_REGNUM)
    regcache_collect (regno, &fpregsetp->fprs[regno - S390_FP0_REGNUM]);
}


#else
#error "There are a few possibilities here"
#error "1) You aren't compiling for linux & don't need a core dumps to work."
#error "2) The header files sys/elf.h sys/user.h sys/ptrace.h & sys/procfs.h"
#error "libc files are inconsistent with linux/include/asm-s390/"
#error "3) you didn't do a completely clean build & delete config.cache."
#endif
#endif /* GDBSERVER */
