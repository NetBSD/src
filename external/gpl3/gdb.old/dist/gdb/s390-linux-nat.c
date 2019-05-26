/* S390 native-dependent code for GDB, the GNU debugger.
   Copyright (C) 2001-2017 Free Software Foundation, Inc.

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
#include "gdbcmd.h"

#include "s390-linux-tdep.h"
#include "elf/common.h"

#include <asm/ptrace.h>
#include "nat/gdb_ptrace.h"
#include <asm/types.h>
#include <sys/procfs.h>
#include <sys/ucontext.h>
#include <elf.h>
#include <algorithm>
#include "inf-ptrace.h"

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

/* PER-event mask bits and PER control bits (CR9).  */

#define PER_BIT(n)			(1UL << (63 - (n)))
#define PER_EVENT_BRANCH		PER_BIT (32)
#define PER_EVENT_IFETCH		PER_BIT (33)
#define PER_EVENT_STORE			PER_BIT (34)
#define PER_EVENT_NULLIFICATION		PER_BIT (39)
#define PER_CONTROL_BRANCH_ADDRESS	PER_BIT (40)
#define PER_CONTROL_SUSPENSION		PER_BIT (41)
#define PER_CONTROL_ALTERATION		PER_BIT (42)


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
  pid_t tid = get_ptrace_pid (regcache_get_ptid (regcache));

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
  pid_t tid = get_ptrace_pid (regcache_get_ptid (regcache));

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

/* For each process we maintain a list of all currently active
   watchpoints, in order to properly handle watchpoint removal.

   The only thing we actually need is the total address space area
   spanned by the watchpoints.  */

typedef struct watch_area
{
  CORE_ADDR lo_addr;
  CORE_ADDR hi_addr;
} s390_watch_area;

DEF_VEC_O (s390_watch_area);

/* Hardware debug state.  */

struct s390_debug_reg_state
{
  VEC_s390_watch_area *watch_areas;
  VEC_s390_watch_area *break_areas;
};

/* Per-process data.  */

struct s390_process_info
{
  struct s390_process_info *next;
  pid_t pid;
  struct s390_debug_reg_state state;
};

static struct s390_process_info *s390_process_list = NULL;

/* Find process data for process PID.  */

static struct s390_process_info *
s390_find_process_pid (pid_t pid)
{
  struct s390_process_info *proc;

  for (proc = s390_process_list; proc; proc = proc->next)
    if (proc->pid == pid)
      return proc;

  return NULL;
}

/* Add process data for process PID.  Returns newly allocated info
   object.  */

static struct s390_process_info *
s390_add_process (pid_t pid)
{
  struct s390_process_info *proc = XCNEW (struct s390_process_info);

  proc->pid = pid;
  proc->next = s390_process_list;
  s390_process_list = proc;

  return proc;
}

/* Get data specific info for process PID, creating it if necessary.
   Never returns NULL.  */

static struct s390_process_info *
s390_process_info_get (pid_t pid)
{
  struct s390_process_info *proc;

  proc = s390_find_process_pid (pid);
  if (proc == NULL)
    proc = s390_add_process (pid);

  return proc;
}

/* Get hardware debug state for process PID.  */

static struct s390_debug_reg_state *
s390_get_debug_reg_state (pid_t pid)
{
  return &s390_process_info_get (pid)->state;
}

/* Called whenever GDB is no longer debugging process PID.  It deletes
   data structures that keep track of hardware debug state.  */

static void
s390_forget_process (pid_t pid)
{
  struct s390_process_info *proc, **proc_link;

  proc = s390_process_list;
  proc_link = &s390_process_list;

  while (proc != NULL)
    {
      if (proc->pid == pid)
	{
	  VEC_free (s390_watch_area, proc->state.watch_areas);
	  VEC_free (s390_watch_area, proc->state.break_areas);
	  *proc_link = proc->next;
	  xfree (proc);
	  return;
	}

      proc_link = &proc->next;
      proc = *proc_link;
    }
}

