/* Target-dependent code for SPARC systems running NetBSD.
   Copyright 2002 Free Software Foundation, Inc.
   Contributed by Wasabi Systems, Inc.

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
#include "regcache.h"
#include "target.h"
#include "value.h"
#include "osabi.h"

#include "sparcnbsd-tdep.h"
#include "nbsd-tdep.h"

#include "solib-svr4.h"

#define REG32_OFFSET_PSR	(0 * 4)
#define REG32_OFFSET_PC		(1 * 4)
#define REG32_OFFSET_NPC	(2 * 4)
#define REG32_OFFSET_Y		(3 * 4)
#define REG32_OFFSET_GLOBAL	(4 * 4)
#define REG32_OFFSET_OUT	(12 * 4)

#define REG64_OFFSET_TSTATE	(0 * 8)
#define REG64_OFFSET_PC		(1 * 8)
#define REG64_OFFSET_NPC	(2 * 8)
#define REG64_OFFSET_Y		(3 * 8)
#define REG64_OFFSET_GLOBAL	(4 * 8)
#define REG64_OFFSET_OUT	(12 * 8)

void
sparcnbsd_supply_reg32 (char *regs, int regno)
{
  int i;

  if (regno == PS_REGNUM || regno == -1)
    supply_register (PS_REGNUM, regs + REG32_OFFSET_PSR);

  if (regno == PC_REGNUM || regno == -1)
    supply_register (PC_REGNUM, regs + REG32_OFFSET_PC);

  if (regno == NPC_REGNUM || regno == -1)
    supply_register (NPC_REGNUM, regs + REG32_OFFSET_NPC);

  if (regno == Y_REGNUM || regno == -1)
    supply_register (Y_REGNUM, regs + REG32_OFFSET_Y);

  if ((regno >= G0_REGNUM && regno <= G7_REGNUM) || regno == -1)
    {
      if (regno == G0_REGNUM || regno == -1)
	supply_register (G0_REGNUM, NULL);	/* %g0 is always zero */
      for (i = G1_REGNUM; i <= G7_REGNUM; i++)
	{
	  if (regno == i || regno == -1)
	    supply_register (i, regs + REG32_OFFSET_GLOBAL +
	                     ((i - G0_REGNUM) * 4));
	}
    }

  if ((regno >= O0_REGNUM && regno <= O7_REGNUM) || regno == -1)
    {
      for (i = O0_REGNUM; i <= O7_REGNUM; i++)
	{
	  if (regno == i || regno == -1)
	    supply_register (i, regs + REG32_OFFSET_OUT +
			     ((i - O0_REGNUM) * 4));
        }
    }

  /* Inputs and Locals are stored onto the stack by by the kernel.  */
  if ((regno >= L0_REGNUM && regno <= I7_REGNUM) || regno == -1)
    {
      CORE_ADDR sp = read_register (SP_REGNUM);
      char buf[4];

      for (i = L0_REGNUM; i <= I7_REGNUM; i++)
	{
	  if (regno == i || regno == -1)
	    {
              target_read_memory (sp + ((i - L0_REGNUM) * 4),
                                  buf, sizeof (buf));
	      supply_register (i, buf);
	    }
	}
    }

  /* FIXME: If we don't set these valid, read_register_bytes() rereads
     all the regs every time it is called!  */
  if (regno == WIM_REGNUM || regno == -1)
    supply_register (WIM_REGNUM, NULL);
  if (regno == TBR_REGNUM || regno == -1)
    supply_register (TBR_REGNUM, NULL);
  if (regno == CPS_REGNUM || regno == -1)
    supply_register (CPS_REGNUM, NULL);
}

