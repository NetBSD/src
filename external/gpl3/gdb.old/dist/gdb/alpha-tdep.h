/* Common target dependent code for GDB on Alpha systems.
   Copyright (C) 1993-2023 Free Software Foundation, Inc.

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

#ifndef ALPHA_TDEP_H
#define ALPHA_TDEP_H

#include "gdbarch.h"

struct regcache;

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places; register_size() is the
   real way to know how big a register is.  */
#define ALPHA_REGISTER_SIZE 8

/* Number of machine registers.  */
#define ALPHA_NUM_REGS 67

/* Register numbers of various important registers.  Note that most of
   these values are "real" register numbers, and correspond to the
   general registers of the machine.  */

#define ALPHA_V0_REGNUM	     0  /* Function integer return value */
#define ALPHA_T7_REGNUM	     8  /* Return address register for OSF/1 __add* */
#define ALPHA_S0_REGNUM      9	/* First saved register */
#define ALPHA_GCC_FP_REGNUM 15  /* Used by gcc as frame register */
#define ALPHA_A0_REGNUM     16  /* Loc of first arg during a subr call */
#define ALPHA_T9_REGNUM     23  /* Return address register for OSF/1 __div* */
#define ALPHA_RA_REGNUM     26  /* Contains return address value */
#define ALPHA_T12_REGNUM    27  /* Contains start addr of current proc */
#define ALPHA_GP_REGNUM     29  /* Contains the global pointer */
#define ALPHA_SP_REGNUM     30  /* Contains address of top of stack */
#define ALPHA_ZERO_REGNUM   31  /* Read-only register, always 0 */
#define ALPHA_FP0_REGNUM    32  /* Floating point register 0 */
#define ALPHA_FPA0_REGNUM   48  /* First float arg during a subr call */
#define ALPHA_FPCR_REGNUM   63  /* Floating point control register */
#define ALPHA_PC_REGNUM     64  /* Contains program counter */
#define ALPHA_UNIQUE_REGNUM 66	/* PAL_rduniq value */

/* Instruction size.  */
#define ALPHA_INSN_SIZE	4

/* The alpha has two different virtual pointers for arguments and locals.
   
   The virtual argument pointer is pointing to the bottom of the argument
   transfer area, which is located immediately below the virtual frame
   pointer.  Its size is fixed for the native compiler, it is either zero
   (for the no arguments case) or large enough to hold all argument registers.
   gcc uses a variable sized argument transfer area.  As it has
   to stay compatible with the native debugging tools it has to use the same
   virtual argument pointer and adjust the argument offsets accordingly.
   
   The virtual local pointer is localoff bytes below the virtual frame
   pointer, the value of localoff is obtained from the PDR.  */
#define ALPHA_NUM_ARG_REGS   6

/* Target-dependent structure in gdbarch.  */
struct alpha_gdbarch_tdep : gdbarch_tdep_base
{
  CORE_ADDR vm_min_address = 0;	/* Used by alpha_heuristic_proc_start.  */

  /* If PC is inside a dynamically-generated signal trampoline function
     (i.e. one copied onto the user stack at run-time), return how many
     bytes PC is beyond the start of that function.  Otherwise, return -1.  */
  LONGEST (*dynamic_sigtramp_offset) (struct gdbarch *, CORE_ADDR) = nullptr;

  /* Translate a signal handler stack base address into the address of
     the sigcontext structure for that signal handler.  */
  CORE_ADDR (*sigcontext_addr) (frame_info_ptr) = nullptr;

  /* Does the PC fall in a signal trampoline.  */
  /* NOTE: cagney/2004-04-30: Do not copy/clone this code.  Instead
     look at tramp-frame.h and other simpler per-architecture
     sigtramp unwinders.  */
  int (*pc_in_sigtramp) (struct gdbarch *gdbarch, CORE_ADDR pc,
			 const char *name) = nullptr;

  /* If TYPE will be returned in memory, return true.  */
  int (*return_in_memory) (struct type *type) = nullptr;

  /* Offset of registers in `struct sigcontext'.  */
  int sc_pc_offset = 0;
  int sc_regs_offset = 0;
  int sc_fpregs_offset = 0;

  int jb_pc = 0;			/* Offset to PC value in jump buffer.
				   If htis is negative, longjmp support
				   will be disabled.  */
  size_t jb_elt_size = 0;		/* And the size of each entry in the buf.  */
};

extern unsigned int alpha_read_insn (struct gdbarch *gdbarch, CORE_ADDR pc);
extern std::vector<CORE_ADDR> alpha_software_single_step
  (struct regcache *regcache);
extern CORE_ADDR alpha_after_prologue (CORE_ADDR pc);

extern void alpha_mdebug_init_abi (struct gdbarch_info, struct gdbarch *);
extern void alpha_dwarf2_init_abi (struct gdbarch_info, struct gdbarch *);

extern void alpha_supply_int_regs (struct regcache *, int, const void *,
				   const void *, const void *);
extern void alpha_fill_int_regs (const struct regcache *, int,
				 void *, void *, void *);
extern void alpha_supply_fp_regs (struct regcache *, int,
				  const void *, const void *);
extern void alpha_fill_fp_regs (const struct regcache *,
				int, void *, void *);

#endif /* ALPHA_TDEP_H */