/* linux_nat_new_fork hook.   */

static void
s390_linux_new_fork (struct lwp_info *parent, pid_t child_pid)
{
  pid_t parent_pid;
  struct s390_debug_reg_state *parent_state;
  struct s390_debug_reg_state *child_state;

  /* NULL means no watchpoint has ever been set in the parent.  In
     that case, there's nothing to do.  */
  if (lwp_arch_private_info (parent) == NULL)
    return;

  /* GDB core assumes the child inherits the watchpoints/hw breakpoints of
     the parent.  So copy the debug state from parent to child.  */

  parent_pid = ptid_get_pid (parent->ptid);
  parent_state = s390_get_debug_reg_state (parent_pid);
  child_state = s390_get_debug_reg_state (child_pid);

  child_state->watch_areas = VEC_copy (s390_watch_area,
				       parent_state->watch_areas);
  child_state->break_areas = VEC_copy (s390_watch_area,
				       parent_state->break_areas);
}

/* Dump PER state.  */

static void
s390_show_debug_regs (int tid, const char *where)
{
  per_struct per_info;
  ptrace_area parea;

  parea.len = sizeof (per_info);
  parea.process_addr = (addr_t) &per_info;
  parea.kernel_addr = offsetof (struct user_regs_struct, per_info);

  if (ptrace (PTRACE_PEEKUSR_AREA, tid, &parea, 0) < 0)
    perror_with_name (_("Couldn't retrieve debug regs"));

  debug_printf ("PER (debug) state for %d -- %s\n"
		"  cr9-11: %lx %lx %lx\n"
		"  start, end: %lx %lx\n"
		"  code/ATMID: %x  address: %lx  PAID: %x\n",
		tid,
		where,
		per_info.control_regs.words.cr[0],
		per_info.control_regs.words.cr[1],
		per_info.control_regs.words.cr[2],
		per_info.starting_addr,
		per_info.ending_addr,
		per_info.lowcore.words.perc_atmid,
		per_info.lowcore.words.address,
		per_info.lowcore.words.access_id);
}