void
sparcnbsd_supply_reg64 (char *regs, int regno)
{
  int i;
  char buf[8];

  if (regno == TSTATE_REGNUM || regno == -1)
    supply_register (PS_REGNUM, regs + REG64_OFFSET_TSTATE);

  if (regno == PC_REGNUM || regno == -1)
    supply_register (PC_REGNUM, regs + REG64_OFFSET_PC);

  if (regno == NPC_REGNUM || regno == -1)
    supply_register (NPC_REGNUM, regs + REG64_OFFSET_NPC);

  if (regno == Y_REGNUM || regno == -1)
    {
      memset (buf, 0, sizeof (buf));
      memcpy (&buf[4], regs + REG64_OFFSET_Y, 4);
      supply_register (Y_REGNUM, buf);
    }

  if ((regno >= G0_REGNUM && regno <= G7_REGNUM) || regno == -1)
    {
      if (regno == G0_REGNUM || regno == -1)
	supply_register (G0_REGNUM, NULL);	/* %g0 is always zero */
      for (i = G1_REGNUM; i <= G7_REGNUM; i++)
	{
	  if (regno == i || regno == -1)
	    supply_register (i, regs + REG64_OFFSET_GLOBAL +
	                     ((i - G0_REGNUM) * 8));
	}
    }

  if ((regno >= O0_REGNUM && regno <= O7_REGNUM) || regno == -1)
    {
      for (i = O0_REGNUM; i <= O7_REGNUM; i++)
	{
	  if (regno == i || regno == -1)
	    supply_register (i, regs + REG64_OFFSET_OUT +
			     ((i - O0_REGNUM) * 8));
        }
    }

  /* Inputs and Locals are stored onto the stack by by the kernel.  */
  if ((regno >= L0_REGNUM && regno <= I7_REGNUM) || regno == -1)
    {
      CORE_ADDR sp = read_register (SP_REGNUM);
      char buf[8];

      if (sp & 1)
	{
	  /* Registers are 64-bit.  */
	  sp += 2047;

	  for (i = L0_REGNUM; i <= I7_REGNUM; i++)
	    {
	      if (regno == i || regno == -1)
	        {
                  target_read_memory (sp + ((i - L0_REGNUM) * 8),
                                      buf, sizeof (buf));
	          supply_register (i, buf);
		}
	    }
	}
      else
	{
	  /* Registers are 32-bit.  Toss any sign-extension of the stack
	     pointer, clear out the top half of the temporary buffer, and
	     put the register value in the bottom half.  */

	  sp &= 0xffffffffUL;
	  memset (buf, 0, sizeof (buf));
	  for (i = L0_REGNUM; i <= I7_REGNUM; i++)
	    {
	      if (regno == i || regno == -1)
		{
		  target_read_memory (sp + ((i - L0_REGNUM) * 4),
				      &buf[4], sizeof (buf));
                  supply_register (i, buf);
		}
	    }
	}
    }

  /* FIXME: If we don't set these valid, read_register_bytes() rereads
     all the regs every time it is called!  */
  if (regno == WIM_REGNUM || regno == -1)
    supply_register (WIM_REGNUM, NULL);
  if (regno == TBR_REGNUM || regno == -1)
    supply_register (TBR_REGNUM, NULL);
  if (regno == CPS_REGNUM || regno == -1)
    supply_register (CPS_REGNUM, NULL);
}

void
sparcnbsd_fill_reg32 (char *regs, int regno)
{
  int i;

  if (regno == PS_REGNUM || regno == -1)
    regcache_collect (PS_REGNUM, regs + REG32_OFFSET_PSR);

  if (regno == PC_REGNUM || regno == -1)
    regcache_collect (PC_REGNUM, regs + REG32_OFFSET_PC);

  if (regno == NPC_REGNUM || regno == -1)
    regcache_collect (NPC_REGNUM, regs + REG32_OFFSET_NPC);

  if (regno == Y_REGNUM || regno == -1)
    regcache_collect (Y_REGNUM, regs + REG32_OFFSET_Y);

  if ((regno >= G0_REGNUM && regno <= G7_REGNUM) || regno == -1)
    {
      /* %g0 is always zero */
      for (i = G1_REGNUM; i <= G7_REGNUM; i++)
	{
	  if (regno == i || regno == -1)
	    regcache_collect (i, regs + REG32_OFFSET_GLOBAL +
	                      ((i - G0_REGNUM) * 4));
	}
    }

  if ((regno >= O0_REGNUM && regno <= O7_REGNUM) || regno == -1)
    {
      for (i = O0_REGNUM; i <= O7_REGNUM; i++)
	{
	  if (regno == i || regno == -1)
	    regcache_collect (i, regs + REG32_OFFSET_OUT +
			      ((i - O0_REGNUM) * 4));
        }
    }

  /* Responsibility for the stack regs is pushed off onto the caller.  */
}

