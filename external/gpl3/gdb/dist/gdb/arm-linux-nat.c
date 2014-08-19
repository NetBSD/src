/* GNU/Linux on ARM native support.
   Copyright (C) 1999-2014 Free Software Foundation, Inc.

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
#include <string.h>
#include "regcache.h"
#include "target.h"
#include "linux-nat.h"
#include "target-descriptions.h"
#include "auxv.h"
#include "observer.h"
#include "gdbthread.h"

#include "arm-tdep.h"
#include "arm-linux-tdep.h"

#include <elf/common.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <sys/utsname.h>
#include <sys/procfs.h>

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"

/* Defines ps_err_e, struct ps_prochandle.  */
#include "gdb_proc_service.h"

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 22
#endif

#ifndef PTRACE_GETWMMXREGS
#define PTRACE_GETWMMXREGS 18
#define PTRACE_SETWMMXREGS 19
#endif

#ifndef PTRACE_GETVFPREGS
#define PTRACE_GETVFPREGS 27
#define PTRACE_SETVFPREGS 28
#endif

#ifndef PTRACE_GETHBPREGS
#define PTRACE_GETHBPREGS 29
#define PTRACE_SETHBPREGS 30
#endif

/* A flag for whether the WMMX registers are available.  */
static int arm_linux_has_wmmx_registers;

/* The number of 64-bit VFP registers we have (expect this to be 0,
   16, or 32).  */
static int arm_linux_vfp_register_count;

extern int arm_apcs_32;

/* On GNU/Linux, threads are implemented as pseudo-processes, in which
   case we may be tracing more than one process at a time.  In that
   case, inferior_ptid will contain the main process ID and the
   individual thread (process) ID.  get_thread_id () is used to get
   the thread id if it's available, and the process id otherwise.  */

static int
get_thread_id (ptid_t ptid)
{
  int tid = ptid_get_lwp (ptid);
  if (0 == tid)
    tid = ptid_get_pid (ptid);
  return tid;
}

#define GET_THREAD_ID(PTID)	get_thread_id (PTID)

/* Get the value of a particular register from the floating point
   state of the process and store it into regcache.  */

static void
fetch_fpregister (struct regcache *regcache, int regno)
{
  int ret, tid;
  gdb_byte fp[ARM_LINUX_SIZEOF_NWFPE];
  
  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);

  /* Read the floating point state.  */
  ret = ptrace (PT_GETFPREGS, tid, 0, fp);
  if (ret < 0)
    {
      warning (_("Unable to fetch floating point register."));
      return;
    }

  /* Fetch fpsr.  */
  if (ARM_FPS_REGNUM == regno)
    regcache_raw_supply (regcache, ARM_FPS_REGNUM,
			 fp + NWFPE_FPSR_OFFSET);

  /* Fetch the floating point register.  */
  if (regno >= ARM_F0_REGNUM && regno <= ARM_F7_REGNUM)
    supply_nwfpe_register (regcache, regno, fp);
}

/* Get the whole floating point state of the process and store it
   into regcache.  */

static void
fetch_fpregs (struct regcache *regcache)
{
  int ret, regno, tid;
  gdb_byte fp[ARM_LINUX_SIZEOF_NWFPE];

  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);
  
  /* Read the floating point state.  */
  ret = ptrace (PT_GETFPREGS, tid, 0, fp);
  if (ret < 0)
    {
      warning (_("Unable to fetch the floating point registers."));
      return;
    }

  /* Fetch fpsr.  */
  regcache_raw_supply (regcache, ARM_FPS_REGNUM,
		       fp + NWFPE_FPSR_OFFSET);

  /* Fetch the floating point registers.  */
  for (regno = ARM_F0_REGNUM; regno <= ARM_F7_REGNUM; regno++)
    supply_nwfpe_register (regcache, regno, fp);
}

/* Save a particular register into the floating point state of the
   process using the contents from regcache.  */

static void
store_fpregister (const struct regcache *regcache, int regno)
{
  int ret, tid;
  gdb_byte fp[ARM_LINUX_SIZEOF_NWFPE];

  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);
  
  /* Read the floating point state.  */
  ret = ptrace (PT_GETFPREGS, tid, 0, fp);
  if (ret < 0)
    {
      warning (_("Unable to fetch the floating point registers."));
      return;
    }

  /* Store fpsr.  */
  if (ARM_FPS_REGNUM == regno
      && REG_VALID == regcache_register_status (regcache, ARM_FPS_REGNUM))
    regcache_raw_collect (regcache, ARM_FPS_REGNUM, fp + NWFPE_FPSR_OFFSET);

  /* Store the floating point register.  */
  if (regno >= ARM_F0_REGNUM && regno <= ARM_F7_REGNUM)
    collect_nwfpe_register (regcache, regno, fp);

  ret = ptrace (PTRACE_SETFPREGS, tid, 0, fp);
  if (ret < 0)
    {
      warning (_("Unable to store floating point register."));
      return;
    }
}

/* Save the whole floating point state of the process using
   the contents from regcache.  */