static int
s390_stopped_by_watchpoint (struct target_ops *ops)
{
  struct s390_debug_reg_state *state
    = s390_get_debug_reg_state (ptid_get_pid (inferior_ptid));
  per_lowcore_bits per_lowcore;
  ptrace_area parea;
  int result;

  if (show_debug_regs)
    s390_show_debug_regs (s390_inferior_tid (), "stop");

  /* Speed up common case.  */
  if (VEC_empty (s390_watch_area, state->watch_areas))
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
  pid_t pid = ptid_get_pid (ptid_of_lwp (lp));

  per_struct per_info;
  ptrace_area parea;

  CORE_ADDR watch_lo_addr = (CORE_ADDR)-1, watch_hi_addr = 0;
  unsigned ix;
  s390_watch_area *area;
  struct arch_lwp_info *lp_priv = lwp_arch_private_info (lp);
  struct s390_debug_reg_state *state = s390_get_debug_reg_state (pid);
  int step = lwp_is_stepping (lp);

  /* Nothing to do if there was never any PER info for this thread.  */
  if (lp_priv == NULL)
    return;

  /* If PER info has changed, update it.  When single-stepping, disable
     hardware breakpoints (if any).  Otherwise we're done.  */
  if (!lp_priv->per_info_changed)
    {
      if (!step || VEC_empty (s390_watch_area, state->break_areas))
	return;
    }

  lp_priv->per_info_changed = 0;

  tid = ptid_get_lwp (ptid_of_lwp (lp));
  if (tid == 0)
    tid = pid;

  parea.len = sizeof (per_info);
  parea.process_addr = (addr_t) & per_info;
  parea.kernel_addr = offsetof (struct user_regs_struct, per_info);

  /* Clear PER info, but adjust the single_step field (used by older
     kernels only).  */
  memset (&per_info, 0, sizeof (per_info));
  per_info.single_step = (step != 0);

  if (!VEC_empty (s390_watch_area, state->watch_areas))
    {
      for (ix = 0;
	   VEC_iterate (s390_watch_area, state->watch_areas, ix, area);
	   ix++)
	{
	  watch_lo_addr = std::min (watch_lo_addr, area->lo_addr);
	  watch_hi_addr = std::max (watch_hi_addr, area->hi_addr);
	}

      /* Enable storage-alteration events.  */
      per_info.control_regs.words.cr[0] |= (PER_EVENT_STORE
					    | PER_CONTROL_ALTERATION);
    }

  if (!VEC_empty (s390_watch_area, state->break_areas))
    {
      /* Don't install hardware breakpoints while single-stepping, since
	 our PER settings (e.g. the nullification bit) might then conflict
	 with the kernel's.  But re-install them afterwards.  */
      if (step)
	lp_priv->per_info_changed = 1;
      else
	{
	  for (ix = 0;
	       VEC_iterate (s390_watch_area, state->break_areas, ix, area);
	       ix++)
	    {
	      watch_lo_addr = std::min (watch_lo_addr, area->lo_addr);
	      watch_hi_addr = std::max (watch_hi_addr, area->hi_addr);
	    }

	  /* If there's just one breakpoint, enable instruction-fetching
	     nullification events for the breakpoint address (fast).
	     Otherwise stop after any instruction within the PER area and
	     after any branch into it (slow).  */
	  if (watch_hi_addr == watch_lo_addr)
	    per_info.control_regs.words.cr[0] |= (PER_EVENT_NULLIFICATION
						  | PER_EVENT_IFETCH);
	  else
	    {
	      /* The PER area must include the instruction before the
		 first breakpoint address.  */
	      watch_lo_addr = watch_lo_addr > 6 ? watch_lo_addr - 6 : 0;
	      per_info.control_regs.words.cr[0]
		|= (PER_EVENT_BRANCH
		    | PER_EVENT_IFETCH
		    | PER_CONTROL_BRANCH_ADDRESS);
	    }
	}
    }
  per_info.starting_addr = watch_lo_addr;
  per_info.ending_addr = watch_hi_addr;

  if (ptrace (PTRACE_POKEUSR_AREA, tid, &parea, 0) < 0)
    perror_with_name (_("Couldn't modify watchpoint status"));

  if (show_debug_regs)
    s390_show_debug_regs (tid, "resume");
}

/* Mark the PER info as changed, so the next resume will update it.  */

static void
s390_mark_per_info_changed (struct lwp_info *lp)
{
  if (lwp_arch_private_info (lp) == NULL)
    lwp_set_arch_private_info (lp, XCNEW (struct arch_lwp_info));

  lwp_arch_private_info (lp)->per_info_changed = 1;
}

/* When attaching to a new thread, mark its PER info as changed.  */

static void
s390_new_thread (struct lwp_info *lp)
{
  s390_mark_per_info_changed (lp);
}

/* Iterator callback for s390_refresh_per_info.  */

static int
s390_refresh_per_info_cb (struct lwp_info *lp, void *arg)
{
  s390_mark_per_info_changed (lp);

  if (!lwp_is_stopped (lp))
    linux_stop_lwp (lp);
  return 0;
}

/* Make sure that threads are stopped and mark PER info as changed.  */

static int
s390_refresh_per_info (void)
{
  ptid_t pid_ptid = pid_to_ptid (ptid_get_pid (current_lwp_ptid ()));

  iterate_over_lwps (pid_ptid, s390_refresh_per_info_cb, NULL);
  return 0;
}

