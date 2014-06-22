/* Common target dependent code for GDB on ARM systems.
   Copyright (C) 2002-2014 Free Software Foundation, Inc.

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

/* Register numbers of various important registers.  */

enum gdb_regnum {
  ARM_A1_REGNUM = 0,		/* first integer-like argument */
  ARM_A4_REGNUM = 3,		/* last integer-like argument */
  ARM_AP_REGNUM = 11,
  ARM_IP_REGNUM = 12,
  ARM_SP_REGNUM = 13,		/* Contains address of top of stack */
  ARM_LR_REGNUM = 14,		/* address to return to from a function call */
  ARM_PC_REGNUM = 15,		/* Contains program counter */
  ARM_F0_REGNUM = 16,		/* first floating point register */
  ARM_F3_REGNUM = 19,		/* last floating point argument register */
  ARM_F7_REGNUM = 23, 		/* last floating point register */
  ARM_FPS_REGNUM = 24,		/* floating point status register */
  ARM_PS_REGNUM = 25,		/* Contains processor status */
  ARM_WR0_REGNUM,		/* WMMX data registers.  */
  ARM_WR15_REGNUM = ARM_WR0_REGNUM + 15,
  ARM_WC0_REGNUM,		/* WMMX control registers.  */
  ARM_WCSSF_REGNUM = ARM_WC0_REGNUM + 2,
  ARM_WCASF_REGNUM = ARM_WC0_REGNUM + 3,
  ARM_WC7_REGNUM = ARM_WC0_REGNUM + 7,
  ARM_WCGR0_REGNUM,		/* WMMX general purpose registers.  */
  ARM_WCGR3_REGNUM = ARM_WCGR0_REGNUM + 3,
  ARM_WCGR7_REGNUM = ARM_WCGR0_REGNUM + 7,
  ARM_D0_REGNUM,		/* VFP double-precision registers.  */
  ARM_D31_REGNUM = ARM_D0_REGNUM + 31,
  ARM_FPSCR_REGNUM,

  ARM_NUM_REGS,

  /* Other useful registers.  */
  ARM_FP_REGNUM = 11,		/* Frame register in ARM code, if used.  */
  THUMB_FP_REGNUM = 7,		/* Frame register in Thumb code, if used.  */
  ARM_NUM_ARG_REGS = 4, 
  ARM_LAST_ARG_REGNUM = ARM_A4_REGNUM,
  ARM_NUM_FP_ARG_REGS = 4,
  ARM_LAST_FP_ARG_REGNUM = ARM_F3_REGNUM
};

/* Size of integer registers.  */
#define INT_REGISTER_SIZE		4

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


/* Instruction condition field values.  */
#define INST_EQ		0x0
#define INST_NE		0x1
#define INST_CS		0x2
#define INST_CC		0x3
#define INST_MI		0x4
#define INST_PL		0x5
#define INST_VS		0x6
#define INST_VC		0x7
#define INST_HI		0x8
#define INST_LS		0x9
#define INST_GE		0xa
#define INST_LT		0xb
#define INST_GT		0xc
#define INST_LE		0xd
#define INST_AL		0xe
#define INST_NV		0xf

#define FLAG_N		0x80000000
#define FLAG_Z		0x40000000
#define FLAG_C		0x20000000
#define FLAG_V		0x10000000

#define CPSR_T		0x20

#define XPSR_T		0x01000000

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
  int have_vfp_registers;	/* Does the target report the VFP registers?  */
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

  /* Cached core file helpers.  */
  struct regset *gregset, *fpregset, *vfpregset;

  /* ISA-specific data types.  */
  struct type *arm_ext_type;
  struct type *neon_double_type;
  struct type *neon_quad_type;

  /* Return the expected next PC if FRAME is stopped at a syscall
     instruction.  */
  CORE_ADDR (*syscall_next_pc) (struct frame_info *frame);

   /* Parse swi insn args, sycall record.  */
  int (*arm_swi_record) (struct regcache *regcache);
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
CORE_ADDR arm_get_next_pc (struct frame_info *, CORE_ADDR);
void arm_insert_single_step_breakpoint (struct gdbarch *,
					struct address_space *, CORE_ADDR);
int arm_deal_with_atomic_sequence (struct frame_info *);
int arm_software_single_step (struct frame_info *);
int arm_frame_is_thumb (struct frame_info *frame);

extern struct displaced_step_closure *
  arm_displaced_step_copy_insn (struct gdbarch *, CORE_ADDR, CORE_ADDR,
				struct regcache *);
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

extern const struct regset *
  armbsd_regset_from_core_section (struct gdbarch *gdbarch,
				   const char *sect_name, size_t sect_size);

/* Target descriptions.  */
extern struct target_desc *tdesc_arm_with_m;
extern struct target_desc *tdesc_arm_with_iwmmxt;
extern struct target_desc *tdesc_arm_with_vfpv2;
extern struct target_desc *tdesc_arm_with_vfpv3;
extern struct target_desc *tdesc_arm_with_neon;

#endif /* arm-tdep.h */
