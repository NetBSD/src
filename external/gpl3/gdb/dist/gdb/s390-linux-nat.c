/* S390 native-dependent code for GDB, the GNU debugger.
   Copyright (C) 2001-2016 Free Software Foundation, Inc.

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
#include "regset.h"
#include "nat/linux-ptrace.h"

#include "s390-linux-tdep.h"
#include "elf/common.h"

#include <asm/ptrace.h>
#include "nat/gdb_ptrace.h"
#include <asm/types.h>
#include <sys/procfs.h>
#include <sys/ucontext.h>
#include <elf.h>

/* Per-thread arch-specific data.  */

struct arch_lwp_info
{
  /* Non-zero if the thread's PER info must be re-written.  */
  int per_info_changed;
};

static int have_regset_last_break = 0;
static int have_regset_system_call = 0;
static int have_regset_tdb = 0;
static int have_regset_vxrs = 0;

/* Register map for 32-bit executables running under a 64-bit
   kernel.  */

#ifdef __s390x__
static const struct regcache_map_entry s390_64_regmap_gregset[] =
  {
    /* Skip PSWM and PSWA, since they must be handled specially.  */
    { 2, REGCACHE_MAP_SKIP, 8 },
    { 1, S390_R0_UPPER_REGNUM, 4 }, { 1, S390_R0_REGNUM, 4 },
    { 1, S390_R1_UPPER_REGNUM, 4 }, { 1, S390_R1_REGNUM, 4 },
    { 1, S390_R2_UPPER_REGNUM, 4 }, { 1, S390_R2_REGNUM, 4 },
    { 1, S390_R3_UPPER_REGNUM, 4 }, { 1, S390_R3_REGNUM, 4 },
    { 1, S390_R4_UPPER_REGNUM, 4 }, { 1, S390_R4_REGNUM, 4 },
    { 1, S390_R5_UPPER_REGNUM, 4 }, { 1, S390_R5_REGNUM, 4 },
    { 1, S390_R6_UPPER_REGNUM, 4 }, { 1, S390_R6_REGNUM, 4 },
    { 1, S390_R7_UPPER_REGNUM, 4 }, { 1, S390_R7_REGNUM, 4 },
    { 1, S390_R8_UPPER_REGNUM, 4 }, { 1, S390_R8_REGNUM, 4 },
    { 1, S390_R9_UPPER_REGNUM, 4 }, { 1, S390_R9_REGNUM, 4 },
    { 1, S390_R10_UPPER_REGNUM, 4 }, { 1, S390_R10_REGNUM, 4 },
    { 1, S390_R11_UPPER_REGNUM, 4 }, { 1, S390_R11_REGNUM, 4 },
    { 1, S390_R12_UPPER_REGNUM, 4 }, { 1, S390_R12_REGNUM, 4 },
    { 1, S390_R13_UPPER_REGNUM, 4 }, { 1, S390_R13_REGNUM, 4 },
    { 1, S390_R14_UPPER_REGNUM, 4 }, { 1, S390_R14_REGNUM, 4 },
    { 1, S390_R15_UPPER_REGNUM, 4 }, { 1, S390_R15_REGNUM, 4 },
    { 16, S390_A0_REGNUM, 4 },
    { 1, REGCACHE_MAP_SKIP, 4 }, { 1, S390_ORIG_R2_REGNUM, 4 },
    { 0 }
  };

static const struct regset s390_64_gregset =
  {
    s390_64_regmap_gregset,
    regcache_supply_regset,
    regcache_collect_regset
  };

#define S390_PSWM_OFFSET 0
#define S390_PSWA_OFFSET 8
#endif

/* Fill GDB's register array with the general-purpose register values
   in *REGP.

   When debugging a 32-bit executable running under a 64-bit kernel,
   we have to fix up the 64-bit registers we get from the kernel to
   make them look like 32-bit registers.  */

