/* GNU/Linux/ARM specific low level interface, for the remote server for GDB.
   Copyright (C) 1995, 1996, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005,
   2006, 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

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

#include "server.h"
#include "linux-low.h"

/* Don't include elf.h if linux/elf.h got included by gdb_proc_service.h.
   On Bionic elf.h and linux/elf.h have conflicting definitions.  */
#ifndef ELFMAG0
#include <elf.h>
#endif
#include <sys/ptrace.h>

/* Defined in auto-generated files.  */
void init_registers_arm (void);
void init_registers_arm_with_iwmmxt (void);
void init_registers_arm_with_vfpv2 (void);
void init_registers_arm_with_vfpv3 (void);
void init_registers_arm_with_neon (void);

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 22
#endif

#ifndef PTRACE_GETWMMXREGS
# define PTRACE_GETWMMXREGS 18
# define PTRACE_SETWMMXREGS 19
#endif

#ifndef PTRACE_GETVFPREGS
# define PTRACE_GETVFPREGS 27
# define PTRACE_SETVFPREGS 28
#endif

static unsigned long arm_hwcap;

/* These are in <asm/elf.h> in current kernels.  */
#define HWCAP_VFP       64
#define HWCAP_IWMMXT    512
#define HWCAP_NEON      4096
#define HWCAP_VFPv3     8192
#define HWCAP_VFPv3D16  16384

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#define arm_num_regs 26

static int arm_regmap[] = {
  0, 4, 8, 12, 16, 20, 24, 28,
  32, 36, 40, 44, 48, 52, 56, 60,
  -1, -1, -1, -1, -1, -1, -1, -1, -1,
  64
};

static int
arm_cannot_store_register (int regno)
{
  return (regno >= arm_num_regs);
}

static int
arm_cannot_fetch_register (int regno)
{
  return (regno >= arm_num_regs);
}

static void
arm_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;

  for (i = 0; i < arm_num_regs; i++)
    if (arm_regmap[i] != -1)
      collect_register (regcache, i, ((char *) buf) + arm_regmap[i]);
}

static void
arm_store_gregset (struct regcache *regcache, const void *buf)
{
  int i;
  char zerobuf[8];

  memset (zerobuf, 0, 8);
  for (i = 0; i < arm_num_regs; i++)
    if (arm_regmap[i] != -1)
      supply_register (regcache, i, ((char *) buf) + arm_regmap[i]);
    else
      supply_register (regcache, i, zerobuf);
}

static void
arm_fill_wmmxregset (struct regcache *regcache, void *buf)
{
  int i;

  if (!(arm_hwcap & HWCAP_IWMMXT))
    return;

  for (i = 0; i < 16; i++)
    collect_register (regcache, arm_num_regs + i, (char *) buf + i * 8);

  /* We only have access to wcssf, wcasf, and wcgr0-wcgr3.  */
  for (i = 0; i < 6; i++)
    collect_register (regcache, arm_num_regs + i + 16,
		      (char *) buf + 16 * 8 + i * 4);
}

static void
arm_store_wmmxregset (struct regcache *regcache, const void *buf)
{
  int i;

  if (!(arm_hwcap & HWCAP_IWMMXT))
    return;

  for (i = 0; i < 16; i++)
    supply_register (regcache, arm_num_regs + i, (char *) buf + i * 8);

  /* We only have access to wcssf, wcasf, and wcgr0-wcgr3.  */
  for (i = 0; i < 6; i++)
    supply_register (regcache, arm_num_regs + i + 16,
		     (char *) buf + 16 * 8 + i * 4);
}

static void
arm_fill_vfpregset (struct regcache *regcache, void *buf)
{
  int i, num, base;

  if (!(arm_hwcap & HWCAP_VFP))
    return;

  if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPv3D16)) == HWCAP_VFPv3)
    num = 32;
  else
    num = 16;

  base = find_regno ("d0");
  for (i = 0; i < num; i++)
    collect_register (regcache, base + i, (char *) buf + i * 8);

  collect_register_by_name (regcache, "fpscr", (char *) buf + 32 * 8);
}

static void
arm_store_vfpregset (struct regcache *regcache, const void *buf)
{
  int i, num, base;

  if (!(arm_hwcap & HWCAP_VFP))
    return;

  if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPv3D16)) == HWCAP_VFPv3)
    num = 32;
  else
    num = 16;

  base = find_regno ("d0");
  for (i = 0; i < num; i++)
    supply_register (regcache, base + i, (char *) buf + i * 8);

  supply_register_by_name (regcache, "fpscr", (char *) buf + 32 * 8);
}

extern int debug_threads;

static CORE_ADDR
arm_get_pc (struct regcache *regcache)
{
  unsigned long pc;
  collect_register_by_name (regcache, "pc", &pc);
  if (debug_threads)
    fprintf (stderr, "stop pc is %08lx\n", pc);
  return pc;
}

static void
arm_set_pc (struct regcache *regcache, CORE_ADDR pc)
{
  unsigned long newpc = pc;
  supply_register_by_name (regcache, "pc", &newpc);
}

/* Correct in either endianness.  */
static const unsigned long arm_breakpoint = 0xef9f0001;
#define arm_breakpoint_len 4
static const unsigned short thumb_breakpoint = 0xde01;
static const unsigned short thumb2_breakpoint[] = { 0xf7f0, 0xa000 };