static void
store_fpregs (const struct regcache *regcache)
{
  int ret, regno, tid;
  gdb_byte fp[ARM_LINUX_SIZEOF_NWFPE];

  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);
  
  /* Read the floating point state.  */
  ret = ptrace (PT_GETFPREGS, tid, 0, fp);
  if (ret < 0)
    {
      warning (_("Unable to fetch the floating point registers."));
      return;
    }

  /* Store fpsr.  */
  if (REG_VALID == regcache_register_status (regcache, ARM_FPS_REGNUM))
    regcache_raw_collect (regcache, ARM_FPS_REGNUM, fp + NWFPE_FPSR_OFFSET);

  /* Store the floating point registers.  */
  for (regno = ARM_F0_REGNUM; regno <= ARM_F7_REGNUM; regno++)
    if (REG_VALID == regcache_register_status (regcache, regno))
      collect_nwfpe_register (regcache, regno, fp);

  ret = ptrace (PTRACE_SETFPREGS, tid, 0, fp);
  if (ret < 0)
    {
      warning (_("Unable to store floating point registers."));
      return;
    }
}

/* Fetch a general register of the process and store into
   regcache.  */

static void
fetch_register (struct regcache *regcache, int regno)
{
  int ret, tid;
  elf_gregset_t regs;

  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);
  
  ret = ptrace (PTRACE_GETREGS, tid, 0, &regs);
  if (ret < 0)
    {
      warning (_("Unable to fetch general register."));
      return;
    }

  if (regno >= ARM_A1_REGNUM && regno < ARM_PC_REGNUM)
    regcache_raw_supply (regcache, regno, (char *) &regs[regno]);

  if (ARM_PS_REGNUM == regno)
    {
      if (arm_apcs_32)
        regcache_raw_supply (regcache, ARM_PS_REGNUM,
			     (char *) &regs[ARM_CPSR_GREGNUM]);
      else
        regcache_raw_supply (regcache, ARM_PS_REGNUM,
			     (char *) &regs[ARM_PC_REGNUM]);
    }
    
  if (ARM_PC_REGNUM == regno)
    { 
      regs[ARM_PC_REGNUM] = gdbarch_addr_bits_remove
			      (get_regcache_arch (regcache),
			       regs[ARM_PC_REGNUM]);
      regcache_raw_supply (regcache, ARM_PC_REGNUM,
			   (char *) &regs[ARM_PC_REGNUM]);
    }
}

/* Fetch all general registers of the process and store into
   regcache.  */

static void
fetch_regs (struct regcache *regcache)
{
  int ret, regno, tid;
  elf_gregset_t regs;

  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);
  
  ret = ptrace (PTRACE_GETREGS, tid, 0, &regs);
  if (ret < 0)
    {
      warning (_("Unable to fetch general registers."));
      return;
    }

  for (regno = ARM_A1_REGNUM; regno < ARM_PC_REGNUM; regno++)
    regcache_raw_supply (regcache, regno, (char *) &regs[regno]);

  if (arm_apcs_32)
    regcache_raw_supply (regcache, ARM_PS_REGNUM,
			 (char *) &regs[ARM_CPSR_GREGNUM]);
  else
    regcache_raw_supply (regcache, ARM_PS_REGNUM,
			 (char *) &regs[ARM_PC_REGNUM]);

  regs[ARM_PC_REGNUM] = gdbarch_addr_bits_remove
			  (get_regcache_arch (regcache), regs[ARM_PC_REGNUM]);
  regcache_raw_supply (regcache, ARM_PC_REGNUM,
		       (char *) &regs[ARM_PC_REGNUM]);
}

/* Store all general registers of the process from the values in
   regcache.  */

static void
store_register (const struct regcache *regcache, int regno)
{
  int ret, tid;
  elf_gregset_t regs;
  
  if (REG_VALID != regcache_register_status (regcache, regno))
    return;

  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);
  
  /* Get the general registers from the process.  */
  ret = ptrace (PTRACE_GETREGS, tid, 0, &regs);
  if (ret < 0)
    {
      warning (_("Unable to fetch general registers."));
      return;
    }

  if (regno >= ARM_A1_REGNUM && regno <= ARM_PC_REGNUM)
    regcache_raw_collect (regcache, regno, (char *) &regs[regno]);
  else if (arm_apcs_32 && regno == ARM_PS_REGNUM)
    regcache_raw_collect (regcache, regno,
			 (char *) &regs[ARM_CPSR_GREGNUM]);
  else if (!arm_apcs_32 && regno == ARM_PS_REGNUM)
    regcache_raw_collect (regcache, ARM_PC_REGNUM,
			 (char *) &regs[ARM_PC_REGNUM]);

  ret = ptrace (PTRACE_SETREGS, tid, 0, &regs);
  if (ret < 0)
    {
      warning (_("Unable to store general register."));
      return;
    }
}

static void
store_regs (const struct regcache *regcache)
{
  int ret, regno, tid;
  elf_gregset_t regs;

  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);
  
  /* Fetch the general registers.  */
  ret = ptrace (PTRACE_GETREGS, tid, 0, &regs);
  if (ret < 0)
    {
      warning (_("Unable to fetch general registers."));
      return;
    }

  for (regno = ARM_A1_REGNUM; regno <= ARM_PC_REGNUM; regno++)
    {
      if (REG_VALID == regcache_register_status (regcache, regno))
	regcache_raw_collect (regcache, regno, (char *) &regs[regno]);
    }

  if (arm_apcs_32 && REG_VALID == regcache_register_status (regcache, ARM_PS_REGNUM))
    regcache_raw_collect (regcache, ARM_PS_REGNUM,
			 (char *) &regs[ARM_CPSR_GREGNUM]);

  ret = ptrace (PTRACE_SETREGS, tid, 0, &regs);

  if (ret < 0)
    {
      warning (_("Unable to store general registers."));
      return;
    }
}

