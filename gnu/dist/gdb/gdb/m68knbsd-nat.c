/* Native-dependent code for Motorola m68k's running NetBSD, for GDB.
   Copyright 1988, 1989, 1991, 1992, 1994, 1996, 2000, 2001
   Free Software Foundation, Inc.

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

#include <sys/types.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/pcb.h>

#include "inferior.h"
#include "gdbcore.h"
#include "regcache.h"

/* Map GDB register index to ptrace register buffer offset. */
static const int regmap[] =
{
  0x00, /* d0 */
  0x04, /* d1 */
  0x08, /* d2 */
  0x0c, /* d3 */
  0x10, /* d4 */
  0x14, /* d5 */
  0x18, /* d6 */
  0x1c, /* d7 */
  0x20, /* a0 */
  0x24, /* a1 */
  0x28, /* a2 */
  0x2c, /* a3 */
  0x30, /* a4 */
  0x34, /* a5 */
  0x38, /* a6 */
  0x3c, /* a7 */
  0x40, /* sr */
  0x44, /* pc */
};

/* Map GDB FP register index to ptrace FP register buffer offset. */
static const int fpregmap[] =
{
  0x00, /* fp0 */
  0x0c, /* fp1 */
  0x18, /* fp2 */
  0x24, /* fp3 */
  0x30, /* fp4 */
  0x3c, /* fp5 */
  0x48, /* fp6 */
  0x54, /* fp7 */
  0x60, /* fpcr */
  0x64, /* fpsr */
  0x68, /* fpiar */
};

#define FPI_REGNUM 28

/* Determine if PT_GETREGS fetches this register. */
#define GETREGS_SUPPLIES(regno) \
  ((regno) >= D0_REGNUM && (regno) <= PC_REGNUM)

static void
supply_regs (regs)
     char *regs;
{
  int i;

  for (i = D0_REGNUM; i <= PC_REGNUM; i++)
    supply_register (i, regs + regmap[i - D0_REGNUM]);
}

static void
supply_fpregs (freg)
     char *freg;
{
  int i;

  for (i = FP0_REGNUM; i <= FPI_REGNUM; i++)
    supply_register (i, freg + fpregmap[i - FP0_REGNUM]);
}

static void
fill_regs (regs)
     char *regs;
{
  int i;

  for (i = D0_REGNUM; i <= PC_REGNUM; i++)
    *(int *)(regs + regmap[i - D0_REGNUM]) = *(int *) &registers[REGISTER_BYTE (i)];
}

static void
fill_fpregs(freg)
     char *freg;
{
  int i;
  char *to;
  char *from;

  for (i = FP0_REGNUM; i <= FPI_REGNUM; i++)
    {
      from = (char *) &registers[REGISTER_BYTE (i)];
      to = (char *) (freg + fpregmap[i - FP0_REGNUM]);
      memcpy (to, from, REGISTER_RAW_SIZE (i));
    }
}

void
fetch_inferior_registers (int regno)
{
  struct reg inferior_registers;
  struct fpreg inferior_fp_registers;

  if (regno == -1 || GETREGS_SUPPLIES (regno))
    {
      ptrace (PT_GETREGS, PIDGET (inferior_ptid),
	      (PTRACE_ARG3_TYPE) & inferior_registers, 0);
      supply_regs ((char *) &inferior_registers);
      
      if (regno != -1)
	return;
    }

  ptrace (PT_GETFPREGS, PIDGET (inferior_ptid),
	  (PTRACE_ARG3_TYPE) & inferior_fp_registers, 0);
  supply_fpregs ((char *) &inferior_fp_registers);
}

void
store_inferior_registers (int regno)
{
  struct reg inferior_registers;
  struct fpreg inferior_fp_registers;

  memcpy (&inferior_registers, &registers[REGISTER_BYTE (0)],
	  sizeof (inferior_registers));
  ptrace (PT_SETREGS, PIDGET (inferior_ptid),
	  (PTRACE_ARG3_TYPE) & inferior_registers, 0);

  memcpy (&inferior_fp_registers, &registers[REGISTER_BYTE (FP0_REGNUM)],
	  sizeof (inferior_fp_registers));
  ptrace (PT_SETFPREGS, PIDGET (inferior_ptid),
	  (PTRACE_ARG3_TYPE) & inferior_fp_registers, 0);
}

struct md_core
{
  struct reg intreg;
  struct fpreg freg;
};

static void
fetch_core_registers (char *core_reg_sect, unsigned core_reg_size, int which,
		      CORE_ADDR ignore)
{
  struct md_core *core_reg = (struct md_core *) core_reg_sect;

  /* Integer registers */
  supply_regs ((char *) &core_reg->intreg);
  /* Floating point registers */
  supply_fpregs ((char *) &core_reg->freg);
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

    case 2:  /* Floating point registers */
      if (core_reg_size != sizeof (struct fpreg))
	warning ("Wrong size FP register set in core file.");
      else
	supply_fpregs (core_reg_sect);
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
  int i, *ip, tmp=0;

  /* D0,D1 */
  ip = &tmp;
  supply_register(0, (char *)ip);
  supply_register(1, (char *)ip);
  /* D2-D7 */
  ip = &pcb->pcb_regs[0];
  for (i = 2; i < 8; i++, ip++)
    supply_register(i, (char *)ip);

  /* A0,A1 */
  ip = &tmp;
  supply_register(8, (char *)ip);
  supply_register(9, (char *)ip);
  /* A2-A7 */
  ip = &pcb->pcb_regs[6];
  for (i = 10; i < 16; i++, (char *)ip++)
    supply_register(i, (char *)ip);

  /* PS (sr) */
  tmp = pcb->pcb_ps & 0xFFFF;
  supply_register(PS_REGNUM, (char *)&tmp);

  /* PC (use return address) */
  tmp = pcb->pcb_regs[10] + 4;
  if (target_read_memory(tmp, (char *)&tmp, sizeof(tmp)))
    tmp = 0;
  supply_register(PC_REGNUM, (char *)&tmp);
}
#endif	/* FETCH_KCORE_REGISTERS */

/* Register that we are able to handle m68knbsd core file formats.
   FIXME: is this really bfd_target_unknown_flavour? */
   
static struct core_fns m68knbsd_core_fns =
{  
  bfd_target_unknown_flavour,           /* core_flavour */
  default_check_format,                 /* check_format */
  default_core_sniffer,                 /* core_sniffer */
  fetch_core_registers,                 /* core_read_registers */
  NULL                                  /* next */
}; 
   
static struct core_fns m68knbsd_elfcore_fns =
{  
  bfd_target_elf_flavour,               /* core_flavour */
  default_check_format,                 /* check_format */
  default_core_sniffer,                 /* core_sniffer */
  fetch_elfcore_registers,              /* core_read_registers */
  NULL                                  /* next */
}; 

void
_initialize_m68knbsd_nat (void)
{
  add_core_fns (&m68knbsd_core_fns);
  add_core_fns (&m68knbsd_elfcore_fns);
}

void
nbsd_reg_to_internal (rgs)
	char *rgs;
{
  supply_regs(rgs);
}

void
nbsd_fpreg_to_internal (frgs)
	char *frgs;
{
  supply_fpregs(frgs);
}

void
nbsd_internal_to_reg (regs)
	char *regs;
{
  fill_regs(regs);
}

void
nbsd_internal_to_fpreg (fpregs)
	char *fpregs;
{
  fill_fpregs(fpregs);
}

