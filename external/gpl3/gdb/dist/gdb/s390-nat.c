/* S390 native-dependent code for GDB, the GNU debugger.
   Copyright (C) 2001-2013 Free Software Foundation, Inc.

   Contributed by D.J. Barrow (djbarrow@de.ibm.com,barrow_dj@yahoo.com)
   for IBM Deutschland Entwicklung GmbH, IBM Corporation.

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
#include "regcache.h"
#include "inferior.h"
#include "target.h"
#include "linux-nat.h"
#include "auxv.h"
#include "gregset.h"

#include "s390-tdep.h"
#include "elf/common.h"

#include <asm/ptrace.h>
#include <sys/ptrace.h>
#include <asm/types.h>
#include <sys/procfs.h>
#include <sys/ucontext.h>
#include <elf.h>

#ifndef HWCAP_S390_HIGH_GPRS
#define HWCAP_S390_HIGH_GPRS 512
#endif

#ifndef PTRACE_GETREGSET
#define PTRACE_GETREGSET 0x4204
#endif

#ifndef PTRACE_SETREGSET
#define PTRACE_SETREGSET 0x4205
#endif

static int have_regset_last_break = 0;
static int have_regset_system_call = 0;

/* Map registers to gregset/ptrace offsets.
   These arrays are defined in s390-tdep.c.  */

#ifdef __s390x__
#define regmap_gregset s390x_regmap_gregset
#else
#define regmap_gregset s390_regmap_gregset
#endif

#define regmap_fpregset s390_regmap_fpregset

/* When debugging a 32-bit executable running under a 64-bit kernel,
   we have to fix up the 64-bit registers we get from the kernel
   to make them look like 32-bit registers.  */

static void
s390_native_supply (struct regcache *regcache, int regno,
		    const gdb_byte *regp, int *regmap)
{
  int offset = regmap[regno];

#ifdef __s390x__
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (offset != -1 && gdbarch_ptr_bit (gdbarch) == 32)
    {
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

      if (regno == S390_PSWM_REGNUM)
	{
	  ULONGEST pswm;
	  gdb_byte buf[4];

	  pswm = extract_unsigned_integer (regp + regmap[S390_PSWM_REGNUM],
					   8, byte_order);

	  store_unsigned_integer (buf, 4, byte_order, (pswm >> 32) | 0x80000);
	  regcache_raw_supply (regcache, regno, buf);
	  return;
	}

      if (regno == S390_PSWA_REGNUM)
	{
	  ULONGEST pswm, pswa;
	  gdb_byte buf[4];

	  pswa = extract_unsigned_integer (regp + regmap[S390_PSWA_REGNUM],
					   8, byte_order);
	  pswm = extract_unsigned_integer (regp + regmap[S390_PSWM_REGNUM],
					   8, byte_order);

	  store_unsigned_integer (buf, 4, byte_order,
				  (pswa & 0x7fffffff) | (pswm & 0x80000000));
	  regcache_raw_supply (regcache, regno, buf);
	  return;
	}

      if ((regno >= S390_R0_REGNUM && regno <= S390_R15_REGNUM)
	  || regno == S390_ORIG_R2_REGNUM)
	offset += 4;
    }
#endif

  if (offset != -1)
    regcache_raw_supply (regcache, regno, regp + offset);
}

static void
s390_native_collect (const struct regcache *regcache, int regno,
		     gdb_byte *regp, int *regmap)
{
  int offset = regmap[regno];

#ifdef __s390x__
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (offset != -1 && gdbarch_ptr_bit (gdbarch) == 32)
    {
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

      if (regno == S390_PSWM_REGNUM)
	{
	  ULONGEST pswm;
	  gdb_byte buf[4];

	  regcache_raw_collect (regcache, regno, buf);
	  pswm = extract_unsigned_integer (buf, 4, byte_order);

	  /* We don't know the final addressing mode until the PSW address
	     is known, so leave it as-is.  When the PSW address is collected
	     (below), the addressing mode will be updated.  */
	  store_unsigned_integer (regp + regmap[S390_PSWM_REGNUM],
				  4, byte_order, pswm & 0xfff7ffff);
	  return;
	}

      if (regno == S390_PSWA_REGNUM)
	{
	  ULONGEST pswa;
	  gdb_byte buf[4];

	  regcache_raw_collect (regcache, regno, buf);
	  pswa = extract_unsigned_integer (buf, 4, byte_order);

	  store_unsigned_integer (regp + regmap[S390_PSWA_REGNUM],
				  8, byte_order, pswa & 0x7fffffff);

	  /* Update basic addressing mode bit in PSW mask, see above.  */
	  store_unsigned_integer (regp + regmap[S390_PSWM_REGNUM] + 4,
				  4, byte_order, pswa & 0x80000000);
	  return;
	}

      if ((regno >= S390_R0_REGNUM && regno <= S390_R15_REGNUM)
	  || regno == S390_ORIG_R2_REGNUM)
	{
	  memset (regp + offset, 0, 4);
	  offset += 4;
	}
    }
#endif

  if (offset != -1)
    regcache_raw_collect (regcache, regno, regp + offset);
}