/* Fetch all WMMX registers of the process and store into
   regcache.  */

#define IWMMXT_REGS_SIZE (16 * 8 + 6 * 4)

static void
fetch_wmmx_regs (struct regcache *regcache)
{
  char regbuf[IWMMXT_REGS_SIZE];
  int ret, regno, tid;

  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);

  ret = ptrace (PTRACE_GETWMMXREGS, tid, 0, regbuf);
  if (ret < 0)
    {
      warning (_("Unable to fetch WMMX registers."));
      return;
    }

  for (regno = 0; regno < 16; regno++)
    regcache_raw_supply (regcache, regno + ARM_WR0_REGNUM,
			 &regbuf[regno * 8]);

  for (regno = 0; regno < 2; regno++)
    regcache_raw_supply (regcache, regno + ARM_WCSSF_REGNUM,
			 &regbuf[16 * 8 + regno * 4]);

  for (regno = 0; regno < 4; regno++)
    regcache_raw_supply (regcache, regno + ARM_WCGR0_REGNUM,
			 &regbuf[16 * 8 + 2 * 4 + regno * 4]);
}

static void
store_wmmx_regs (const struct regcache *regcache)
{
  char regbuf[IWMMXT_REGS_SIZE];
  int ret, regno, tid;

  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);

  ret = ptrace (PTRACE_GETWMMXREGS, tid, 0, regbuf);
  if (ret < 0)
    {
      warning (_("Unable to fetch WMMX registers."));
      return;
    }

  for (regno = 0; regno < 16; regno++)
    if (REG_VALID == regcache_register_status (regcache,
					       regno + ARM_WR0_REGNUM))
      regcache_raw_collect (regcache, regno + ARM_WR0_REGNUM,
			    &regbuf[regno * 8]);

  for (regno = 0; regno < 2; regno++)
    if (REG_VALID == regcache_register_status (regcache,
					       regno + ARM_WCSSF_REGNUM))
      regcache_raw_collect (regcache, regno + ARM_WCSSF_REGNUM,
			    &regbuf[16 * 8 + regno * 4]);

  for (regno = 0; regno < 4; regno++)
    if (REG_VALID == regcache_register_status (regcache,
					       regno + ARM_WCGR0_REGNUM))
      regcache_raw_collect (regcache, regno + ARM_WCGR0_REGNUM,
			    &regbuf[16 * 8 + 2 * 4 + regno * 4]);

  ret = ptrace (PTRACE_SETWMMXREGS, tid, 0, regbuf);

  if (ret < 0)
    {
      warning (_("Unable to store WMMX registers."));
      return;
    }
}

/* Fetch and store VFP Registers.  The kernel object has space for 32
   64-bit registers, and the FPSCR.  This is even when on a VFPv2 or
   VFPv3D16 target.  */
#define VFP_REGS_SIZE (32 * 8 + 4)

static void
fetch_vfp_regs (struct regcache *regcache)
{
  char regbuf[VFP_REGS_SIZE];
  int ret, regno, tid;

  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);

  ret = ptrace (PTRACE_GETVFPREGS, tid, 0, regbuf);
  if (ret < 0)
    {
      warning (_("Unable to fetch VFP registers."));
      return;
    }

  for (regno = 0; regno < arm_linux_vfp_register_count; regno++)
    regcache_raw_supply (regcache, regno + ARM_D0_REGNUM,
			 (char *) regbuf + regno * 8);

  regcache_raw_supply (regcache, ARM_FPSCR_REGNUM,
		       (char *) regbuf + 32 * 8);
}

static void
store_vfp_regs (const struct regcache *regcache)
{
  char regbuf[VFP_REGS_SIZE];
  int ret, regno, tid;

  /* Get the thread id for the ptrace call.  */
  tid = GET_THREAD_ID (inferior_ptid);

  ret = ptrace (PTRACE_GETVFPREGS, tid, 0, regbuf);
  if (ret < 0)
    {
      warning (_("Unable to fetch VFP registers (for update)."));
      return;
    }

  for (regno = 0; regno < arm_linux_vfp_register_count; regno++)
    regcache_raw_collect (regcache, regno + ARM_D0_REGNUM,
			  (char *) regbuf + regno * 8);

  regcache_raw_collect (regcache, ARM_FPSCR_REGNUM,
			(char *) regbuf + 32 * 8);

  ret = ptrace (PTRACE_SETVFPREGS, tid, 0, regbuf);

  if (ret < 0)
    {
      warning (_("Unable to store VFP registers."));
      return;
    }
}

/* Fetch registers from the child process.  Fetch all registers if
   regno == -1, otherwise fetch all general registers or all floating
   point registers depending upon the value of regno.  */

