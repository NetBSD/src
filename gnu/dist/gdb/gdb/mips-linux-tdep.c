/* Target-dependent code for GNU/Linux on MIPS processors.
   Copyright 2001 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "gdbcore.h"
#include "target.h"
#include "solib-svr4.h"

/* Copied from <asm/elf.h>.  */
#define ELF_NGREG       45
#define ELF_NFPREG      33

typedef unsigned char elf_greg_t[4];
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

typedef unsigned char elf_fpreg_t[8];
typedef elf_fpreg_t elf_fpregset_t[ELF_NFPREG];

/* 0 - 31 are integer registers, 32 - 63 are fp registers.  */
#define FPR_BASE        32
#define PC              64
#define CAUSE           65
#define BADVADDR        66
#define MMHI            67
#define MMLO            68
#define FPC_CSR         69
#define FPC_EIR         70

#define EF_REG0			6
#define EF_REG31		37
#define EF_LO			38
#define EF_HI			39
#define EF_CP0_EPC		40
#define EF_CP0_BADVADDR		41
#define EF_CP0_STATUS		42
#define EF_CP0_CAUSE		43

#define EF_SIZE			180

/* Figure out where the longjmp will land.
   We expect the first arg to be a pointer to the jmp_buf structure from
   which we extract the pc (MIPS_LINUX_JB_PC) that we will land at.  The pc
   is copied into PC.  This routine returns 1 on success.  */

int
mips_linux_get_longjmp_target (CORE_ADDR *pc)
{
  CORE_ADDR jb_addr;
  char buf[TARGET_PTR_BIT / TARGET_CHAR_BIT];

  jb_addr = read_register (A0_REGNUM);

  if (target_read_memory (jb_addr
			  + MIPS_LINUX_JB_PC * MIPS_LINUX_JB_ELEMENT_SIZE,
			  buf, TARGET_PTR_BIT / TARGET_CHAR_BIT))
    return 0;

  *pc = extract_address (buf, TARGET_PTR_BIT / TARGET_CHAR_BIT);

  return 1;
}

/* Unpack an elf_gregset_t into GDB's register cache.  */

void 
supply_gregset (elf_gregset_t *gregsetp)
{
  int regi;
  elf_greg_t *regp = *gregsetp;
  char *zerobuf = alloca (MAX_REGISTER_RAW_SIZE);

  memset (zerobuf, 0, MAX_REGISTER_RAW_SIZE);

  for (regi = EF_REG0; regi <= EF_REG31; regi++)
    supply_register ((regi - EF_REG0), (char *)(regp + regi));

  supply_register (LO_REGNUM, (char *)(regp + EF_LO));
  supply_register (HI_REGNUM, (char *)(regp + EF_HI));

  supply_register (PC_REGNUM, (char *)(regp + EF_CP0_EPC));
  supply_register (BADVADDR_REGNUM, (char *)(regp + EF_CP0_BADVADDR));
  supply_register (PS_REGNUM, (char *)(regp + EF_CP0_STATUS));
  supply_register (CAUSE_REGNUM, (char *)(regp + EF_CP0_CAUSE));

  /* Fill inaccessible registers with zero.  */
  supply_register (FP_REGNUM, zerobuf);
  supply_register (UNUSED_REGNUM, zerobuf);
  for (regi = FIRST_EMBED_REGNUM; regi < LAST_EMBED_REGNUM; regi++)
    supply_register (regi, zerobuf);
}

/* Pack our registers (or one register) into an elf_gregset_t.  */

void
fill_gregset (elf_gregset_t *gregsetp, int regno)
{
  int regaddr, regi;
  elf_greg_t *regp = *gregsetp;
  void *src, *dst;

  if (regno == -1)
    {
      memset (regp, 0, sizeof (elf_gregset_t));
      for (regi = 0; regi < 32; regi++)
        fill_gregset (gregsetp, regi);
      fill_gregset (gregsetp, LO_REGNUM);
      fill_gregset (gregsetp, HI_REGNUM);
      fill_gregset (gregsetp, PC_REGNUM);
      fill_gregset (gregsetp, BADVADDR_REGNUM);
      fill_gregset (gregsetp, PS_REGNUM);
      fill_gregset (gregsetp, CAUSE_REGNUM);

      return;
   }

  if (regno < 32)
    {
      src = &registers[REGISTER_BYTE (regno)];
      dst = regp + regno + EF_REG0;
      memcpy (dst, src, sizeof (elf_greg_t));
      return;
    }

  regaddr = -1;
  switch (regno)
    {
      case LO_REGNUM:
	regaddr = EF_LO;
	break;
      case HI_REGNUM:
	regaddr = EF_HI;
	break;
      case PC_REGNUM:
	regaddr = EF_CP0_EPC;
	break;
      case BADVADDR_REGNUM:
	regaddr = EF_CP0_BADVADDR;
	break;
      case PS_REGNUM:
	regaddr = EF_CP0_STATUS;
	break;
      case CAUSE_REGNUM:
	regaddr = EF_CP0_CAUSE;
	break;
    }

  if (regaddr != -1)
    {
      src = &registers[REGISTER_BYTE (regno)];
      dst = regp + regaddr;
      memcpy (dst, src, sizeof (elf_greg_t));
    }
}

/* Likewise, unpack an elf_fpregset_t.  */