void
sparcnbsd_fill_reg64 (char *regs, int regno)
{
  int i;

  if (regno == TSTATE_REGNUM || regno == -1)
    regcache_collect (TSTATE_REGNUM, regs + REG64_OFFSET_TSTATE);

  if (regno == PC_REGNUM || regno == -1)
    regcache_collect (PC_REGNUM, regs + REG64_OFFSET_PC);

  if (regno == NPC_REGNUM || regno == -1)
    regcache_collect (NPC_REGNUM, regs + REG64_OFFSET_NPC);

  if (regno == Y_REGNUM || regno == -1)
    regcache_collect (Y_REGNUM, regs + REG64_OFFSET_Y);

  if ((regno >= G0_REGNUM && regno <= G7_REGNUM) || regno == -1)
    {
      /* %g0 is always zero */
      for (i = G1_REGNUM; i <= G7_REGNUM; i++)
	{
	  if (regno == i || regno == -1)
	    regcache_collect (i, regs + REG64_OFFSET_GLOBAL +
	                      ((i - G0_REGNUM) * 4));
	}
    }

  if ((regno >= O0_REGNUM && regno <= O7_REGNUM) || regno == -1)
    {
      for (i = O0_REGNUM; i <= O7_REGNUM; i++)
	{
	  if (regno == i || regno == -1)
	    regcache_collect (i, regs + REG64_OFFSET_OUT +
			      ((i - O0_REGNUM) * 4));
        }
    }

  /* Responsibility for the stack regs is pushed off onto the caller.  */
}

void
sparcnbsd_supply_fpreg32 (char *fpregs, int regno)
{
  int i;

  for (i = 0; i <= 31; i++)
    {
      if (regno == (FP0_REGNUM + i) || regno == -1)
	supply_register (FP0_REGNUM + i, fpregs + (i * 4));
    }

  if (regno == FPS_REGNUM || regno == -1)
    supply_register (FPS_REGNUM, fpregs + (32 * 4));
}

void
sparcnbsd_supply_fpreg64 (char *fpregs, int regno)
{
  int i;

  for (i = 0; i <= 31; i++)
    {
      if (regno == (FP0_REGNUM + i) || regno == -1)
	supply_register (FP0_REGNUM + i, fpregs + (i * 4));
    }

  for (; i <= 47; i++)
    {
      if (regno == (FP0_REGNUM + i) || regno == -1)
	supply_register (FP0_REGNUM + i, fpregs + (32 * 4) + (i * 8));
    }

  if (regno == FPS_REGNUM || regno == -1)
    supply_register (FPS_REGNUM, fpregs + (32 * 4) + (16 * 8));

  /* XXX %gsr */
}

void
sparcnbsd_fill_fpreg32 (char *fpregs, int regno)
{
  int i;

  for (i = 0; i <= 31; i++)
    {
      if (regno == (FP0_REGNUM + i) || regno == -1)
	regcache_collect (FP0_REGNUM + i, fpregs + (i * 4));
    }

  if (regno == FPS_REGNUM || regno == -1)
    regcache_collect (FPS_REGNUM, fpregs + (32 * 4));
}

void
sparcnbsd_fill_fpreg64 (char *fpregs, int regno)
{
  int i;

  for (i = 0; i <= 31; i++)
    {
      if (regno == (FP0_REGNUM + i) || regno == -1)
	regcache_collect (FP0_REGNUM + i, fpregs + (i * 4));
    }

  for (; i <= 47; i++)
    {
      if (regno == (FP0_REGNUM + i) || regno == -1)
	regcache_collect (FP0_REGNUM + i, fpregs + (32 * 4) + (i * 8));
    }

  if (regno == FPS_REGNUM || regno == -1)
    regcache_collect (FPS_REGNUM, fpregs + (32 * 4) + (16 * 8));

  /* XXX %gsr */
}

/* Unlike other NetBSD implementations, the SPARC port historically used
   .reg and .reg2 (see bfd/netbsd-core.c), and as such, we can share one
   routine for a.out and ELF core files.  */
