/* GNU/Linux S/390 specific low level interface, for the remote server
   for GDB.
   Copyright (C) 2001-2013 Free Software Foundation, Inc.

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

/* This file is used for both 31-bit and 64-bit S/390 systems.  */

#include "server.h"
#include "linux-low.h"
#include "elf/common.h"

#include <asm/ptrace.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
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

/* Defined in auto-generated file s390-linux32.c.  */
void init_registers_s390_linux32 (void);
/* Defined in auto-generated file s390-linux32v1.c.  */
void init_registers_s390_linux32v1 (void);
/* Defined in auto-generated file s390-linux32v2.c.  */
void init_registers_s390_linux32v2 (void);
/* Defined in auto-generated file s390-linux64.c.  */
void init_registers_s390_linux64 (void);
/* Defined in auto-generated file s390-linux64v1.c.  */
void init_registers_s390_linux64v1 (void);
/* Defined in auto-generated file s390-linux64v2.c.  */
void init_registers_s390_linux64v2 (void);
/* Defined in auto-generated file s390x-linux64.c.  */
void init_registers_s390x_linux64 (void);
/* Defined in auto-generated file s390x-linux64v1.c.  */
void init_registers_s390x_linux64v1 (void);
/* Defined in auto-generated file s390x-linux64v2.c.  */
void init_registers_s390x_linux64v2 (void);

#define s390_num_regs 52

static int s390_regmap[] = {
  PT_PSWMASK, PT_PSWADDR,

  PT_GPR0, PT_GPR1, PT_GPR2, PT_GPR3,
  PT_GPR4, PT_GPR5, PT_GPR6, PT_GPR7,
  PT_GPR8, PT_GPR9, PT_GPR10, PT_GPR11,
  PT_GPR12, PT_GPR13, PT_GPR14, PT_GPR15,

  PT_ACR0, PT_ACR1, PT_ACR2, PT_ACR3,
  PT_ACR4, PT_ACR5, PT_ACR6, PT_ACR7,
  PT_ACR8, PT_ACR9, PT_ACR10, PT_ACR11,
  PT_ACR12, PT_ACR13, PT_ACR14, PT_ACR15,

  PT_FPC,

#ifndef __s390x__
  PT_FPR0_HI, PT_FPR1_HI, PT_FPR2_HI, PT_FPR3_HI,
  PT_FPR4_HI, PT_FPR5_HI, PT_FPR6_HI, PT_FPR7_HI,
  PT_FPR8_HI, PT_FPR9_HI, PT_FPR10_HI, PT_FPR11_HI,
  PT_FPR12_HI, PT_FPR13_HI, PT_FPR14_HI, PT_FPR15_HI,
#else
  PT_FPR0, PT_FPR1, PT_FPR2, PT_FPR3,
  PT_FPR4, PT_FPR5, PT_FPR6, PT_FPR7,
  PT_FPR8, PT_FPR9, PT_FPR10, PT_FPR11,
  PT_FPR12, PT_FPR13, PT_FPR14, PT_FPR15,
#endif

  PT_ORIGGPR2,
};

#ifdef __s390x__
#define s390_num_regs_3264 68

static int s390_regmap_3264[] = {
  PT_PSWMASK, PT_PSWADDR,

  PT_GPR0, PT_GPR0, PT_GPR1, PT_GPR1,
  PT_GPR2, PT_GPR2, PT_GPR3, PT_GPR3,
  PT_GPR4, PT_GPR4, PT_GPR5, PT_GPR5,
  PT_GPR6, PT_GPR6, PT_GPR7, PT_GPR7,
  PT_GPR8, PT_GPR8, PT_GPR9, PT_GPR9,
  PT_GPR10, PT_GPR10, PT_GPR11, PT_GPR11,
  PT_GPR12, PT_GPR12, PT_GPR13, PT_GPR13,
  PT_GPR14, PT_GPR14, PT_GPR15, PT_GPR15,

  PT_ACR0, PT_ACR1, PT_ACR2, PT_ACR3,
  PT_ACR4, PT_ACR5, PT_ACR6, PT_ACR7,
  PT_ACR8, PT_ACR9, PT_ACR10, PT_ACR11,
  PT_ACR12, PT_ACR13, PT_ACR14, PT_ACR15,

  PT_FPC,

  PT_FPR0, PT_FPR1, PT_FPR2, PT_FPR3,
  PT_FPR4, PT_FPR5, PT_FPR6, PT_FPR7,
  PT_FPR8, PT_FPR9, PT_FPR10, PT_FPR11,
  PT_FPR12, PT_FPR13, PT_FPR14, PT_FPR15,

  PT_ORIGGPR2,
};
#endif


