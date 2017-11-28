/* Common target dependent code for GDB on ARM systems.
   Copyright (C) 2002-2016 Free Software Foundation, Inc.

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

#ifndef ARM_TDEP_H
#define ARM_TDEP_H

/* Forward declarations.  */
struct gdbarch;
struct regset;
struct address_space;
struct get_next_pcs;
struct arm_get_next_pcs;
struct gdb_get_next_pcs;

#include "arch/arm.h"

/* Say how long FP registers are.  Used for documentation purposes and
   code readability in this header.  IEEE extended doubles are 80
   bits.  DWORD aligned they use 96 bits.  */
#define FP_REGISTER_SIZE	12

/* Say how long VFP double precision registers are.  Used for documentation
   purposes and code readability.  These are fixed at 64 bits.  */
#define VFP_REGISTER_SIZE	8

/* Number of machine registers.  The only define actually required 
   is gdbarch_num_regs.  The other definitions are used for documentation
   purposes and code readability.  */
/* For 26 bit ARM code, a fake copy of the PC is placed in register 25 (PS)
   (and called PS for processor status) so the status bits can be cleared
   from the PC (register 15).  For 32 bit ARM code, a copy of CPSR is placed
   in PS.  */
#define NUM_FREGS	8	/* Number of floating point registers.  */
#define NUM_SREGS	2	/* Number of status registers.  */
#define NUM_GREGS	16	/* Number of general purpose registers.  */



/* Type of floating-point code in use by inferior.  There are really 3 models
   that are traditionally supported (plus the endianness issue), but gcc can
   only generate 2 of those.  The third is APCS_FLOAT, where arguments to
   functions are passed in floating-point registers.  

   In addition to the traditional models, VFP adds two more. 

   If you update this enum, don't forget to update fp_model_strings in 
   arm-tdep.c.  */

enum arm_float_model
{
  ARM_FLOAT_AUTO,	/* Automatic detection.  Do not set in tdep.  */
  ARM_FLOAT_SOFT_FPA,	/* Traditional soft-float (mixed-endian on LE ARM).  */
  ARM_FLOAT_FPA,	/* FPA co-processor.  GCC calling convention.  */
  ARM_FLOAT_SOFT_VFP,	/* Soft-float with pure-endian doubles.  */
  ARM_FLOAT_VFP,	/* Full VFP calling convention.  */
  ARM_FLOAT_LAST	/* Keep at end.  */
};

/* ABI used by the inferior.  */
enum arm_abi_kind
{
  ARM_ABI_AUTO,
  ARM_ABI_APCS,
  ARM_ABI_AAPCS,
  ARM_ABI_LAST
};

/* Convention for returning structures.  */

enum struct_return
{
  pcc_struct_return,		/* Return "short" structures in memory.  */
  reg_struct_return		/* Return "short" structures in registers.  */
};

/* Target-dependent structure in gdbarch.  */
struct gdbarch_tdep
{
  /* The ABI for this architecture.  It should never be set to
     ARM_ABI_AUTO.  */
  enum arm_abi_kind arm_abi;

  enum arm_float_model fp_model; /* Floating point calling conventions.  */

  int have_fpa_registers;	/* Does the target report the FPA registers?  */
  int have_wmmx_registers;	/* Does the target report the WMMX registers?  */
  /* The number of VFP registers reported by the target.  It is zero
     if VFP registers are not supported.  */
  int vfp_register_count;
  int have_vfp_pseudos;		/* Are we synthesizing the single precision
				   VFP registers?  */
  int have_neon_pseudos;	/* Are we synthesizing the quad precision
				   NEON registers?  Requires
				   have_vfp_pseudos.  */
  int have_neon;		/* Do we have a NEON unit?  */

  int is_m;			/* Does the target follow the "M" profile.  */
  CORE_ADDR lowest_pc;		/* Lowest address at which instructions 
				   will appear.  */

  const gdb_byte *arm_breakpoint;	/* Breakpoint pattern for an ARM insn.  */
  int arm_breakpoint_size;	/* And its size.  */
  const gdb_byte *thumb_breakpoint;	/* Breakpoint pattern for a Thumb insn.  */
  int thumb_breakpoint_size;	/* And its size.  */

  /* If the Thumb breakpoint is an undefined instruction (which is
     affected by IT blocks) rather than a BKPT instruction (which is
     not), then we need a 32-bit Thumb breakpoint to preserve the
     instruction count in IT blocks.  */
  const gdb_byte *thumb2_breakpoint;
  int thumb2_breakpoint_size;

  int jb_pc;			/* Offset to PC value in jump buffer.
				   If this is negative, longjmp support
				   will be disabled.  */
  size_t jb_elt_size;		/* And the size of each entry in the buf.  */

  /* Convention for returning structures.  */
  enum struct_return struct_return;

  /* ISA-specific data types.  */
  struct type *arm_ext_type;
  struct type *neon_double_type;
  struct type *neon_quad_type;

   /* syscall record.  */
  int (*arm_syscall_record) (struct regcache *regcache, unsigned long svc_number);
};

/* Structures used for displaced stepping.  */

/* The maximum number of temporaries available for displaced instructions.  */
#define DISPLACED_TEMPS			16
/* The maximum number of modified instructions generated for one single-stepped
   instruction, including the breakpoint (usually at the end of the instruction
   sequence) and any scratch words, etc.  */