/* For new EABI binaries.  We recognize it regardless of which ABI
   is used for gdbserver, so single threaded debugging should work
   OK, but for multi-threaded debugging we only insert the current
   ABI's breakpoint instruction.  For now at least.  */
static const unsigned long arm_eabi_breakpoint = 0xe7f001f0;

static int
arm_breakpoint_at (CORE_ADDR where)
{
  struct regcache *regcache = get_thread_regcache (current_inferior, 1);
  unsigned long cpsr;

  collect_register_by_name (regcache, "cpsr", &cpsr);

  if (cpsr & 0x20)
    {
      /* Thumb mode.  */
      unsigned short insn;

      (*the_target->read_memory) (where, (unsigned char *) &insn, 2);
      if (insn == thumb_breakpoint)
	return 1;

      if (insn == thumb2_breakpoint[0])
	{
	  (*the_target->read_memory) (where + 2, (unsigned char *) &insn, 2);
	  if (insn == thumb2_breakpoint[1])
	    return 1;
	}
    }
  else
    {
      /* ARM mode.  */
      unsigned long insn;

      (*the_target->read_memory) (where, (unsigned char *) &insn, 4);
      if (insn == arm_breakpoint)
	return 1;

      if (insn == arm_eabi_breakpoint)
	return 1;
    }

  return 0;
}

/* We only place breakpoints in empty marker functions, and thread locking
   is outside of the function.  So rather than importing software single-step,
   we can just run until exit.  */
static CORE_ADDR
arm_reinsert_addr (void)
{
  struct regcache *regcache = get_thread_regcache (current_inferior, 1);
  unsigned long pc;
  collect_register_by_name (regcache, "lr", &pc);
  return pc;
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

static int
arm_get_hwcap (unsigned long *valp)
{
  unsigned char *data = alloca (8);
  int offset = 0;

  while ((*the_target->read_auxv) (offset, data, 8) == 8)
    {
      unsigned int *data_p = (unsigned int *)data;
      if (data_p[0] == AT_HWCAP)
	{
	  *valp = data_p[1];
	  return 1;
	}

      offset += 8;
    }

  *valp = 0;
  return 0;
}

static void
arm_arch_setup (void)
{
  arm_hwcap = 0;
  if (arm_get_hwcap (&arm_hwcap) == 0)
    {
      init_registers_arm ();
      return;
    }

  if (arm_hwcap & HWCAP_IWMMXT)
    {
      init_registers_arm_with_iwmmxt ();
      return;
    }

  if (arm_hwcap & HWCAP_VFP)
    {
      int pid;
      char *buf;

      /* NEON implies either no VFP, or VFPv3-D32.  We only support
	 it with VFP.  */
      if (arm_hwcap & HWCAP_NEON)
	init_registers_arm_with_neon ();
      else if ((arm_hwcap & (HWCAP_VFPv3 | HWCAP_VFPv3D16)) == HWCAP_VFPv3)
	init_registers_arm_with_vfpv3 ();
      else
	init_registers_arm_with_vfpv2 ();

      /* Now make sure that the kernel supports reading these
	 registers.  Support was added in 2.6.30.  */
      pid = lwpid_of (get_thread_lwp (current_inferior));
      errno = 0;
      buf = xmalloc (32 * 8 + 4);
      if (ptrace (PTRACE_GETVFPREGS, pid, 0, buf) < 0
	  && errno == EIO)
	{
	  arm_hwcap = 0;
	  init_registers_arm ();
	}
      free (buf);

      return;
    }

  /* The default configuration uses legacy FPA registers, probably
     simulated.  */
  init_registers_arm ();
}

struct regset_info target_regsets[] = {
  { PTRACE_GETREGS, PTRACE_SETREGS, 0, 18 * 4,
    GENERAL_REGS,
    arm_fill_gregset, arm_store_gregset },
  { PTRACE_GETWMMXREGS, PTRACE_SETWMMXREGS, 0, 16 * 8 + 6 * 4,
    EXTENDED_REGS,
    arm_fill_wmmxregset, arm_store_wmmxregset },
  { PTRACE_GETVFPREGS, PTRACE_SETVFPREGS, 0, 32 * 8 + 4,
    EXTENDED_REGS,
    arm_fill_vfpregset, arm_store_vfpregset },
  { 0, 0, 0, -1, -1, NULL, NULL }
};

struct linux_target_ops the_low_target = {
  arm_arch_setup,
  arm_num_regs,
  arm_regmap,
  arm_cannot_fetch_register,
  arm_cannot_store_register,
  arm_get_pc,
  arm_set_pc,

  /* Define an ARM-mode breakpoint; we only set breakpoints in the C
     library, which is most likely to be ARM.  If the kernel supports
     clone events, we will never insert a breakpoint, so even a Thumb
     C library will work; so will mixing EABI/non-EABI gdbserver and
     application.  */
#ifndef __ARM_EABI__
  (const unsigned char *) &arm_breakpoint,
#else
  (const unsigned char *) &arm_eabi_breakpoint,
#endif
  arm_breakpoint_len,
  arm_reinsert_addr,
  0,
  arm_breakpoint_at,
};