void
supply_fpregset (elf_fpregset_t *fpregsetp)
{
  register int regi;
  char *zerobuf = alloca (MAX_REGISTER_RAW_SIZE);

  memset (zerobuf, 0, MAX_REGISTER_RAW_SIZE);

  for (regi = 0; regi < 32; regi++)
    supply_register (FP0_REGNUM + regi,
		     (char *)(*fpregsetp + regi));

  supply_register (FCRCS_REGNUM, (char *)(*fpregsetp + 32));

  /* FIXME: how can we supply FCRIR_REGNUM?  The ABI doesn't tell us. */
  supply_register (FCRIR_REGNUM, zerobuf);
}

/* Likewise, pack one or all floating point registers into an
   elf_fpregset_t.  */

void
fill_fpregset (elf_fpregset_t *fpregsetp, int regno)
{
  char *from, *to;

  if ((regno >= FP0_REGNUM) && (regno < FP0_REGNUM + 32))
    {
      from = (char *) &registers[REGISTER_BYTE (regno)];
      to = (char *) (*fpregsetp + regno - FP0_REGNUM);
      memcpy (to, from, REGISTER_RAW_SIZE (regno - FP0_REGNUM));
    }
  else if (regno == FCRCS_REGNUM)
    {
      from = (char *) &registers[REGISTER_BYTE (regno)];
      to = (char *) (*fpregsetp + 32);
      memcpy (to, from, REGISTER_RAW_SIZE (regno));
    }
  else if (regno == -1)
    {
      int regi;

      for (regi = 0; regi < 32; regi++)
	fill_fpregset (fpregsetp, FP0_REGNUM + regi);
      fill_fpregset(fpregsetp, FCRCS_REGNUM);
    }
}

/* Map gdb internal register number to ptrace ``address''.
   These ``addresses'' are normally defined in <asm/ptrace.h>.  */

CORE_ADDR
register_addr (int regno, CORE_ADDR blockend)
{
  int regaddr;

  if (regno < 0 || regno >= NUM_REGS)
    error ("Bogon register number %d.", regno);

  if (regno < 32)
    regaddr = regno;
  else if ((regno >= FP0_REGNUM) && (regno < FP0_REGNUM + 32))
    regaddr = FPR_BASE + (regno - FP0_REGNUM);
  else if (regno == PC_REGNUM)
    regaddr = PC;
  else if (regno == CAUSE_REGNUM)
    regaddr = CAUSE;
  else if (regno == BADVADDR_REGNUM)
    regaddr = BADVADDR;
  else if (regno == LO_REGNUM)
    regaddr = MMLO;
  else if (regno == HI_REGNUM)
    regaddr = MMHI;
  else if (regno == FCRCS_REGNUM)
    regaddr = FPC_CSR;
  else if (regno == FCRIR_REGNUM)
    regaddr = FPC_EIR;
  else
    error ("Unknowable register number %d.", regno);

  return regaddr;
}

/*  Use a local version of this function to get the correct types for
    regsets, until multi-arch core support is ready.  */

static void
fetch_core_registers (char *core_reg_sect, unsigned core_reg_size,
		      int which, CORE_ADDR reg_addr)
{
  elf_gregset_t gregset;
  elf_fpregset_t fpregset;

  if (which == 0)
    {
      if (core_reg_size != sizeof (gregset))
	{
	  warning ("wrong size gregset struct in core file");
	}
      else
	{
	  memcpy ((char *) &gregset, core_reg_sect, sizeof (gregset));
	  supply_gregset (&gregset);
	}
    }
  else if (which == 2)
    {
      if (core_reg_size != sizeof (fpregset))
	{
	  warning ("wrong size fpregset struct in core file");
	}
      else
	{
	  memcpy ((char *) &fpregset, core_reg_sect, sizeof (fpregset));
	  supply_fpregset (&fpregset);
	}
    }
}

/* Register that we are able to handle ELF file formats using standard
   procfs "regset" structures.  */

static struct core_fns regset_core_fns =
{
  bfd_target_elf_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

/* Fetch (and possibly build) an appropriate link_map_offsets
   structure for native GNU/Linux MIPS targets using the struct offsets
   defined in link.h (but without actual reference to that file).

   This makes it possible to access GNU/Linux MIPS shared libraries from a
   GDB that was built on a different host platform (for cross debugging).  */

struct link_map_offsets *
mips_linux_svr4_fetch_link_map_offsets (void)
{ 
  static struct link_map_offsets lmo;
  static struct link_map_offsets *lmp = NULL;

  if (lmp == NULL)
    { 
      lmp = &lmo;

      lmo.r_debug_size = 8;	/* The actual size is 20 bytes, but
				   this is all we need.  */
      lmo.r_map_offset = 4;
      lmo.r_map_size   = 4;

      lmo.link_map_size = 20;

      lmo.l_addr_offset = 0;
      lmo.l_addr_size   = 4;

      lmo.l_name_offset = 4;
      lmo.l_name_size   = 4;

      lmo.l_next_offset = 12;
      lmo.l_next_size   = 4;

      lmo.l_prev_offset = 16;
      lmo.l_prev_size   = 4;
    }

  return lmp;
}

void
_initialize_mips_linux_tdep (void)
{
  add_core_fns (&regset_core_fns);
}