#define DISPLACED_MODIFIED_INSNS	8

struct displaced_step_closure
{
  ULONGEST tmp[DISPLACED_TEMPS];
  int rd;
  int wrote_to_pc;
  union
  {
    struct
    {
      int xfersize;
      int rn;			   /* Writeback register.  */
      unsigned int immed : 1;      /* Offset is immediate.  */
      unsigned int writeback : 1;  /* Perform base-register writeback.  */
      unsigned int restore_r4 : 1; /* Used r4 as scratch.  */
    } ldst;

    struct
    {
      unsigned long dest;
      unsigned int link : 1;
      unsigned int exchange : 1;
      unsigned int cond : 4;
    } branch;

    struct
    {
      unsigned int regmask;
      int rn;
      CORE_ADDR xfer_addr;
      unsigned int load : 1;
      unsigned int user : 1;
      unsigned int increment : 1;
      unsigned int before : 1;
      unsigned int writeback : 1;
      unsigned int cond : 4;
    } block;

    struct
    {
      unsigned int immed : 1;
    } preload;

    struct
    {
      /* If non-NULL, override generic SVC handling (e.g. for a particular
         OS).  */
      int (*copy_svc_os) (struct gdbarch *gdbarch, struct regcache *regs,
			  struct displaced_step_closure *dsc);
    } svc;
  } u;

  /* The size of original instruction, 2 or 4.  */
  unsigned int insn_size;
  /* True if the original insn (and thus all replacement insns) are Thumb
     instead of ARM.   */
  unsigned int is_thumb;

  /* The slots in the array is used in this way below,
     - ARM instruction occupies one slot,
     - Thumb 16 bit instruction occupies one slot,
     - Thumb 32-bit instruction occupies *two* slots, one part for each.  */
  unsigned long modinsn[DISPLACED_MODIFIED_INSNS];
  int numinsns;
  CORE_ADDR insn_addr;
  CORE_ADDR scratch_base;
  void (*cleanup) (struct gdbarch *, struct regcache *,
		   struct displaced_step_closure *);
};

/* Values for the WRITE_PC argument to displaced_write_reg.  If the register
   write may write to the PC, specifies the way the CPSR T bit, etc. is
   modified by the instruction.  */

enum pc_write_style
{
  BRANCH_WRITE_PC,
  BX_WRITE_PC,
  LOAD_WRITE_PC,
  ALU_WRITE_PC,
  CANNOT_WRITE_PC
};

extern void
  arm_process_displaced_insn (struct gdbarch *gdbarch, CORE_ADDR from,
			      CORE_ADDR to, struct regcache *regs,
			      struct displaced_step_closure *dsc);
extern void
  arm_displaced_init_closure (struct gdbarch *gdbarch, CORE_ADDR from,
			      CORE_ADDR to, struct displaced_step_closure *dsc);
extern ULONGEST
  displaced_read_reg (struct regcache *regs, struct displaced_step_closure *dsc,
		      int regno);
extern void
  displaced_write_reg (struct regcache *regs,
		       struct displaced_step_closure *dsc, int regno,
		       ULONGEST val, enum pc_write_style write_pc);

CORE_ADDR arm_skip_stub (struct frame_info *, CORE_ADDR);

ULONGEST arm_get_next_pcs_read_memory_unsigned_integer (CORE_ADDR memaddr,
							int len,
							int byte_order);

CORE_ADDR arm_get_next_pcs_addr_bits_remove (struct arm_get_next_pcs *self,
					     CORE_ADDR val);

int arm_get_next_pcs_is_thumb (struct arm_get_next_pcs *self);

void arm_insert_single_step_breakpoint (struct gdbarch *,
					struct address_space *, CORE_ADDR);
int arm_software_single_step (struct frame_info *);
int arm_is_thumb (struct regcache *regcache);
int arm_frame_is_thumb (struct frame_info *frame);

extern void arm_displaced_step_fixup (struct gdbarch *,
				      struct displaced_step_closure *,
				      CORE_ADDR, CORE_ADDR, struct regcache *);

/* Return the bit mask in ARM_PS_REGNUM that indicates Thumb mode.  */
extern int arm_psr_thumb_bit (struct gdbarch *);

/* Is the instruction at the given memory address a Thumb or ARM
   instruction?  */
extern int arm_pc_is_thumb (struct gdbarch *, CORE_ADDR);

extern int arm_process_record (struct gdbarch *gdbarch, 
                               struct regcache *regcache, CORE_ADDR addr);
/* Functions exported from armbsd-tdep.h.  */

/* Return the appropriate register set for the core section identified
   by SECT_NAME and SECT_SIZE.  */

extern void
  armbsd_iterate_over_regset_sections (struct gdbarch *gdbarch,
				       iterate_over_regset_sections_cb *cb,
				       void *cb_data,
				       const struct regcache *regcache);

/* Target descriptions.  */
extern struct target_desc *tdesc_arm_with_m;
extern struct target_desc *tdesc_arm_with_iwmmxt;
extern struct target_desc *tdesc_arm_with_vfpv2;
extern struct target_desc *tdesc_arm_with_vfpv3;
extern struct target_desc *tdesc_arm_with_neon;

#endif /* arm-tdep.h */
