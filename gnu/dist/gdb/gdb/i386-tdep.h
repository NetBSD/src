/* Target-dependent code for GDB, the GNU debugger.
   Copyright 2001, 2002
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

#ifndef I386_TDEP_H
#define I386_TDEP_H

#include "osabi.h"

/* GDB's i386 target supports both the 32-bit Intel Architecture
   (IA-32) and the 64-bit AMD x86-64 architecture.  Internally it uses
   a similar register layout for both.

   - General purpose registers
   - FPU data registers
   - FPU control registers
   - SSE data registers
   - SSE control register

   The general purpose registers for the x86-64 architecture are quite
   different from IA-32.  Therefore, the FP0_REGNUM target macro
   determines the register number at which the FPU data registers
   start.  The number of FPU data and control registers is the same
   for both architectures.  The number of SSE registers however,
   differs and is determined by the num_xmm_regs member of `struct
   gdbarch_tdep'.  */

/* Convention for returning structures.  */

enum struct_return
{
  pcc_struct_return,		/* Return "short" structures in memory.  */
  reg_struct_return		/* Return "short" structures in registers.  */
};

/* i386 architecture specific information.  */
struct gdbarch_tdep
{
  /* ABI.  */
  enum gdb_osabi osabi;

  /* Number of SSE registers.  */
  int num_xmm_regs;

  /* Offset of saved PC in jmp_buf.  */
  int jb_pc_offset;

  /* Convention for returning structures.  */
  enum struct_return struct_return;

  /* Address range where sigtramp lives.  */
  CORE_ADDR sigtramp_start;
  CORE_ADDR sigtramp_end;

  /* Get address of sigcontext for sigtramp.  */
  CORE_ADDR (*sigcontext_addr) (struct frame_info *);

  /* Offset of saved PC and SP in `struct sigcontext'.  */
  int sc_pc_offset;
  int sc_sp_offset;
};

/* Floating-point registers.  */

#define FPU_REG_RAW_SIZE 10

/* All FPU control regusters (except for FIOFF and FOOFF) are 16-bit
   (at most) in the FPU, but are zero-extended to 32 bits in GDB's
   register cache.  */

/* "Generic" floating point control register.  */
#define FPC_REGNUM	(FP0_REGNUM + 8)

/* FPU control word.  */
#define FCTRL_REGNUM	FPC_REGNUM

/* FPU status word.  */
#define FSTAT_REGNUM	(FPC_REGNUM + 1)

/* FPU register tag word.  */
#define FTAG_REGNUM	(FPC_REGNUM + 2)

/* FPU instruction's code segment selector, called "FPU Instruction
   Pointer Selector" in the IA-32 manuals.  */
#define FISEG_REGNUM	(FPC_REGNUM + 3)

/* FPU instruction's offset within segment.  */
#define FIOFF_REGNUM	(FPC_REGNUM + 4)

/* FPU operand's data segment.  */
#define FOSEG_REGNUM	(FPC_REGNUM + 5)

/* FPU operand's offset within segment */
#define FOOFF_REGNUM	(FPC_REGNUM + 6)

/* FPU opcode, bottom eleven bits.  */
#define FOP_REGNUM	(FPC_REGNUM + 7)

/* Return non-zero if N corresponds to a FPU data registers.  */
#define FP_REGNUM_P(n)	(FP0_REGNUM && FP0_REGNUM <= (n) && (n) < FPC_REGNUM)

/* Return non-zero if N corresponds to a FPU control register.  */
#define FPC_REGNUM_P(n)	(FPC_REGNUM <= (n) && (n) < XMM0_REGNUM)

/* SSE registers.  */

/* First SSE data register.  */
#define XMM0_REGNUM	(FPC_REGNUM + 8)

/* SSE control/status register.  */
#define MXCSR_REGNUM \
  (XMM0_REGNUM + gdbarch_tdep (current_gdbarch)->num_xmm_regs)

/* Return non-zero if N corresponds to a SSE data register.  */
#define SSE_REGNUM_P(n) (XMM0_REGNUM <= (n) && (n) < MXCSR_REGNUM)

/* FIXME: kettenis/2001-11-24: Obsolete macro's.  */
#define FCS_REGNUM FISEG_REGNUM
#define FCOFF_REGNUM FIOFF_REGNUM
#define FDS_REGNUM FOSEG_REGNUM
#define FDOFF_REGNUM FOOFF_REGNUM
#define IS_FP_REGNUM(n) FP_REGNUM_P (n)
#define IS_FPU_CTRL_REGNUM(n) FPC_REGNUM_P (n)
#define IS_SSE_REGNUM(n) SSE_REGNUM_P (n)

#define I386_NUM_GREGS	16
#define I386_NUM_FREGS	16
#define I386_NUM_XREGS  9

#define I386_SSE_NUM_REGS	(I386_NUM_GREGS + I386_NUM_FREGS \
				 + I386_NUM_XREGS)

/* Sizes of individual register sets.  These cover the entire register
   file, so summing up the sizes of those portions actually present
   yields REGISTER_BYTES.  */
#define I386_SIZEOF_GREGS	(I386_NUM_GREGS * 4)
#define I386_SIZEOF_FREGS	(8 * 10 + 8 * 4)
#define I386_SIZEOF_XREGS	(8 * 16 + 4)

#define I386_SSE_SIZEOF_REGS	(I386_SIZEOF_GREGS + I386_SIZEOF_FREGS \
				 + I386_SIZEOF_XREGS)

/* Size of the largest register.  */
#define I386_MAX_REGISTER_SIZE	16

/* Functions exported from i386-tdep.c.  */
extern CORE_ADDR i386_pe_skip_trampoline_code (CORE_ADDR pc, char *name);

/* Return the name of register REG.  */
extern char const *i386_register_name (int reg);

/* Initialize a basic ELF architecture variant.  */
extern void i386_elf_init_abi (struct gdbarch_info, struct gdbarch *);

/* Initialize a SVR4 architecture variant.  */
extern void i386_svr4_init_abi (struct gdbarch_info, struct gdbarch *);

/* Functions exported from i386bsd-tdep.c.  */

extern CORE_ADDR i386bsd_sigcontext_addr (struct frame_info *frame);
extern void i386bsd_init_abi (struct gdbarch_info, struct gdbarch *);

#endif /* i386-tdep.h */