static int
s390_cannot_fetch_register (int regno)
{
  return 0;
}

static int
s390_cannot_store_register (int regno)
{
  return 0;
}

static void
s390_collect_ptrace_register (struct regcache *regcache, int regno, char *buf)
{
  int size = register_size (regno);
  if (size < sizeof (long))
    {
      int regaddr = the_low_target.regmap[regno];

      memset (buf, 0, sizeof (long));

      if ((regno ^ 1) < the_low_target.num_regs
	  && the_low_target.regmap[regno ^ 1] == regaddr)
	{
	  collect_register (regcache, regno & ~1, buf);
	  collect_register (regcache, (regno & ~1) + 1,
			    buf + sizeof (long) - size);
	}
      else if (regaddr == PT_PSWMASK)
	{
	  /* Convert 4-byte PSW mask to 8 bytes by clearing bit 12 and copying
	     the basic addressing mode bit from the PSW address.  */
	  char *addr = alloca (register_size (regno ^ 1));
	  collect_register (regcache, regno, buf);
	  collect_register (regcache, regno ^ 1, addr);
	  buf[1] &= ~0x8;
	  buf[size] |= (addr[0] & 0x80);
	}
      else if (regaddr == PT_PSWADDR)
	{
	  /* Convert 4-byte PSW address to 8 bytes by clearing the addressing
	     mode bit (which gets copied to the PSW mask instead).  */
	  collect_register (regcache, regno, buf + sizeof (long) - size);
	  buf[sizeof (long) - size] &= ~0x80;
	}
      else if ((regaddr >= PT_GPR0 && regaddr <= PT_GPR15)
	       || regaddr == PT_ORIGGPR2)
	collect_register (regcache, regno, buf + sizeof (long) - size);
      else
	collect_register (regcache, regno, buf);
    }
  else
    collect_register (regcache, regno, buf);
}

static void
s390_supply_ptrace_register (struct regcache *regcache,
			     int regno, const char *buf)
{
  int size = register_size (regno);
  if (size < sizeof (long))
    {
      int regaddr = the_low_target.regmap[regno];

      if ((regno ^ 1) < the_low_target.num_regs
	  && the_low_target.regmap[regno ^ 1] == regaddr)
	{
	  supply_register (regcache, regno & ~1, buf);
	  supply_register (regcache, (regno & ~1) + 1,
			   buf + sizeof (long) - size);
	}
      else if (regaddr == PT_PSWMASK)
	{
	  /* Convert 8-byte PSW mask to 4 bytes by setting bit 12 and copying
	     the basic addressing mode into the PSW address.  */
	  char *mask = alloca (size);
	  char *addr = alloca (register_size (regno ^ 1));
	  memcpy (mask, buf, size);
	  mask[1] |= 0x8;
	  supply_register (regcache, regno, mask);

	  collect_register (regcache, regno ^ 1, addr);
	  addr[0] &= ~0x80;
	  addr[0] |= (buf[size] & 0x80);
	  supply_register (regcache, regno ^ 1, addr);
	}
      else if (regaddr == PT_PSWADDR)
	{
	  /* Convert 8-byte PSW address to 4 bytes by truncating, but
	     keeping the addressing mode bit (which was set from the mask).  */
	  char *addr = alloca (size);
	  char amode;
	  collect_register (regcache, regno, addr);
	  amode = addr[0] & 0x80;
	  memcpy (addr, buf + sizeof (long) - size, size);
	  addr[0] &= ~0x80;
	  addr[0] |= amode;
	  supply_register (regcache, regno, addr);
	}
      else if ((regaddr >= PT_GPR0 && regaddr <= PT_GPR15)
	       || regaddr == PT_ORIGGPR2)
	supply_register (regcache, regno, buf + sizeof (long) - size);
      else
	supply_register (regcache, regno, buf);
    }
  else
    supply_register (regcache, regno, buf);
}