void
supply_gregset (struct regcache *regcache, const gregset_t *regp)
{
#ifdef __s390x__
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (gdbarch_ptr_bit (gdbarch) == 32)
    {
      enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
      ULONGEST pswm, pswa;
      gdb_byte buf[4];

      regcache_supply_regset (&s390_64_gregset, regcache, -1,
			      regp, sizeof (gregset_t));
      pswm = extract_unsigned_integer ((const gdb_byte *) regp
				       + S390_PSWM_OFFSET, 8, byte_order);
      pswa = extract_unsigned_integer ((const gdb_byte *) regp
				       + S390_PSWA_OFFSET, 8, byte_order);
      store_unsigned_integer (buf, 4, byte_order, (pswm >> 32) | 0x80000);
      regcache_raw_supply (regcache, S390_PSWM_REGNUM, buf);
      store_unsigned_integer (buf, 4, byte_order,
			      (pswa & 0x7fffffff) | (pswm & 0x80000000));
      regcache_raw_supply (regcache, S390_PSWA_REGNUM, buf);
      return;
    }
#endif

  regcache_supply_regset (&s390_gregset, regcache, -1, regp,
			  sizeof (gregset_t));
}

/* Fill register REGNO (if it is a general-purpose register) in
   *REGP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */

void
fill_gregset (const struct regcache *regcache, gregset_t *regp, int regno)
{
#ifdef __s390x__
  struct gdbarch *gdbarch = get_regcache_arch (regcache);
  if (gdbarch_ptr_bit (gdbarch) == 32)
    {
      regcache_collect_regset (&s390_64_gregset, regcache, regno,
			       regp, sizeof (gregset_t));

      if (regno == -1
	  || regno == S390_PSWM_REGNUM || regno == S390_PSWA_REGNUM)
	{
	  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
	  ULONGEST pswa, pswm;
	  gdb_byte buf[4];
	  gdb_byte *pswm_p = (gdb_byte *) regp + S390_PSWM_OFFSET;
	  gdb_byte *pswa_p = (gdb_byte *) regp + S390_PSWA_OFFSET;

	  pswm = extract_unsigned_integer (pswm_p, 8, byte_order);

	  if (regno == -1 || regno == S390_PSWM_REGNUM)
	    {
	      pswm &= 0x80000000;
	      regcache_raw_collect (regcache, S390_PSWM_REGNUM, buf);
	      pswm |= (extract_unsigned_integer (buf, 4, byte_order)
		       & 0xfff7ffff) << 32;
	    }

	  if (regno == -1 || regno == S390_PSWA_REGNUM)
	    {
	      regcache_raw_collect (regcache, S390_PSWA_REGNUM, buf);
	      pswa = extract_unsigned_integer (buf, 4, byte_order);
	      pswm ^= (pswm ^ pswa) & 0x80000000;
	      pswa &= 0x7fffffff;
	      store_unsigned_integer (pswa_p, 8, byte_order, pswa);
	    }

	  store_unsigned_integer (pswm_p, 8, byte_order, pswm);
	}
      return;
    }
#endif

  regcache_collect_regset (&s390_gregset, regcache, regno, regp,
			   sizeof (gregset_t));
}

/* Fill GDB's register array with the floating-point register values
   in *REGP.  */
void
supply_fpregset (struct regcache *regcache, const fpregset_t *regp)
{
  regcache_supply_regset (&s390_fpregset, regcache, -1, regp,
			  sizeof (fpregset_t));
}

/* Fill register REGNO (if it is a general-purpose register) in
   *REGP with the value in GDB's register array.  If REGNO is -1,
   do this for all registers.  */
void
fill_fpregset (const struct regcache *regcache, fpregset_t *regp, int regno)
{
  regcache_collect_regset (&s390_fpregset, regcache, regno, regp,
			   sizeof (fpregset_t));
}