/* Fill GDB's register array with the general-purpose register values
   in *REGP.  */
void
supply_gregset (struct regcache *regcache, const gregset_t *regp)
{
  int i;
  for (i = 0; i < S390_NUM_REGS; i++)
    s390_native_supply (regcache, i, (const gdb_byte *) regp, regmap_gregset);
}

/* Fill register REGNO (if it is a general-purpose register) in
   *REGP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */
void
fill_gregset (const struct regcache *regcache, gregset_t *regp, int regno)
{
  int i;
  for (i = 0; i < S390_NUM_REGS; i++)
    if (regno == -1 || regno == i)
      s390_native_collect (regcache, i, (gdb_byte *) regp, regmap_gregset);
}

/* Fill GDB's register array with the floating-point register values
   in *REGP.  */
void
supply_fpregset (struct regcache *regcache, const fpregset_t *regp)
{
  int i;
  for (i = 0; i < S390_NUM_REGS; i++)
    s390_native_supply (regcache, i, (const gdb_byte *) regp, regmap_fpregset);
}

/* Fill register REGNO (if it is a general-purpose register) in
   *REGP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */
void
fill_fpregset (const struct regcache *regcache, fpregset_t *regp, int regno)
{
  int i;
  for (i = 0; i < S390_NUM_REGS; i++)
    if (regno == -1 || regno == i)
      s390_native_collect (regcache, i, (gdb_byte *) regp, regmap_fpregset);
}

/* Find the TID for the current inferior thread to use with ptrace.  */
static int
s390_inferior_tid (void)
{
  /* GNU/Linux LWP ID's are process ID's.  */
  int tid = TIDGET (inferior_ptid);
  if (tid == 0)
    tid = PIDGET (inferior_ptid); /* Not a threaded program.  */

  return tid;
}

/* Fetch all general-purpose registers from process/thread TID and
   store their values in GDB's register cache.  */
static void
fetch_regs (struct regcache *regcache, int tid)
{
  gregset_t regs;
  ptrace_area parea;

  parea.len = sizeof (regs);
  parea.process_addr = (addr_t) &regs;
  parea.kernel_addr = offsetof (struct user_regs_struct, psw);
  if (ptrace (PTRACE_PEEKUSR_AREA, tid, (long) &parea) < 0)
    perror_with_name (_("Couldn't get registers"));

  supply_gregset (regcache, (const gregset_t *) &regs);
}

/* Store all valid general-purpose registers in GDB's register cache
   into the process/thread specified by TID.  */
static void
store_regs (const struct regcache *regcache, int tid, int regnum)
{
  gregset_t regs;
  ptrace_area parea;

  parea.len = sizeof (regs);
  parea.process_addr = (addr_t) &regs;
  parea.kernel_addr = offsetof (struct user_regs_struct, psw);
  if (ptrace (PTRACE_PEEKUSR_AREA, tid, (long) &parea) < 0)
    perror_with_name (_("Couldn't get registers"));

  fill_gregset (regcache, &regs, regnum);

  if (ptrace (PTRACE_POKEUSR_AREA, tid, (long) &parea) < 0)
    perror_with_name (_("Couldn't write registers"));
}

/* Fetch all floating-point registers from process/thread TID and store
   their values in GDB's register cache.  */
static void
fetch_fpregs (struct regcache *regcache, int tid)
{
  fpregset_t fpregs;
  ptrace_area parea;

  parea.len = sizeof (fpregs);
  parea.process_addr = (addr_t) &fpregs;
  parea.kernel_addr = offsetof (struct user_regs_struct, fp_regs);
  if (ptrace (PTRACE_PEEKUSR_AREA, tid, (long) &parea) < 0)
    perror_with_name (_("Couldn't get floating point status"));

  supply_fpregset (regcache, (const fpregset_t *) &fpregs);
}