static void
arm_linux_fetch_inferior_registers (struct target_ops *ops,
				    struct regcache *regcache, int regno)
{
  if (-1 == regno)
    {
      fetch_regs (regcache);
      fetch_fpregs (regcache);
      if (arm_linux_has_wmmx_registers)
	fetch_wmmx_regs (regcache);
      if (arm_linux_vfp_register_count > 0)
	fetch_vfp_regs (regcache);
    }
  else 
    {
      if (regno < ARM_F0_REGNUM || regno == ARM_PS_REGNUM)
        fetch_register (regcache, regno);
      else if (regno >= ARM_F0_REGNUM && regno <= ARM_FPS_REGNUM)
        fetch_fpregister (regcache, regno);
      else if (arm_linux_has_wmmx_registers
	       && regno >= ARM_WR0_REGNUM && regno <= ARM_WCGR7_REGNUM)
	fetch_wmmx_regs (regcache);
      else if (arm_linux_vfp_register_count > 0
	       && regno >= ARM_D0_REGNUM
	       && regno <= ARM_D0_REGNUM + arm_linux_vfp_register_count)
	fetch_vfp_regs (regcache);
    }
}

/* Store registers back into the inferior.  Store all registers if
   regno == -1, otherwise store all general registers or all floating
   point registers depending upon the value of regno.  */

static void
arm_linux_store_inferior_registers (struct target_ops *ops,
				    struct regcache *regcache, int regno)
{
  if (-1 == regno)
    {
      store_regs (regcache);
      store_fpregs (regcache);
      if (arm_linux_has_wmmx_registers)
	store_wmmx_regs (regcache);
      if (arm_linux_vfp_register_count > 0)
	store_vfp_regs (regcache);
    }
  else
    {
      if (regno < ARM_F0_REGNUM || regno == ARM_PS_REGNUM)
        store_register (regcache, regno);
      else if ((regno >= ARM_F0_REGNUM) && (regno <= ARM_FPS_REGNUM))
        store_fpregister (regcache, regno);
      else if (arm_linux_has_wmmx_registers
	       && regno >= ARM_WR0_REGNUM && regno <= ARM_WCGR7_REGNUM)
	store_wmmx_regs (regcache);
      else if (arm_linux_vfp_register_count > 0
	       && regno >= ARM_D0_REGNUM
	       && regno <= ARM_D0_REGNUM + arm_linux_vfp_register_count)
	store_vfp_regs (regcache);
    }
}

/* Wrapper functions for the standard regset handling, used by
   thread debugging.  */

void
fill_gregset (const struct regcache *regcache,	
	      gdb_gregset_t *gregsetp, int regno)
{
  arm_linux_collect_gregset (NULL, regcache, regno, gregsetp, 0);
}

void
supply_gregset (struct regcache *regcache, const gdb_gregset_t *gregsetp)
{
  arm_linux_supply_gregset (NULL, regcache, -1, gregsetp, 0);
}

void
fill_fpregset (const struct regcache *regcache,
	       gdb_fpregset_t *fpregsetp, int regno)
{
  arm_linux_collect_nwfpe (NULL, regcache, regno, fpregsetp, 0);
}

/* Fill GDB's register array with the floating-point register values
   in *fpregsetp.  */

void
supply_fpregset (struct regcache *regcache, const gdb_fpregset_t *fpregsetp)
{
  arm_linux_supply_nwfpe (NULL, regcache, -1, fpregsetp, 0);
}

/* Fetch the thread-local storage pointer for libthread_db.  */

ps_err_e
ps_get_thread_area (const struct ps_prochandle *ph,
                    lwpid_t lwpid, int idx, void **base)
{
  if (ptrace (PTRACE_GET_THREAD_AREA, lwpid, NULL, base) != 0)
    return PS_ERR;

  /* IDX is the bias from the thread pointer to the beginning of the
     thread descriptor.  It has to be subtracted due to implementation
     quirks in libthread_db.  */
  *base = (void *) ((char *)*base - idx);

  return PS_OK;
}

static const struct target_desc *
arm_linux_read_description (struct target_ops *ops)
{
  CORE_ADDR arm_hwcap = 0;
  arm_linux_has_wmmx_registers = 0;
  arm_linux_vfp_register_count = 0;

  if (target_auxv_search (ops, AT_HWCAP, &arm_hwcap) != 1)
    {
      return NULL;
    }

  if (arm_hwcap & HWCAP_IWMMXT)
    {
      arm_linux_has_wmmx_registers = 1;
      return tdesc_arm_with_iwmmxt;
    }

  if (arm_hwcap & HWCAP_VFP)
    {
      int pid;
      char *buf;
      const struct target_desc * result = NULL;

      /* NEON implies VFPv3-D32 or no-VFP unit.  Say that we only support
	 Neon with VFPv3-D32.  */
      if (arm_hwcap & HWCAP_NEON)
	{
	  arm_linux_vfp_register_count = 32;
	  result = tdesc_arm_with_neon;
	}
      else if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPv3D16)) == HWCAP_VFPv3)
	{
	  arm_linux_vfp_register_count = 32;
	  result = tdesc_arm_with_vfpv3;
	}
      else
	{
	  arm_linux_vfp_register_count = 16;
	  result = tdesc_arm_with_vfpv2;
	}

      /* Now make sure that the kernel supports reading these
	 registers.  Support was added in 2.6.30.  */
      pid = ptid_get_lwp (inferior_ptid);
      errno = 0;
      buf = alloca (VFP_REGS_SIZE);
      if (ptrace (PTRACE_GETVFPREGS, pid, 0, buf) < 0
	  && errno == EIO)
	result = NULL;

      return result;
    }

  return NULL;
}