/* Find the TID for the current inferior thread to use with ptrace.  */
static int
s390_inferior_tid (void)
{
  /* GNU/Linux LWP ID's are process ID's.  */
  int tid = ptid_get_lwp (inferior_ptid);
  if (tid == 0)
    tid = ptid_get_pid (inferior_ptid); /* Not a threaded program.  */

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
  if (ptrace (PTRACE_PEEKUSR_AREA, tid, (long) &parea, 0) < 0)
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
  if (ptrace (PTRACE_PEEKUSR_AREA, tid, (long) &parea, 0) < 0)
    perror_with_name (_("Couldn't get registers"));

  fill_gregset (regcache, &regs, regnum);

  if (ptrace (PTRACE_POKEUSR_AREA, tid, (long) &parea, 0) < 0)
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
  if (ptrace (PTRACE_PEEKUSR_AREA, tid, (long) &parea, 0) < 0)
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
  if (ptrace (PTRACE_PEEKUSR_AREA, tid, (long) &parea, 0) < 0)
    perror_with_name (_("Couldn't get floating point status"));

  fill_fpregset (regcache, &fpregs, regnum);

  if (ptrace (PTRACE_POKEUSR_AREA, tid, (long) &parea, 0) < 0)
    perror_with_name (_("Couldn't write floating point status"));
}

/* Fetch all registers in the kernel's register set whose number is
   REGSET_ID, whose size is REGSIZE, and whose layout is described by
   REGSET, from process/thread TID and store their values in GDB's
   register cache.  */
static void
fetch_regset (struct regcache *regcache, int tid,
	      int regset_id, int regsize, const struct regset *regset)
{
  void *buf = alloca (regsize);
  struct iovec iov;

  iov.iov_base = buf;
  iov.iov_len = regsize;

  if (ptrace (PTRACE_GETREGSET, tid, (long) regset_id, (long) &iov) < 0)
    {
      if (errno == ENODATA)
	regcache_supply_regset (regset, regcache, -1, NULL, regsize);
      else
	perror_with_name (_("Couldn't get register set"));
    }
  else
    regcache_supply_regset (regset, regcache, -1, buf, regsize);
}

/* Store all registers in the kernel's register set whose number is
   REGSET_ID, whose size is REGSIZE, and whose layout is described by
   REGSET, from GDB's register cache back to process/thread TID.  */
static void
store_regset (struct regcache *regcache, int tid,
	      int regset_id, int regsize, const struct regset *regset)
{
  void *buf = alloca (regsize);
  struct iovec iov;

  iov.iov_base = buf;
  iov.iov_len = regsize;

  if (ptrace (PTRACE_GETREGSET, tid, (long) regset_id, (long) &iov) < 0)
    perror_with_name (_("Couldn't get register set"));

  regcache_collect_regset (regset, regcache, -1, buf, regsize);

  if (ptrace (PTRACE_SETREGSET, tid, (long) regset_id, (long) &iov) < 0)
    perror_with_name (_("Couldn't set register set"));
}

/* Check whether the kernel provides a register set with number REGSET
   of size REGSIZE for process/thread TID.  */
static int
check_regset (int tid, int regset, int regsize)
{
  void *buf = alloca (regsize);
  struct iovec iov;

  iov.iov_base = buf;
  iov.iov_len = regsize;

  if (ptrace (PTRACE_GETREGSET, tid, (long) regset, (long) &iov) >= 0
      || errno == ENODATA)
    return 1;
  return 0;
}

/* Fetch register REGNUM from the child process.  If REGNUM is -1, do
   this for all registers.  */
static void
s390_linux_fetch_inferior_registers (struct target_ops *ops,
				     struct regcache *regcache, int regnum)
{
  int tid = s390_inferior_tid ();

  if (regnum == -1 || S390_IS_GREGSET_REGNUM (regnum))
    fetch_regs (regcache, tid);

  if (regnum == -1 || S390_IS_FPREGSET_REGNUM (regnum))
    fetch_fpregs (regcache, tid);

  if (have_regset_last_break)
    if (regnum == -1 || regnum == S390_LAST_BREAK_REGNUM)
      fetch_regset (regcache, tid, NT_S390_LAST_BREAK, 8,
		    (gdbarch_ptr_bit (get_regcache_arch (regcache)) == 32
		     ? &s390_last_break_regset : &s390x_last_break_regset));

  if (have_regset_system_call)
    if (regnum == -1 || regnum == S390_SYSTEM_CALL_REGNUM)
      fetch_regset (regcache, tid, NT_S390_SYSTEM_CALL, 4,
		    &s390_system_call_regset);

