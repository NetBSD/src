/* Native-dependent code for NetBSD/i386, for GDB.
   Copyright 1988, 1989, 1991, 1992, 1994, 1996, 2000
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
#include "inferior.h"
#include "gdbcore.h" /* for registers_fetched() */

#define RF(dst, src) \
	memcpy(&registers[REGISTER_BYTE(dst)], &src, sizeof(src))

#define RS(src, dst) \
	memcpy(&dst, &registers[REGISTER_BYTE(src)], sizeof(dst))
     
struct env387
  {
    unsigned short control;
    unsigned short r0;
    unsigned short status;
    unsigned short r1;
    unsigned short tag;  
    unsigned short r2;
    unsigned long eip;
    unsigned short code_seg;
    unsigned short opcode;
    unsigned long operand; 
    unsigned short operand_seg;
    unsigned short r3;
    unsigned char regs[8][10];
  };

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct env387 inferior_fpregisters;

  ptrace (PT_GETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);
  ptrace (PT_GETFPREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_fpregisters, 0);

  RF ( 0, inferior_registers.r_eax);
  RF ( 1, inferior_registers.r_ecx);
  RF ( 2, inferior_registers.r_edx);
  RF ( 3, inferior_registers.r_ebx);
  RF ( 4, inferior_registers.r_esp);
  RF ( 5, inferior_registers.r_ebp);
  RF ( 6, inferior_registers.r_esi);
  RF ( 7, inferior_registers.r_edi);
  RF ( 8, inferior_registers.r_eip);
  RF ( 9, inferior_registers.r_eflags);
  RF (10, inferior_registers.r_cs);
  RF (11, inferior_registers.r_ss);
  RF (12, inferior_registers.r_ds);
  RF (13, inferior_registers.r_es);
  RF (14, inferior_registers.r_fs);
  RF (15, inferior_registers.r_gs);

  RF (FP0_REGNUM,     inferior_fpregisters.regs[0]);
  RF (FP0_REGNUM + 1, inferior_fpregisters.regs[1]);
  RF (FP0_REGNUM + 2, inferior_fpregisters.regs[2]);
  RF (FP0_REGNUM + 3, inferior_fpregisters.regs[3]);
  RF (FP0_REGNUM + 4, inferior_fpregisters.regs[4]);
  RF (FP0_REGNUM + 5, inferior_fpregisters.regs[5]);
  RF (FP0_REGNUM + 6, inferior_fpregisters.regs[6]);
  RF (FP0_REGNUM + 7, inferior_fpregisters.regs[7]);

  RF (FCTRL_REGNUM,   inferior_fpregisters.control);
  RF (FSTAT_REGNUM,   inferior_fpregisters.status);
  RF (FTAG_REGNUM,    inferior_fpregisters.tag);
  RF (FCS_REGNUM,     inferior_fpregisters.code_seg);
  RF (FCOFF_REGNUM,   inferior_fpregisters.eip);
  RF (FDS_REGNUM,     inferior_fpregisters.operand_seg);
  RF (FDOFF_REGNUM,   inferior_fpregisters.operand);
  RF (FOP_REGNUM,     inferior_fpregisters.opcode);

  registers_fetched ();
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct env387 inferior_fpregisters;

  RS ( 0, inferior_registers.r_eax);
  RS ( 1, inferior_registers.r_ecx);
  RS ( 2, inferior_registers.r_edx);
  RS ( 3, inferior_registers.r_ebx);
  RS ( 4, inferior_registers.r_esp);
  RS ( 5, inferior_registers.r_ebp);
  RS ( 6, inferior_registers.r_esi);
  RS ( 7, inferior_registers.r_edi);
  RS ( 8, inferior_registers.r_eip);
  RS ( 9, inferior_registers.r_eflags);
  RS (10, inferior_registers.r_cs);
  RS (11, inferior_registers.r_ss);
  RS (12, inferior_registers.r_ds);
  RS (13, inferior_registers.r_es);
  RS (14, inferior_registers.r_fs);
  RS (15, inferior_registers.r_gs);

  
  RS (FP0_REGNUM,     inferior_fpregisters.regs[0]);
  RS (FP0_REGNUM + 1, inferior_fpregisters.regs[1]);
  RS (FP0_REGNUM + 2, inferior_fpregisters.regs[2]);
  RS (FP0_REGNUM + 3, inferior_fpregisters.regs[3]);
  RS (FP0_REGNUM + 4, inferior_fpregisters.regs[4]);
  RS (FP0_REGNUM + 5, inferior_fpregisters.regs[5]);
  RS (FP0_REGNUM + 6, inferior_fpregisters.regs[6]);
  RS (FP0_REGNUM + 7, inferior_fpregisters.regs[7]);

  RS (FCTRL_REGNUM,   inferior_fpregisters.control);
  RS (FSTAT_REGNUM,   inferior_fpregisters.status);
  RS (FTAG_REGNUM,    inferior_fpregisters.tag);
  RS (FCS_REGNUM,     inferior_fpregisters.code_seg);
  RS (FCOFF_REGNUM,   inferior_fpregisters.eip);
  RS (FDS_REGNUM,     inferior_fpregisters.operand_seg);
  RS (FDOFF_REGNUM,   inferior_fpregisters.operand);
  RS (FOP_REGNUM,     inferior_fpregisters.opcode);
  
  ptrace (PT_SETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);
  ptrace (PT_SETFPREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_fpregisters, 0);
}