/* Information describing the hardware breakpoint capabilities.  */
struct arm_linux_hwbp_cap
{
  gdb_byte arch;
  gdb_byte max_wp_length;
  gdb_byte wp_count;
  gdb_byte bp_count;
};

/* Get hold of the Hardware Breakpoint information for the target we are
   attached to.  Returns NULL if the kernel doesn't support Hardware 
   breakpoints at all, or a pointer to the information structure.  */
static const struct arm_linux_hwbp_cap *
arm_linux_get_hwbp_cap (void)
{
  /* The info structure we return.  */
  static struct arm_linux_hwbp_cap info;

  /* Is INFO in a good state?  -1 means that no attempt has been made to
     initialize INFO; 0 means an attempt has been made, but it failed; 1
     means INFO is in an initialized state.  */
  static int available = -1;

  if (available == -1)
    {
      int tid;
      unsigned int val;

      tid = GET_THREAD_ID (inferior_ptid);
      if (ptrace (PTRACE_GETHBPREGS, tid, 0, &val) < 0)
	available = 0;
      else
	{
	  info.arch = (gdb_byte)((val >> 24) & 0xff);
	  info.max_wp_length = (gdb_byte)((val >> 16) & 0xff);
	  info.wp_count = (gdb_byte)((val >> 8) & 0xff);
	  info.bp_count = (gdb_byte)(val & 0xff);
	  available = (info.arch != 0);
	}
    }

  return available == 1 ? &info : NULL;
}

/* How many hardware breakpoints are available?  */
static int
arm_linux_get_hw_breakpoint_count (void)
{
  const struct arm_linux_hwbp_cap *cap = arm_linux_get_hwbp_cap ();
  return cap != NULL ? cap->bp_count : 0;
}

/* How many hardware watchpoints are available?  */
static int
arm_linux_get_hw_watchpoint_count (void)
{
  const struct arm_linux_hwbp_cap *cap = arm_linux_get_hwbp_cap ();
  return cap != NULL ? cap->wp_count : 0;
}

/* Have we got a free break-/watch-point available for use?  Returns -1 if
   there is not an appropriate resource available, otherwise returns 1.  */
static int
arm_linux_can_use_hw_breakpoint (int type, int cnt, int ot)
{
  if (type == bp_hardware_watchpoint || type == bp_read_watchpoint
      || type == bp_access_watchpoint || type == bp_watchpoint)
    {
      if (cnt + ot > arm_linux_get_hw_watchpoint_count ())
	return -1;
    }
  else if (type == bp_hardware_breakpoint)
    {
      if (cnt > arm_linux_get_hw_breakpoint_count ())
	return -1;
    }
  else
    gdb_assert (FALSE);

  return 1;
}

/* Enum describing the different types of ARM hardware break-/watch-points.  */
typedef enum
{
  arm_hwbp_break = 0,
  arm_hwbp_load = 1,
  arm_hwbp_store = 2,
  arm_hwbp_access = 3
} arm_hwbp_type;

/* Type describing an ARM Hardware Breakpoint Control register value.  */
typedef unsigned int arm_hwbp_control_t;

/* Structure used to keep track of hardware break-/watch-points.  */
struct arm_linux_hw_breakpoint
{
  /* Address to break on, or being watched.  */
  unsigned int address;
  /* Control register for break-/watch- point.  */
  arm_hwbp_control_t control;
};

/* Structure containing arrays of the break and watch points which are have
   active in each thread.

   The Linux ptrace interface to hardware break-/watch-points presents the 
   values in a vector centred around 0 (which is used fo generic information).
   Positive indicies refer to breakpoint addresses/control registers, negative
   indices to watchpoint addresses/control registers.

   The Linux vector is indexed as follows:
      -((i << 1) + 2): Control register for watchpoint i.
      -((i << 1) + 1): Address register for watchpoint i.
                    0: Information register.
       ((i << 1) + 1): Address register for breakpoint i.
       ((i << 1) + 2): Control register for breakpoint i.

   This structure is used as a per-thread cache of the state stored by the 
   kernel, so that we don't need to keep calling into the kernel to find a 
   free breakpoint.

   We treat break-/watch-points with their enable bit clear as being deleted.
   */
typedef struct arm_linux_thread_points
{
  /* Thread ID.  */
  int tid;
  /* Breakpoints for thread.  */
  struct arm_linux_hw_breakpoint *bpts;
  /* Watchpoint for threads.  */
  struct arm_linux_hw_breakpoint *wpts;
} *arm_linux_thread_points_p;
DEF_VEC_P (arm_linux_thread_points_p);

/* Vector of hardware breakpoints for each thread.  */
VEC(arm_linux_thread_points_p) *arm_threads = NULL;

/* Find the list of hardware break-/watch-points for a thread with id TID.
   If no list exists for TID we return NULL if ALLOC_NEW is 0, otherwise we
   create a new list and return that.  */