/* Provide only a fill function for the general register set.  ps_lgetregs
   will use this for NPTL support.  */

static void s390_fill_gregset (struct regcache *regcache, void *buf)
{
  int i;

  for (i = 0; i < the_low_target.num_regs; i++)
    {
      if (the_low_target.regmap[i] < PT_PSWMASK
	  || the_low_target.regmap[i] > PT_ACR15)
	continue;

      s390_collect_ptrace_register (regcache, i, (char *) buf
				       + the_low_target.regmap[i]);
    }
}

/* Fill and store functions for extended register sets.  */

static void
s390_fill_last_break (struct regcache *regcache, void *buf)
{
  /* Last break address is read-only.  */
}

static void
s390_store_last_break (struct regcache *regcache, const void *buf)
{
  supply_register_by_name (regcache, "last_break",
			   (const char *)buf + 8 - register_size (0));
}

static void
s390_fill_system_call (struct regcache *regcache, void *buf)
{
  collect_register_by_name (regcache, "system_call", buf);
}

static void
s390_store_system_call (struct regcache *regcache, const void *buf)
{
  supply_register_by_name (regcache, "system_call", buf);
}

struct regset_info target_regsets[] = {
  { 0, 0, 0, 0, GENERAL_REGS, s390_fill_gregset, NULL },
  /* Last break address is read-only; do not attempt PTRACE_SETREGSET.  */
  { PTRACE_GETREGSET, PTRACE_GETREGSET, NT_S390_LAST_BREAK, 0,
    EXTENDED_REGS, s390_fill_last_break, s390_store_last_break },
  { PTRACE_GETREGSET, PTRACE_SETREGSET, NT_S390_SYSTEM_CALL, 0,
    EXTENDED_REGS, s390_fill_system_call, s390_store_system_call },
  { 0, 0, 0, -1, -1, NULL, NULL }
};


static const unsigned char s390_breakpoint[] = { 0, 1 };
#define s390_breakpoint_len 2

static CORE_ADDR
s390_get_pc (struct regcache *regcache)
{
  if (register_size (0) == 4)
    {
      unsigned int pswa;
      collect_register_by_name (regcache, "pswa", &pswa);
      return pswa & 0x7fffffff;
    }
  else
    {
      unsigned long pc;
      collect_register_by_name (regcache, "pswa", &pc);
      return pc;
    }
}

static void
s390_set_pc (struct regcache *regcache, CORE_ADDR newpc)
{
  if (register_size (0) == 4)
    {
      unsigned int pswa;
      collect_register_by_name (regcache, "pswa", &pswa);
      pswa = (pswa & 0x80000000) | (newpc & 0x7fffffff);
      supply_register_by_name (regcache, "pswa", &pswa);
    }
  else
    {
      unsigned long pc = newpc;
      supply_register_by_name (regcache, "pswa", &pc);
    }
}

#ifdef __s390x__
static unsigned long
s390_get_hwcap (void)
{
  int wordsize = register_size (0);
  unsigned char *data = alloca (2 * wordsize);
  int offset = 0;

  while ((*the_target->read_auxv) (offset, data, 2 * wordsize) == 2 * wordsize)
    {
      if (wordsize == 4)
        {
          unsigned int *data_p = (unsigned int *)data;
          if (data_p[0] == AT_HWCAP)
	    return data_p[1];
        }
      else
        {
          unsigned long *data_p = (unsigned long *)data;
          if (data_p[0] == AT_HWCAP)
	    return data_p[1];
        }

      offset += 2 * wordsize;
    }

  return 0;
}
#endif