  if (have_regset_tdb)
    if (regnum == -1 || S390_IS_TDBREGSET_REGNUM (regnum))
      fetch_regset (regcache, tid, NT_S390_TDB, s390_sizeof_tdbregset,
		    &s390_tdb_regset);

  if (have_regset_vxrs)
    {
      if (regnum == -1 || (regnum >= S390_V0_LOWER_REGNUM
			   && regnum <= S390_V15_LOWER_REGNUM))
	fetch_regset (regcache, tid, NT_S390_VXRS_LOW, 16 * 8,
		      &s390_vxrs_low_regset);
      if (regnum == -1 || (regnum >= S390_V16_REGNUM
			   && regnum <= S390_V31_REGNUM))
	fetch_regset (regcache, tid, NT_S390_VXRS_HIGH, 16 * 16,
		      &s390_vxrs_high_regset);
    }
}

/* Store register REGNUM back into the child process.  If REGNUM is
   -1, do this for all registers.  */
static void
s390_linux_store_inferior_registers (struct target_ops *ops,
				     struct regcache *regcache, int regnum)
{
  int tid = s390_inferior_tid ();

  if (regnum == -1 || S390_IS_GREGSET_REGNUM (regnum))
    store_regs (regcache, tid, regnum);

  if (regnum == -1 || S390_IS_FPREGSET_REGNUM (regnum))
    store_fpregs (regcache, tid, regnum);

  /* S390_LAST_BREAK_REGNUM is read-only.  */

  if (have_regset_system_call)
    if (regnum == -1 || regnum == S390_SYSTEM_CALL_REGNUM)
      store_regset (regcache, tid, NT_S390_SYSTEM_CALL, 4,
		    &s390_system_call_regset);

  if (have_regset_vxrs)
    {
      if (regnum == -1 || (regnum >= S390_V0_LOWER_REGNUM
			   && regnum <= S390_V15_LOWER_REGNUM))
	store_regset (regcache, tid, NT_S390_VXRS_LOW, 16 * 8,
		      &s390_vxrs_low_regset);
      if (regnum == -1 || (regnum >= S390_V16_REGNUM
			   && regnum <= S390_V31_REGNUM))
	store_regset (regcache, tid, NT_S390_VXRS_HIGH, 16 * 16,
		      &s390_vxrs_high_regset);
    }
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
s390_stopped_by_watchpoint (struct target_ops *ops)
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
  if (ptrace (PTRACE_PEEKUSR_AREA, s390_inferior_tid (), &parea, 0) < 0)
    perror_with_name (_("Couldn't retrieve watchpoint status"));

  result = (per_lowcore.perc_storage_alteration == 1
	    && per_lowcore.perc_store_real_address == 0);

  if (result)
    {
      /* Do not report this watchpoint again.  */
      memset (&per_lowcore, 0, sizeof (per_lowcore));
      if (ptrace (PTRACE_POKEUSR_AREA, s390_inferior_tid (), &parea, 0) < 0)
	perror_with_name (_("Couldn't clear watchpoint status"));
    }

  return result;
}

/* Each time before resuming a thread, update its PER info.  */

static void
s390_prepare_to_resume (struct lwp_info *lp)
{
  int tid;

  per_struct per_info;
  ptrace_area parea;

  CORE_ADDR watch_lo_addr = (CORE_ADDR)-1, watch_hi_addr = 0;
  struct watch_area *area;

  if (lp->arch_private == NULL
      || !lp->arch_private->per_info_changed)
    return;

  lp->arch_private->per_info_changed = 0;

  tid = ptid_get_lwp (lp->ptid);
  if (tid == 0)
    tid = ptid_get_pid (lp->ptid);

  for (area = watch_base; area; area = area->next)
    {
      watch_lo_addr = min (watch_lo_addr, area->lo_addr);
      watch_hi_addr = max (watch_hi_addr, area->hi_addr);
    }

  parea.len = sizeof (per_info);
  parea.process_addr = (addr_t) & per_info;
  parea.kernel_addr = offsetof (struct user_regs_struct, per_info);
  if (ptrace (PTRACE_PEEKUSR_AREA, tid, &parea, 0) < 0)
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

  if (ptrace (PTRACE_POKEUSR_AREA, tid, &parea, 0) < 0)
    perror_with_name (_("Couldn't modify watchpoint status"));
}