static struct arm_linux_thread_points *
arm_linux_find_breakpoints_by_tid (int tid, int alloc_new)
{
  int i;
  struct arm_linux_thread_points *t;

  for (i = 0; VEC_iterate (arm_linux_thread_points_p, arm_threads, i, t); ++i)
    {
      if (t->tid == tid)
	return t;
    }

  t = NULL;

  if (alloc_new)
    {
      t = xmalloc (sizeof (struct arm_linux_thread_points));
      t->tid = tid;
      t->bpts = xzalloc (arm_linux_get_hw_breakpoint_count ()
			 * sizeof (struct arm_linux_hw_breakpoint));
      t->wpts = xzalloc (arm_linux_get_hw_watchpoint_count ()
			 * sizeof (struct arm_linux_hw_breakpoint));
      VEC_safe_push (arm_linux_thread_points_p, arm_threads, t);
    }

  return t;
}

/* Initialize an ARM hardware break-/watch-point control register value.
   BYTE_ADDRESS_SELECT is the mask of bytes to trigger on; HWBP_TYPE is the 
   type of break-/watch-point; ENABLE indicates whether the point is enabled.
   */
static arm_hwbp_control_t 
arm_hwbp_control_initialize (unsigned byte_address_select,
			     arm_hwbp_type hwbp_type,
			     int enable)
{
  gdb_assert ((byte_address_select & ~0xffU) == 0);
  gdb_assert (hwbp_type != arm_hwbp_break 
	      || ((byte_address_select & 0xfU) != 0));

  return (byte_address_select << 5) | (hwbp_type << 3) | (3 << 1) | enable;
}

/* Does the breakpoint control value CONTROL have the enable bit set?  */
static int
arm_hwbp_control_is_enabled (arm_hwbp_control_t control)
{
  return control & 0x1;
}

/* Change a breakpoint control word so that it is in the disabled state.  */
static arm_hwbp_control_t
arm_hwbp_control_disable (arm_hwbp_control_t control)
{
  return control & ~0x1;
}

/* Initialise the hardware breakpoint structure P.  The breakpoint will be
   enabled, and will point to the placed address of BP_TGT.  */
static void
arm_linux_hw_breakpoint_initialize (struct gdbarch *gdbarch,
				    struct bp_target_info *bp_tgt,
				    struct arm_linux_hw_breakpoint *p)
{
  unsigned mask;
  CORE_ADDR address = bp_tgt->placed_address;

  /* We have to create a mask for the control register which says which bits
     of the word pointed to by address to break on.  */
  if (arm_pc_is_thumb (gdbarch, address))
    {
      mask = 0x3;
      address &= ~1;
    }
  else
    {
      mask = 0xf;
      address &= ~3;
    }

  p->address = (unsigned int) address;
  p->control = arm_hwbp_control_initialize (mask, arm_hwbp_break, 1);
}

/* Get the ARM hardware breakpoint type from the RW value we're given when
   asked to set a watchpoint.  */
static arm_hwbp_type 
arm_linux_get_hwbp_type (int rw)
{
  if (rw == hw_read)
    return arm_hwbp_load;
  else if (rw == hw_write)
    return arm_hwbp_store;
  else
    return arm_hwbp_access;
}

/* Initialize the hardware breakpoint structure P for a watchpoint at ADDR
   to LEN.  The type of watchpoint is given in RW.  */
static void
arm_linux_hw_watchpoint_initialize (CORE_ADDR addr, int len, int rw,
				    struct arm_linux_hw_breakpoint *p)
{
  const struct arm_linux_hwbp_cap *cap = arm_linux_get_hwbp_cap ();
  unsigned mask;

  gdb_assert (cap != NULL);
  gdb_assert (cap->max_wp_length != 0);

  mask = (1 << len) - 1;

  p->address = (unsigned int) addr;
  p->control = arm_hwbp_control_initialize (mask, 
					    arm_linux_get_hwbp_type (rw), 1);
}

/* Are two break-/watch-points equal?  */
static int
arm_linux_hw_breakpoint_equal (const struct arm_linux_hw_breakpoint *p1,
			       const struct arm_linux_hw_breakpoint *p2)
{
  return p1->address == p2->address && p1->control == p2->control;
}

/* Insert the hardware breakpoint (WATCHPOINT = 0) or watchpoint (WATCHPOINT
   =1) BPT for thread TID.  */
static void
arm_linux_insert_hw_breakpoint1 (const struct arm_linux_hw_breakpoint* bpt, 
				int tid, int watchpoint)
{
  struct arm_linux_thread_points *t = arm_linux_find_breakpoints_by_tid (tid, 1);
  gdb_byte count, i;
  struct arm_linux_hw_breakpoint* bpts;
  int dir;

  gdb_assert (t != NULL);

  if (watchpoint)
    {
      count = arm_linux_get_hw_watchpoint_count ();
      bpts = t->wpts;
      dir = -1;
    }
  else
    {
      count = arm_linux_get_hw_breakpoint_count ();
      bpts = t->bpts;
      dir = 1;
    }

  for (i = 0; i < count; ++i)
    if (!arm_hwbp_control_is_enabled (bpts[i].control))
      {
	errno = 0;
	if (ptrace (PTRACE_SETHBPREGS, tid, dir * ((i << 1) + 1), 
		    &bpt->address) < 0)
	  perror_with_name (_("Unexpected error setting breakpoint address"));
	if (ptrace (PTRACE_SETHBPREGS, tid, dir * ((i << 1) + 2), 
		    &bpt->control) < 0)
	  perror_with_name (_("Unexpected error setting breakpoint"));

	memcpy (bpts + i, bpt, sizeof (struct arm_linux_hw_breakpoint));
	break;
      }

  gdb_assert (i != count);
}