/* Store all valid floating-point registers in GDB's register cache
   into the process/thread specified by TID.  */
static void
store_fpregs (const struct regcache *regcache, int tid, int regnum)
{
  fpregset_t fpregs;
  ptrace_area parea;

  parea.len = sizeof (fpregs);
  parea.process_addr = (addr_t) &fpregs;
  parea.kernel_addr = offsetof (struct user_regs_struct, fp_regs);
  if (ptrace (PTRACE_PEEKUSR_AREA, tid, (long) &parea) < 0)
    perror_with_name (_("Couldn't get floating point status"));

  fill_fpregset (regcache, &fpregs, regnum);

  if (ptrace (PTRACE_POKEUSR_AREA, tid, (long) &parea) < 0)
    perror_with_name (_("Couldn't write floating point status"));
}

/* Fetch all registers in the kernel's register set whose number is REGSET,
   whose size is REGSIZE, and whose layout is described by REGMAP, from
   process/thread TID and store their values in GDB's register cache.  */
static void
fetch_regset (struct regcache *regcache, int tid,
	      int regset, int regsize, int *regmap)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  gdb_byte *buf = alloca (regsize);
  struct iovec iov;
  int i;

  iov.iov_base = buf;
  iov.iov_len = regsize;

  if (ptrace (PTRACE_GETREGSET, tid, (long) regset, (long) &iov) < 0)
    perror_with_name (_("Couldn't get register set"));

  for (i = 0; i < S390_NUM_REGS; i++)
    s390_native_supply (regcache, i, buf, regmap);
}

/* Store all registers in the kernel's register set whose number is REGSET,
   whose size is REGSIZE, and whose layout is described by REGMAP, from
   GDB's register cache back to process/thread TID.  */
static void
store_regset (struct regcache *regcache, int tid,
	      int regset, int regsize, int *regmap)
{
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  gdb_byte *buf = alloca (regsize);
  struct iovec iov;
  int i;

  iov.iov_base = buf;
  iov.iov_len = regsize;

  if (ptrace (PTRACE_GETREGSET, tid, (long) regset, (long) &iov) < 0)
    perror_with_name (_("Couldn't get register set"));

  for (i = 0; i < S390_NUM_REGS; i++)
    s390_native_collect (regcache, i, buf, regmap);

  if (ptrace (PTRACE_SETREGSET, tid, (long) regset, (long) &iov) < 0)
    perror_with_name (_("Couldn't set register set"));
}

/* Check whether the kernel provides a register set with number REGSET
   of size REGSIZE for process/thread TID.  */
static int
check_regset (int tid, int regset, int regsize)
{
  gdb_byte *buf = alloca (regsize);
  struct iovec iov;

  iov.iov_base = buf;
  iov.iov_len = regsize;

  if (ptrace (PTRACE_GETREGSET, tid, (long) regset, (long) &iov) < 0)
    return 0;
  else
    return 1;
}

/* Fetch register REGNUM from the child process.  If REGNUM is -1, do
   this for all registers.  */
static void
s390_linux_fetch_inferior_registers (struct target_ops *ops,
				     struct regcache *regcache, int regnum)
{
  int tid = s390_inferior_tid ();

  if (regnum == -1 
      || (regnum < S390_NUM_REGS && regmap_gregset[regnum] != -1))
    fetch_regs (regcache, tid);

  if (regnum == -1 
      || (regnum < S390_NUM_REGS && regmap_fpregset[regnum] != -1))
    fetch_fpregs (regcache, tid);

  if (have_regset_last_break)
    if (regnum == -1 || regnum == S390_LAST_BREAK_REGNUM)
      fetch_regset (regcache, tid, NT_S390_LAST_BREAK, 8,
		    (gdbarch_ptr_bit (get_regcache_arch (regcache)) == 32
		     ? s390_regmap_last_break : s390x_regmap_last_break));

  if (have_regset_system_call)
    if (regnum == -1 || regnum == S390_SYSTEM_CALL_REGNUM)
      fetch_regset (regcache, tid, NT_S390_SYSTEM_CALL, 4,
		    s390_regmap_system_call);
}

/* Store register REGNUM back into the child process.  If REGNUM is
   -1, do this for all registers.  */
static void
s390_linux_store_inferior_registers (struct target_ops *ops,
				     struct regcache *regcache, int regnum)
{
  int tid = s390_inferior_tid ();