int
i386nbsd_use_struct_convention (int gcc_p, struct type *type)
{
  return !(TYPE_LENGTH (type) == 1
	   || TYPE_LENGTH (type) == 2
	   || TYPE_LENGTH (type) == 4
	   || TYPE_LENGTH (type) == 8);
}

struct md_core
{
  struct reg intreg;
  struct env387 freg;
};

static void
fetch_core_registers (core_reg_sect, core_reg_size, which, ignore)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR ignore;
{
  struct md_core *core_reg = (struct md_core *) core_reg_sect;

  /* integer registers */
  memcpy (&registers[REGISTER_BYTE (0)], &core_reg->intreg,
	  sizeof (struct reg));

  /* floating point registers */
  RF (FP0_REGNUM,     core_reg->freg.regs[0]);
  RF (FP0_REGNUM + 1, core_reg->freg.regs[1]);
  RF (FP0_REGNUM + 2, core_reg->freg.regs[2]);
  RF (FP0_REGNUM + 3, core_reg->freg.regs[3]);
  RF (FP0_REGNUM + 4, core_reg->freg.regs[4]);
  RF (FP0_REGNUM + 5, core_reg->freg.regs[5]);
  RF (FP0_REGNUM + 6, core_reg->freg.regs[6]);
  RF (FP0_REGNUM + 7, core_reg->freg.regs[7]);

  RF (FCTRL_REGNUM,   core_reg->freg.control);
  RF (FSTAT_REGNUM,   core_reg->freg.status);
  RF (FTAG_REGNUM,    core_reg->freg.tag);
  RF (FCS_REGNUM,     core_reg->freg.code_seg);
  RF (FCOFF_REGNUM,   core_reg->freg.eip);
  RF (FDS_REGNUM,     core_reg->freg.operand_seg);
  RF (FDOFF_REGNUM,   core_reg->freg.operand);
  RF (FOP_REGNUM,     core_reg->freg.opcode);

  registers_fetched ();
}

/* Register that we are able to handle i386nbsd core file formats.
   FIXME: is this really bfd_target_unknown_flavour? */

static struct core_fns i386nbsd_core_fns =
{
  bfd_target_unknown_flavour,		/* core_flavour */
  default_check_format,			/* check_format */
  default_core_sniffer,			/* core_sniffer */
  fetch_core_registers,			/* core_read_registers */
  NULL					/* next */
};

void
_initialize_i386nbsd_nat ()
{
  add_core_fns (&i386nbsd_core_fns);
}