/* Remove the hardware breakpoint (WATCHPOINT = 0) or watchpoint
   (WATCHPOINT = 1) BPT for thread TID.  */
static void
arm_linux_remove_hw_breakpoint1 (const struct arm_linux_hw_breakpoint *bpt, 
				 int tid, int watchpoint)
{
  struct arm_linux_thread_points *t = arm_linux_find_breakpoints_by_tid (tid, 0);
  gdb_byte count, i;
  struct arm_linux_hw_breakpoint *bpts;
  int dir;

  gdb_assert (t != NULL);

  if (watchpoint)
    {
      count = arm_linux_get_hw_watchpoint_count ();
      bpts = t->wpts;
      dir = -1;
    }
  else
    {
      count = arm_linux_get_hw_breakpoint_count ();
      bpts = t->bpts;
      dir = 1;
    }

  for (i = 0; i < count; ++i)
    if (arm_linux_hw_breakpoint_equal (bpt, bpts + i))
      {
	errno = 0;
	bpts[i].control = arm_hwbp_control_disable (bpts[i].control);
	if (ptrace (PTRACE_SETHBPREGS, tid, dir * ((i << 1) + 2), 
		    &bpts[i].control) < 0)
	  perror_with_name (_("Unexpected error clearing breakpoint"));
	break;
      }

  gdb_assert (i != count);
}

/* Insert a Hardware breakpoint.  */
static int
arm_linux_insert_hw_breakpoint (struct gdbarch *gdbarch, 
				struct bp_target_info *bp_tgt)
{
  struct lwp_info *lp;
  struct arm_linux_hw_breakpoint p;

  if (arm_linux_get_hw_breakpoint_count () == 0)
    return -1;

  arm_linux_hw_breakpoint_initialize (gdbarch, bp_tgt, &p);
  ALL_LWPS (lp)
    arm_linux_insert_hw_breakpoint1 (&p, ptid_get_lwp (lp->ptid), 0);

  return 0;
}

/* Remove a hardware breakpoint.  */
static int
arm_linux_remove_hw_breakpoint (struct gdbarch *gdbarch, 
				struct bp_target_info *bp_tgt)
{
  struct lwp_info *lp;
  struct arm_linux_hw_breakpoint p;

  if (arm_linux_get_hw_breakpoint_count () == 0)
    return -1;

  arm_linux_hw_breakpoint_initialize (gdbarch, bp_tgt, &p);
  ALL_LWPS (lp)
    arm_linux_remove_hw_breakpoint1 (&p, ptid_get_lwp (lp->ptid), 0);

  return 0;
}

/* Are we able to use a hardware watchpoint for the LEN bytes starting at 
   ADDR?  */
static int
arm_linux_region_ok_for_hw_watchpoint (CORE_ADDR addr, int len)
{
  const struct arm_linux_hwbp_cap *cap = arm_linux_get_hwbp_cap ();
  CORE_ADDR max_wp_length, aligned_addr;

  /* Can not set watchpoints for zero or negative lengths.  */
  if (len <= 0)
    return 0;

  /* Need to be able to use the ptrace interface.  */
  if (cap == NULL || cap->wp_count == 0)
    return 0;

  /* Test that the range [ADDR, ADDR + LEN) fits into the largest address
     range covered by a watchpoint.  */
  max_wp_length = (CORE_ADDR)cap->max_wp_length;
  aligned_addr = addr & ~(max_wp_length - 1);

  if (aligned_addr + max_wp_length < addr + len)
    return 0;

  /* The current ptrace interface can only handle watchpoints that are a
     power of 2.  */
  if ((len & (len - 1)) != 0)
    return 0;

  /* All tests passed so we must be able to set a watchpoint.  */
  return 1;
}

/* Insert a Hardware breakpoint.  */
static int
arm_linux_insert_watchpoint (CORE_ADDR addr, int len, int rw,
			     struct expression *cond)
{
  struct lwp_info *lp;
  struct arm_linux_hw_breakpoint p;

  if (arm_linux_get_hw_watchpoint_count () == 0)
    return -1;

  arm_linux_hw_watchpoint_initialize (addr, len, rw, &p);
  ALL_LWPS (lp)
    arm_linux_insert_hw_breakpoint1 (&p, ptid_get_lwp (lp->ptid), 1);

  return 0;
}

/* Remove a hardware breakpoint.  */
static int
arm_linux_remove_watchpoint (CORE_ADDR addr, int len, int rw,
			     struct expression *cond)
{
  struct lwp_info *lp;
  struct arm_linux_hw_breakpoint p;

  if (arm_linux_get_hw_watchpoint_count () == 0)
    return -1;