/* Make sure that LP is stopped and mark its PER info as changed, so
   the next resume will update it.  */

static void
s390_refresh_per_info (struct lwp_info *lp)
{
  if (lp->arch_private == NULL)
    lp->arch_private = XCNEW (struct arch_lwp_info);

  lp->arch_private->per_info_changed = 1;

  if (!lp->stopped)
    linux_stop_lwp (lp);
}

/* When attaching to a new thread, mark its PER info as changed.  */

static void
s390_new_thread (struct lwp_info *lp)
{
  lp->arch_private = XCNEW (struct arch_lwp_info);
  lp->arch_private->per_info_changed = 1;
}

static int
s390_insert_watchpoint (struct target_ops *self,
			CORE_ADDR addr, int len, enum target_hw_bp_type type,
			struct expression *cond)
{
  struct lwp_info *lp;
  struct watch_area *area = XNEW (struct watch_area);

  if (!area)
    return -1;

  area->lo_addr = addr;
  area->hi_addr = addr + len - 1;

  area->next = watch_base;
  watch_base = area;

  ALL_LWPS (lp)
    s390_refresh_per_info (lp);
  return 0;
}

static int
s390_remove_watchpoint (struct target_ops *self,
			CORE_ADDR addr, int len, enum target_hw_bp_type type,
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
    s390_refresh_per_info (lp);
  return 0;
}

static int
s390_can_use_hw_breakpoint (struct target_ops *self,
			    enum bptype type, int cnt, int othertype)
{
  return type == bp_hardware_watchpoint;
}

static int
s390_region_ok_for_hw_watchpoint (struct target_ops *self,
				  CORE_ADDR addr, int cnt)
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

static const struct target_desc *
s390_read_description (struct target_ops *ops)
{
  int tid = s390_inferior_tid ();

  have_regset_last_break
    = check_regset (tid, NT_S390_LAST_BREAK, 8);
  have_regset_system_call
    = check_regset (tid, NT_S390_SYSTEM_CALL, 4);

  /* If GDB itself is compiled as 64-bit, we are running on a machine in
     z/Architecture mode.  If the target is running in 64-bit addressing
     mode, report s390x architecture.  If the target is running in 31-bit
     addressing mode, but the kernel supports using 64-bit registers in
     that mode, report s390 architecture with 64-bit GPRs.  */
#ifdef __s390x__
  {
    CORE_ADDR hwcap = 0;

    target_auxv_search (&current_target, AT_HWCAP, &hwcap);
    have_regset_tdb = (hwcap & HWCAP_S390_TE)
      && check_regset (tid, NT_S390_TDB, s390_sizeof_tdbregset);

    have_regset_vxrs = (hwcap & HWCAP_S390_VX)
      && check_regset (tid, NT_S390_VXRS_LOW, 16 * 8)
      && check_regset (tid, NT_S390_VXRS_HIGH, 16 * 16);

    if (s390_target_wordsize () == 8)
      return (have_regset_vxrs ?
	      (have_regset_tdb ? tdesc_s390x_tevx_linux64 :
	       tdesc_s390x_vx_linux64) :
	      have_regset_tdb ? tdesc_s390x_te_linux64 :
	      have_regset_system_call ? tdesc_s390x_linux64v2 :
	      have_regset_last_break ? tdesc_s390x_linux64v1 :
	      tdesc_s390x_linux64);

    if (hwcap & HWCAP_S390_HIGH_GPRS)
      return (have_regset_vxrs ?
	      (have_regset_tdb ? tdesc_s390_tevx_linux64 :
	       tdesc_s390_vx_linux64) :
	      have_regset_tdb ? tdesc_s390_te_linux64 :
	      have_regset_system_call ? tdesc_s390_linux64v2 :
	      have_regset_last_break ? tdesc_s390_linux64v1 :
	      tdesc_s390_linux64);
  }
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
  linux_nat_set_new_thread (t, s390_new_thread);
  linux_nat_set_prepare_to_resume (t, s390_prepare_to_resume);
}