static void
fetch_core_registers (char *core_reg_sect, unsigned core_reg_size, int which,
                      CORE_ADDR ignore)
{
  int reg_size, fpreg_size;

  if (gdbarch_ptr_bit (current_gdbarch) == 32)
    {
      reg_size = (20 * 4);
      fpreg_size = (33 * 4);
    }
  else
    {
      reg_size = (20 * 8);
      fpreg_size = (64 * 4)
        + 8  /* fsr */
        + 4  /* gsr */
        + 4; /* pad */
    }

  switch (which)
    {
    case 0:  /* Integer registers */
      if (core_reg_size != reg_size)
	warning ("Wrong size register set in core file.");
      else if (gdbarch_ptr_bit (current_gdbarch) == 32)
	sparcnbsd_supply_reg32 (core_reg_sect, -1);
      else
	sparcnbsd_supply_reg64 (core_reg_sect, -1);
      break;

    case 2:  /* Floating pointer registers */
      if (core_reg_size != fpreg_size)
	warning ("Wrong size FP register set in core file.");
      else if (gdbarch_ptr_bit (current_gdbarch) == 32)
	sparcnbsd_supply_fpreg32 (core_reg_sect, -1);
      else
	sparcnbsd_supply_fpreg64 (core_reg_sect, -1);
      break;

    default:
      /* Don't know what kind of register request this is; just ignore it.  */
      break;
    }
}

static struct core_fns sparcnbsd_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL
};

static struct core_fns sparcnbsd_elfcore_fns =
{
  bfd_target_elf_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL
};

/* FIXME: Need PC_IN_SIGTRAMP() support, but NetBSD/sparc signal trampolines
   aren't easily identified.  */

static int
sparcnbsd_get_longjmp_target_32 (CORE_ADDR *pc)
{
  CORE_ADDR jb_addr;
  char buf[4];

  jb_addr = read_register (O0_REGNUM);

  if (target_read_memory (jb_addr + 12, buf, sizeof (buf)))
    return 0;

  *pc = extract_address (buf, sizeof (buf));

  return 1;
}

static int
sparcnbsd_get_longjmp_target_64 (CORE_ADDR *pc)
{
  CORE_ADDR jb_addr;
  char buf[8];

  jb_addr = read_register (O0_REGNUM);

  if (target_read_memory (jb_addr + 16, buf, sizeof (buf)))
    return 0;

  *pc = extract_address (buf, sizeof (buf));

  return 1;
}

static int
sparcnbsd_aout_in_solib_call_trampoline (CORE_ADDR pc, char *name)
{
  if (strcmp (name, "_DYNAMIC") == 0)
    return 1;

  return 0;
}

static void
sparcnbsd_init_abi_common (struct gdbarch_info info,
                           struct gdbarch *gdbarch)
{
  set_gdbarch_get_longjmp_target (gdbarch, gdbarch_ptr_bit (gdbarch) == 32 ?
	                                   sparcnbsd_get_longjmp_target_32 :
	                                   sparcnbsd_get_longjmp_target_64);
}

static void
sparcnbsd_init_abi_aout (struct gdbarch_info info,
                         struct gdbarch *gdbarch)
{
  sparcnbsd_init_abi_common (info, gdbarch);

  set_gdbarch_in_solib_call_trampoline (gdbarch,
                                     sparcnbsd_aout_in_solib_call_trampoline);
}

static void
sparcnbsd_init_abi_elf (struct gdbarch_info info,
                        struct gdbarch *gdbarch)
{
  sparcnbsd_init_abi_common (info, gdbarch);

  set_gdbarch_pc_in_sigtramp (gdbarch, nbsd_pc_in_sigtramp);

  set_solib_svr4_fetch_link_map_offsets (gdbarch,
				         gdbarch_ptr_bit (gdbarch) == 32 ?
                                nbsd_ilp32_solib_svr4_fetch_link_map_offsets :
				nbsd_lp64_solib_svr4_fetch_link_map_offsets);
}

static enum gdb_osabi
sparcnbsd_aout_osabi_sniffer (bfd *abfd)
{
  if (strcmp (bfd_get_target (abfd), "a.out-sparc-netbsd") == 0)
    return GDB_OSABI_NETBSD_AOUT;

  return GDB_OSABI_UNKNOWN;
}

void
_initialize_sparnbsd_tdep (void)
{
  gdbarch_register_osabi_sniffer (bfd_arch_sparc, bfd_target_aout_flavour,
				  sparcnbsd_aout_osabi_sniffer);

  gdbarch_register_osabi (bfd_arch_sparc, GDB_OSABI_NETBSD_AOUT,
			  sparcnbsd_init_abi_aout);
  gdbarch_register_osabi (bfd_arch_sparc, GDB_OSABI_NETBSD_ELF,
			  sparcnbsd_init_abi_elf);

  add_core_fns (&sparcnbsd_core_fns);
  add_core_fns (&sparcnbsd_elfcore_fns);
}