  arm_linux_hw_watchpoint_initialize (addr, len, rw, &p);
  ALL_LWPS (lp)
    arm_linux_remove_hw_breakpoint1 (&p, ptid_get_lwp (lp->ptid), 1);

  return 0;
}

/* What was the data address the target was stopped on accessing.  */
static int
arm_linux_stopped_data_address (struct target_ops *target, CORE_ADDR *addr_p)
{
  siginfo_t siginfo;
  int slot;

  if (!linux_nat_get_siginfo (inferior_ptid, &siginfo))
    return 0;

  /* This must be a hardware breakpoint.  */
  if (siginfo.si_signo != SIGTRAP
      || (siginfo.si_code & 0xffff) != 0x0004 /* TRAP_HWBKPT */)
    return 0;

  /* We must be able to set hardware watchpoints.  */
  if (arm_linux_get_hw_watchpoint_count () == 0)
    return 0;

  slot = siginfo.si_errno;

  /* If we are in a positive slot then we're looking at a breakpoint and not
     a watchpoint.  */
  if (slot >= 0)
    return 0;

  *addr_p = (CORE_ADDR) (uintptr_t) siginfo.si_addr;
  return 1;
}

/* Has the target been stopped by hitting a watchpoint?  */
static int
arm_linux_stopped_by_watchpoint (void)
{
  CORE_ADDR addr;
  return arm_linux_stopped_data_address (&current_target, &addr);
}

static int
arm_linux_watchpoint_addr_within_range (struct target_ops *target,
					CORE_ADDR addr,
					CORE_ADDR start, int length)
{
  return start <= addr && start + length - 1 >= addr;
}

/* Handle thread creation.  We need to copy the breakpoints and watchpoints
   in the parent thread to the child thread.  */
static void
arm_linux_new_thread (struct lwp_info *lp)
{
  int tid = ptid_get_lwp (lp->ptid);
  const struct arm_linux_hwbp_cap *info = arm_linux_get_hwbp_cap ();

  if (info != NULL)
    {
      int i;
      struct arm_linux_thread_points *p;
      struct arm_linux_hw_breakpoint *bpts;

      if (VEC_empty (arm_linux_thread_points_p, arm_threads))
	return;

      /* Get a list of breakpoints from any thread. */
      p = VEC_last (arm_linux_thread_points_p, arm_threads);

      /* Copy that thread's breakpoints and watchpoints to the new thread. */
      for (i = 0; i < info->bp_count; i++)
	if (arm_hwbp_control_is_enabled (p->bpts[i].control))
	  arm_linux_insert_hw_breakpoint1 (p->bpts + i, tid, 0);
      for (i = 0; i < info->wp_count; i++)
	if (arm_hwbp_control_is_enabled (p->wpts[i].control))
	  arm_linux_insert_hw_breakpoint1 (p->wpts + i, tid, 1);
    }
}

/* Handle thread exit.  Tidy up the memory that has been allocated for the
   thread.  */
static void
arm_linux_thread_exit (struct thread_info *tp, int silent)
{
  const struct arm_linux_hwbp_cap *info = arm_linux_get_hwbp_cap ();

  if (info != NULL)
    {
      int i;
      int tid = ptid_get_lwp (tp->ptid);
      struct arm_linux_thread_points *t = NULL, *p;

      for (i = 0; 
	   VEC_iterate (arm_linux_thread_points_p, arm_threads, i, p); i++)
	{
	  if (p->tid == tid)
	    {
	      t = p;
	      break;
	    }
	}

      if (t == NULL)
	return;

      VEC_unordered_remove (arm_linux_thread_points_p, arm_threads, i);

      xfree (t->bpts);
      xfree (t->wpts);
      xfree (t);
    }
}

void _initialize_arm_linux_nat (void);

void
_initialize_arm_linux_nat (void)
{
  struct target_ops *t;

  /* Fill in the generic GNU/Linux methods.  */
  t = linux_target ();

  /* Add our register access methods.  */
  t->to_fetch_registers = arm_linux_fetch_inferior_registers;
  t->to_store_registers = arm_linux_store_inferior_registers;

  /* Add our hardware breakpoint and watchpoint implementation.  */
  t->to_can_use_hw_breakpoint = arm_linux_can_use_hw_breakpoint;
  t->to_insert_hw_breakpoint = arm_linux_insert_hw_breakpoint;
  t->to_remove_hw_breakpoint = arm_linux_remove_hw_breakpoint;
  t->to_region_ok_for_hw_watchpoint = arm_linux_region_ok_for_hw_watchpoint;
  t->to_insert_watchpoint = arm_linux_insert_watchpoint;
  t->to_remove_watchpoint = arm_linux_remove_watchpoint;
  t->to_stopped_by_watchpoint = arm_linux_stopped_by_watchpoint;
  t->to_stopped_data_address = arm_linux_stopped_data_address;
  t->to_watchpoint_addr_within_range = arm_linux_watchpoint_addr_within_range;

  t->to_read_description = arm_linux_read_description;

  /* Register the target.  */
  linux_nat_add_target (t);

  /* Handle thread creation and exit */
  observer_attach_thread_exit (arm_linux_thread_exit);
  linux_nat_set_new_thread (t, arm_linux_new_thread);
}