  if (regnum == -1 
      || (regnum < S390_NUM_REGS && regmap_gregset[regnum] != -1))
    store_regs (regcache, tid, regnum);

  if (regnum == -1 
      || (regnum < S390_NUM_REGS && regmap_fpregset[regnum] != -1))
    store_fpregs (regcache, tid, regnum);

  /* S390_LAST_BREAK_REGNUM is read-only.  */

  if (have_regset_system_call)
    if (regnum == -1 || regnum == S390_SYSTEM_CALL_REGNUM)
      store_regset (regcache, tid, NT_S390_SYSTEM_CALL, 4,
		    s390_regmap_system_call);
}


/* Hardware-assisted watchpoint handling.  */

/* We maintain a list of all currently active watchpoints in order
   to properly handle watchpoint removal.

   The only thing we actually need is the total address space area
   spanned by the watchpoints.  */

struct watch_area
{
  struct watch_area *next;
  CORE_ADDR lo_addr;
  CORE_ADDR hi_addr;
};

static struct watch_area *watch_base = NULL;

static int
s390_stopped_by_watchpoint (void)
{
  per_lowcore_bits per_lowcore;
  ptrace_area parea;
  int result;

  /* Speed up common case.  */
  if (!watch_base)
    return 0;

  parea.len = sizeof (per_lowcore);
  parea.process_addr = (addr_t) & per_lowcore;
  parea.kernel_addr = offsetof (struct user_regs_struct, per_info.lowcore);
  if (ptrace (PTRACE_PEEKUSR_AREA, s390_inferior_tid (), &parea) < 0)
    perror_with_name (_("Couldn't retrieve watchpoint status"));

  result = (per_lowcore.perc_storage_alteration == 1
	    && per_lowcore.perc_store_real_address == 0);

  if (result)
    {
      /* Do not report this watchpoint again.  */
      memset (&per_lowcore, 0, sizeof (per_lowcore));
      if (ptrace (PTRACE_POKEUSR_AREA, s390_inferior_tid (), &parea) < 0)
	perror_with_name (_("Couldn't clear watchpoint status"));
    }

  return result;
}

static void
s390_fix_watch_points (struct lwp_info *lp)
{
  int tid;

  per_struct per_info;
  ptrace_area parea;

  CORE_ADDR watch_lo_addr = (CORE_ADDR)-1, watch_hi_addr = 0;
  struct watch_area *area;

  tid = TIDGET (lp->ptid);
  if (tid == 0)
    tid = PIDGET (lp->ptid);

  for (area = watch_base; area; area = area->next)
    {
      watch_lo_addr = min (watch_lo_addr, area->lo_addr);
      watch_hi_addr = max (watch_hi_addr, area->hi_addr);
    }

  parea.len = sizeof (per_info);
  parea.process_addr = (addr_t) & per_info;
  parea.kernel_addr = offsetof (struct user_regs_struct, per_info);
  if (ptrace (PTRACE_PEEKUSR_AREA, tid, &parea) < 0)
    perror_with_name (_("Couldn't retrieve watchpoint status"));

  if (watch_base)
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

  if (ptrace (PTRACE_POKEUSR_AREA, tid, &parea) < 0)
    perror_with_name (_("Couldn't modify watchpoint status"));
}

static int
s390_insert_watchpoint (CORE_ADDR addr, int len, int type,
			struct expression *cond)
{
  struct lwp_info *lp;
  struct watch_area *area = xmalloc (sizeof (struct watch_area));

  if (!area)
    return -1; 

  area->lo_addr = addr;
  area->hi_addr = addr + len - 1;
 
  area->next = watch_base;
  watch_base = area;

  ALL_LWPS (lp)
    s390_fix_watch_points (lp);
  return 0;
}

static int
s390_remove_watchpoint (CORE_ADDR addr, int len, int type,
			struct expression *cond)
{
  struct lwp_info *lp;
  struct watch_area *area, **parea;

  for (parea = &watch_base; *parea; parea = &(*parea)->next)
    if ((*parea)->lo_addr == addr
	&& (*parea)->hi_addr == addr + len - 1)
      break;

  if (!*parea)
    {
      fprintf_unfiltered (gdb_stderr,
			  "Attempt to remove nonexistent watchpoint.\n");
      return -1;
    }

  area = *parea;
  *parea = area->next;
  xfree (area);

  ALL_LWPS (lp)
    s390_fix_watch_points (lp);
  return 0;
}

static int
s390_can_use_hw_breakpoint (int type, int cnt, int othertype)
{
  return type == bp_hardware_watchpoint;
}