static int
s390_insert_watchpoint (struct target_ops *self,
			CORE_ADDR addr, int len, enum target_hw_bp_type type,
			struct expression *cond)
{
  s390_watch_area area;
  struct s390_debug_reg_state *state
    = s390_get_debug_reg_state (ptid_get_pid (inferior_ptid));

  area.lo_addr = addr;
  area.hi_addr = addr + len - 1;
  VEC_safe_push (s390_watch_area, state->watch_areas, &area);

  return s390_refresh_per_info ();
}

static int
s390_remove_watchpoint (struct target_ops *self,
			CORE_ADDR addr, int len, enum target_hw_bp_type type,
			struct expression *cond)
{
  unsigned ix;
  s390_watch_area *area;
  struct s390_debug_reg_state *state
    = s390_get_debug_reg_state (ptid_get_pid (inferior_ptid));

  for (ix = 0;
       VEC_iterate (s390_watch_area, state->watch_areas, ix, area);
       ix++)
    {
      if (area->lo_addr == addr && area->hi_addr == addr + len - 1)
	{
	  VEC_unordered_remove (s390_watch_area, state->watch_areas, ix);
	  return s390_refresh_per_info ();
	}
    }

  fprintf_unfiltered (gdb_stderr,
		      "Attempt to remove nonexistent watchpoint.\n");
  return -1;
}

/* Implement the "can_use_hw_breakpoint" target_ops method. */

static int
s390_can_use_hw_breakpoint (struct target_ops *self,
			    enum bptype type, int cnt, int othertype)
{
  if (type == bp_hardware_watchpoint || type == bp_hardware_breakpoint)
    return 1;
  return 0;
}

/* Implement the "insert_hw_breakpoint" target_ops method.  */

static int
s390_insert_hw_breakpoint (struct target_ops *self,
			   struct gdbarch *gdbarch,
			   struct bp_target_info *bp_tgt)
{
  s390_watch_area area;
  struct s390_debug_reg_state *state;

  area.lo_addr = bp_tgt->placed_address = bp_tgt->reqstd_address;
  area.hi_addr = area.lo_addr;
  state = s390_get_debug_reg_state (ptid_get_pid (inferior_ptid));
  VEC_safe_push (s390_watch_area, state->break_areas, &area);

  return s390_refresh_per_info ();
}

/* Implement the "remove_hw_breakpoint" target_ops method.  */

static int
s390_remove_hw_breakpoint (struct target_ops *self,
			   struct gdbarch *gdbarch,
			   struct bp_target_info *bp_tgt)
{
  unsigned ix;
  struct watch_area *area;
  struct s390_debug_reg_state *state;

  state = s390_get_debug_reg_state (ptid_get_pid (inferior_ptid));
  for (ix = 0;
       VEC_iterate (s390_watch_area, state->break_areas, ix, area);
       ix++)
    {
      if (area->lo_addr == bp_tgt->placed_address)
	{
	  VEC_unordered_remove (s390_watch_area, state->break_areas, ix);
	  return s390_refresh_per_info ();
	}
    }

  fprintf_unfiltered (gdb_stderr,
		      "Attempt to remove nonexistent breakpoint.\n");
  return -1;
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
  t->to_insert_hw_breakpoint = s390_insert_hw_breakpoint;
  t->to_remove_hw_breakpoint = s390_remove_hw_breakpoint;
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
  linux_nat_set_forget_process (t, s390_forget_process);
  linux_nat_set_new_fork (t, s390_linux_new_fork);

  /* A maintenance command to enable showing the PER state.  */
  add_setshow_boolean_cmd ("show-debug-regs", class_maintenance,
			   &show_debug_regs, _("\
Set whether to show the PER (debug) hardware state."), _("\
Show whether to show the PER (debug) hardware state."), _("\
Use \"on\" to enable, \"off\" to disable.\n\
If enabled, the PER state is shown after it is changed by GDB,\n\
and when the inferior triggers a breakpoint or watchpoint."),
			   NULL,
			   NULL,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);
}
