/* Native-dependent code for SuperH running NetBSD, for GDB.
   Copyright 1988, 1989, 1991, 1992, 1994, 1996 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>

#include "defs.h"
#include "inferior.h"
#include "gdbcore.h"

/* Map GDB register index to ptrace register buffer offset. */
static const int regmap[] =
{
  (20 * 4),	/* r0 */
  (19 * 4),	/* r1 */
  (18 * 4),	/* r2 */
  (17 * 4),	/* r3 */
  (16 * 4),	/* r4 */
  (15 * 4),	/* r5 */
  (14 * 4),	/* r6 */
  (13 * 4),	/* r7 */
  (12 * 4),	/* r8 */
  (11 * 4),	/* r9 */
  (10 * 4),	/* r10 */
  ( 9 * 4),	/* r11 */
  ( 8 * 4),	/* r12 */
  ( 7 * 4),	/* r13 */
  ( 6 * 4),	/* r14 */
  ( 5 * 4),	/* r15 */
};

/* Determine if PT_GETREGS fetches this register. */
#define GETREGS_SUPPLIES(regno) \
  (((regno) >= R0_REGNUM && (regno) <= (R0_REGNUM + 15)) \
|| (regno) == PC_REGNUM || (regno) == PR_REGNUM \
|| (regno) == MACH_REGNUM || (regno) == MACL_REGNUM \
|| (regno) == SR_REGNUM)

static void
supply_regs (regs)
     char *regs;
{
  int i;

  for (i = R0_REGNUM; i <= (R0_REGNUM + 15); i++)
    supply_register (i, regs + regmap[i - R0_REGNUM]);

  supply_register (PC_REGNUM, regs + (0 * 4));
  supply_register (SR_REGNUM, regs + (1 * 4));
  supply_register (PR_REGNUM, regs + (2 * 4));
  supply_register (MACH_REGNUM, regs + (3 * 4));
  supply_register (MACL_REGNUM, regs + (4 * 4));
}

static void
fill_regs (regs)
     char *regs;
{
  int i;

  for (i = R0_REGNUM; i <= (R0_REGNUM + 15); i++)
    memcpy (regs + regmap[i - R0_REGNUM],
	    &registers[REGISTER_BYTE (i)], REGISTER_RAW_SIZE (i));

  memcpy (regs + (0 * 4), &registers[REGISTER_BYTE (PC_REGNUM)],
	  REGISTER_RAW_SIZE (PC_REGNUM));
  memcpy (regs + (1 * 4), &registers[REGISTER_BYTE (SR_REGNUM)],
	  REGISTER_RAW_SIZE (SR_REGNUM));
  memcpy (regs + (2 * 4), &registers[REGISTER_BYTE (PR_REGNUM)],
	  REGISTER_RAW_SIZE (PR_REGNUM));
  memcpy (regs + (3 * 4), &registers[REGISTER_BYTE (MACH_REGNUM)],
	  REGISTER_RAW_SIZE (MACH_REGNUM));
  memcpy (regs + (4 * 4), &registers[REGISTER_BYTE (MACL_REGNUM)],
	  REGISTER_RAW_SIZE (MACL_REGNUM));
}

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;

  if (regno == -1 || GETREGS_SUPPLIES (regno))
    {
      ptrace (PT_GETREGS, inferior_pid,
	      (PTRACE_ARG3_TYPE) & inferior_registers, 0);
      supply_regs ((char *) &inferior_registers);
      
      if (regno != -1)
	return;
    }
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;

  fill_regs ((char *) &inferior_registers);
  ptrace (PT_SETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) & inferior_registers, 0);
}

struct md_core
{
  struct reg intreg;
};

static void
fetch_core_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;
{
  struct md_core *core_reg = (struct md_core *) core_reg_sect;

  /* We get everything from the .reg section.  */
  if (which != 0)
    return;

  if (core_reg_size < sizeof (struct reg))
    {
      warning ("Wrong size register set in core file.");
      return;
    }

  /* Integer registers */
  supply_regs ((char *) &core_reg->intreg);
}

static void
fetch_elfcore_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;
{
  switch (which)
    {
    case 0:  /* Integer registers */
      if (core_reg_size != sizeof (struct reg))
	warning ("Wrong size register set in core file.");
      else
	supply_regs (core_reg_sect);
      break;

    default:
      /* Don't know what kind of register request this is; just ignore it.  */
      break;
    }
}

#ifdef	FETCH_KCORE_REGISTERS
/*
 * Get registers from a kernel crash dump or live kernel.
 * Called by kcore-nbsd.c:get_kcore_registers().
 */
void
fetch_kcore_registers (pcb)
     struct pcb *pcb;
{

  /* FIXME */
  registers_fetched ();
}
#endif	/* FETCH_KCORE_REGISTERS */

/* Register that we are able to handle m68knbsd core file formats.
   FIXME: is this really bfd_target_unknown_flavour? */
   
static struct core_fns sh_nbsd_core_fns =
{  
  bfd_target_unknown_flavour,           /* core_flavour */
  default_check_format,                 /* check_format */
  default_core_sniffer,                 /* core_sniffer */
  fetch_core_registers,                 /* core_read_registers */
  NULL                                  /* next */
}; 

static struct core_fns sh_nbsd_elfcore_fns =
{  
  bfd_target_elf_flavour,               /* core_flavour */
  default_check_format,                 /* check_format */
  default_core_sniffer,                 /* core_sniffer */
  fetch_elfcore_registers,              /* core_read_registers */
  NULL                                  /* next */
}; 

void
_initialize_sh_nbsd_nat ()
{
  add_core_fns (&sh_nbsd_core_fns);
  add_core_fns (&sh_nbsd_elfcore_fns);
}