static int
s390_check_regset (int pid, int regset, int regsize)
{
  gdb_byte *buf = alloca (regsize);
  struct iovec iov;

  iov.iov_base = buf;
  iov.iov_len = regsize;

  if (ptrace (PTRACE_GETREGSET, pid, (long) regset, (long) &iov) < 0)
    return 0;
  else
    return 1;
}

static void
s390_arch_setup (void)
{
  struct regset_info *regset;

  /* Check whether the kernel supports extra register sets.  */
  int pid = pid_of (get_thread_lwp (current_inferior));
  int have_regset_last_break
    = s390_check_regset (pid, NT_S390_LAST_BREAK, 8);
  int have_regset_system_call
    = s390_check_regset (pid, NT_S390_SYSTEM_CALL, 4);

  /* Update target_regsets according to available register sets.  */
  for (regset = target_regsets; regset->fill_function != NULL; regset++)
    if (regset->get_request == PTRACE_GETREGSET)
      switch (regset->nt_type)
	{
	case NT_S390_LAST_BREAK:
	  regset->size = have_regset_last_break? 8 : 0;
	  break;
	case NT_S390_SYSTEM_CALL:
	  regset->size = have_regset_system_call? 4 : 0;
	  break;
	default:
	  break;
	}

  /* Assume 31-bit inferior process.  */
  if (have_regset_system_call)
    init_registers_s390_linux32v2 ();
  else if (have_regset_last_break)
    init_registers_s390_linux32v1 ();
  else
    init_registers_s390_linux32 ();

  the_low_target.num_regs = s390_num_regs;
  the_low_target.regmap = s390_regmap;

  /* On a 64-bit host, check the low bit of the (31-bit) PSWM
     -- if this is one, we actually have a 64-bit inferior.  */
#ifdef __s390x__
  {
    unsigned int pswm;
    struct regcache *regcache = new_register_cache ();
    fetch_inferior_registers (regcache, find_regno ("pswm"));
    collect_register_by_name (regcache, "pswm", &pswm);
    free_register_cache (regcache);

    if (pswm & 1)
      {
	if (have_regset_system_call)
	  init_registers_s390x_linux64v2 ();
	else if (have_regset_last_break)
	  init_registers_s390x_linux64v1 ();
	else
	  init_registers_s390x_linux64 ();
      }

    /* For a 31-bit inferior, check whether the kernel supports
       using the full 64-bit GPRs.  */
    else if (s390_get_hwcap () & HWCAP_S390_HIGH_GPRS)
      {
	if (have_regset_system_call)
	  init_registers_s390_linux64v2 ();
	else if (have_regset_last_break)
	  init_registers_s390_linux64v1 ();
	else
	  init_registers_s390_linux64 ();

	the_low_target.num_regs = s390_num_regs_3264;
	the_low_target.regmap = s390_regmap_3264;
      }
  }
#endif
}


static int
s390_breakpoint_at (CORE_ADDR pc)
{
  unsigned char c[s390_breakpoint_len];
  read_inferior_memory (pc, c, s390_breakpoint_len);
  return memcmp (c, s390_breakpoint, s390_breakpoint_len) == 0;
}


struct linux_target_ops the_low_target = {
  s390_arch_setup,
  s390_num_regs,
  s390_regmap,
  NULL,
  s390_cannot_fetch_register,
  s390_cannot_store_register,
  NULL, /* fetch_register */
  s390_get_pc,
  s390_set_pc,
  s390_breakpoint,
  s390_breakpoint_len,
  NULL,
  s390_breakpoint_len,
  s390_breakpoint_at,
  NULL,
  NULL,
  NULL,
  NULL,
  s390_collect_ptrace_register,
  s390_supply_ptrace_register,
};