static int
s390_region_ok_for_hw_watchpoint (CORE_ADDR addr, int cnt)
{
  return 1;
}

static int
s390_target_wordsize (void)
{
  int wordsize = 4;

  /* Check for 64-bit inferior process.  This is the case when the host is
     64-bit, and in addition bit 32 of the PSW mask is set.  */
#ifdef __s390x__
  long pswm;

  errno = 0;
  pswm = (long) ptrace (PTRACE_PEEKUSER, s390_inferior_tid (), PT_PSWMASK, 0);
  if (errno == 0 && (pswm & 0x100000000ul) != 0)
    wordsize = 8;
#endif

  return wordsize;
}

static int
s390_auxv_parse (struct target_ops *ops, gdb_byte **readptr,
		 gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp)
{
  int sizeof_auxv_field = s390_target_wordsize ();
  enum bfd_endian byte_order = gdbarch_byte_order (target_gdbarch ());
  gdb_byte *ptr = *readptr;

  if (endptr == ptr)
    return 0;

  if (endptr - ptr < sizeof_auxv_field * 2)
    return -1;

  *typep = extract_unsigned_integer (ptr, sizeof_auxv_field, byte_order);
  ptr += sizeof_auxv_field;
  *valp = extract_unsigned_integer (ptr, sizeof_auxv_field, byte_order);
  ptr += sizeof_auxv_field;

  *readptr = ptr;
  return 1;
}

#ifdef __s390x__
static unsigned long
s390_get_hwcap (void)
{
  CORE_ADDR field;

  if (target_auxv_search (&current_target, AT_HWCAP, &field))
    return (unsigned long) field;

  return 0;
}
#endif

static const struct target_desc *
s390_read_description (struct target_ops *ops)
{
  int tid = s390_inferior_tid ();

  have_regset_last_break
    = check_regset (tid, NT_S390_LAST_BREAK, 8);
  have_regset_system_call
    = check_regset (tid, NT_S390_SYSTEM_CALL, 4);

#ifdef __s390x__
  /* If GDB itself is compiled as 64-bit, we are running on a machine in
     z/Architecture mode.  If the target is running in 64-bit addressing
     mode, report s390x architecture.  If the target is running in 31-bit
     addressing mode, but the kernel supports using 64-bit registers in
     that mode, report s390 architecture with 64-bit GPRs.  */

  if (s390_target_wordsize () == 8)
    return (have_regset_system_call? tdesc_s390x_linux64v2 :
	    have_regset_last_break? tdesc_s390x_linux64v1 :
	    tdesc_s390x_linux64);

  if (s390_get_hwcap () & HWCAP_S390_HIGH_GPRS)
    return (have_regset_system_call? tdesc_s390_linux64v2 :
	    have_regset_last_break? tdesc_s390_linux64v1 :
	    tdesc_s390_linux64);
#endif

  /* If GDB itself is compiled as 31-bit, or if we're running a 31-bit inferior
     on a 64-bit kernel that does not support using 64-bit registers in 31-bit
     mode, report s390 architecture with 32-bit GPRs.  */
  return (have_regset_system_call? tdesc_s390_linux32v2 :
	  have_regset_last_break? tdesc_s390_linux32v1 :
	  tdesc_s390_linux32);
}

void _initialize_s390_nat (void);

void
_initialize_s390_nat (void)
{
  struct target_ops *t;

  /* Fill in the generic GNU/Linux methods.  */
  t = linux_target ();

  /* Add our register access methods.  */
  t->to_fetch_registers = s390_linux_fetch_inferior_registers;
  t->to_store_registers = s390_linux_store_inferior_registers;

  /* Add our watchpoint methods.  */
  t->to_can_use_hw_breakpoint = s390_can_use_hw_breakpoint;
  t->to_region_ok_for_hw_watchpoint = s390_region_ok_for_hw_watchpoint;
  t->to_have_continuable_watchpoint = 1;
  t->to_stopped_by_watchpoint = s390_stopped_by_watchpoint;
  t->to_insert_watchpoint = s390_insert_watchpoint;
  t->to_remove_watchpoint = s390_remove_watchpoint;

  /* Detect target architecture.  */
  t->to_read_description = s390_read_description;
  t->to_auxv_parse = s390_auxv_parse;

  /* Register the target.  */
  linux_nat_add_target (t);
  linux_nat_set_new_thread (t, s390_fix_watch_points);
}
