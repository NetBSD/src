/* Target-dependent code for the Xtensa port of GDB, the GNU debugger.

   Copyright (C) 2003-2016 Free Software Foundation, Inc.

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


#include "arch/xtensa.h"

/* XTENSA_TDEP_VERSION can/should be changed along with XTENSA_CONFIG_VERSION
   whenever the "tdep" structure changes in an incompatible way.  */

#define XTENSA_TDEP_VERSION 0x60

/*  Xtensa register type.  */

typedef enum 
{
  xtRegisterTypeArRegfile = 1,	/* Register File ar0..arXX.  */
  xtRegisterTypeSpecialReg,	/* CPU states, such as PS, Booleans, (rsr).  */
  xtRegisterTypeUserReg,	/* User defined registers (rur).  */
  xtRegisterTypeTieRegfile,	/* User define register files.  */
  xtRegisterTypeTieState,	/* TIE States (mapped on user regs).  */
  xtRegisterTypeMapped,		/* Mapped on Special Registers.  */
  xtRegisterTypeUnmapped,	/* Special case of masked registers.  */
  xtRegisterTypeWindow,		/* Live window registers (a0..a15).  */
  xtRegisterTypeVirtual,	/* PC, FP.  */
  xtRegisterTypeUnknown
} xtensa_register_type_t;


/*  Xtensa register group.  */

#define XTENSA_MAX_COPROCESSOR	0x10  /* Number of Xtensa coprocessors.  */

typedef enum 
{
  xtRegisterGroupUnknown = 0,
  xtRegisterGroupRegFile	= 0x0001,    /* Register files without ARx.  */
  xtRegisterGroupAddrReg	= 0x0002,    /* ARx.  */
  xtRegisterGroupSpecialReg	= 0x0004,    /* SRxx.  */
  xtRegisterGroupUserReg	= 0x0008,    /* URxx.  */
  xtRegisterGroupState 		= 0x0010,    /* States.  */

  xtRegisterGroupGeneral	= 0x0100,    /* General registers, Ax, SR.  */
  xtRegisterGroupUser		= 0x0200,    /* User registers.  */
  xtRegisterGroupFloat		= 0x0400,    /* Floating Point.  */
  xtRegisterGroupVectra		= 0x0800,    /* Vectra.  */
  xtRegisterGroupSystem		= 0x1000,    /* System.  */

  xtRegisterGroupNCP	    = 0x00800000,    /* Non-CP non-base opt/custom.  */
  xtRegisterGroupCP0	    = 0x01000000,    /* CP0.  */
  xtRegisterGroupCP1	    = 0x02000000,    /* CP1.  */
  xtRegisterGroupCP2	    = 0x04000000,    /* CP2.  */
  xtRegisterGroupCP3	    = 0x08000000,    /* CP3.  */
  xtRegisterGroupCP4	    = 0x10000000,    /* CP4.  */
  xtRegisterGroupCP5	    = 0x20000000,    /* CP5.  */
  xtRegisterGroupCP6	    = 0x40000000,    /* CP6.  */
  xtRegisterGroupCP7	    = 0x80000000,    /* CP7.  */

} xtensa_register_group_t;


/*  Xtensa target flags.  */

typedef enum 
{
  xtTargetFlagsNonVisibleRegs	= 0x0001,
  xtTargetFlagsUseFetchStore	= 0x0002,
} xtensa_target_flags_t;


/*  Mask.  */

typedef struct 
{
  int reg_num;
  int bit_start;
  int bit_size;
} xtensa_reg_mask_t;

typedef struct 
{
  int count;
  xtensa_reg_mask_t *mask;
} xtensa_mask_t;


/*  Xtensa register representation.  */

typedef struct 
{
  char* name;             	/* Register name.  */
  int offset;             	/* Offset.  */
  xtensa_register_type_t type;  /* Register type.  */
  xtensa_register_group_t group;/* Register group.  */
  struct type* ctype;		/* C-type.  */
  int bit_size;  		/* The actual bit size in the target.  */
  int byte_size;          	/* Actual space allocated in registers[].  */
  int align;			/* Alignment for this register.  */

  unsigned int target_number;	/* Register target number.  */

  int flags;			/* Flags.  */
  int coprocessor;		/* Coprocessor num, -1 for non-CP, else -2.  */

  const xtensa_mask_t *mask;	/* Register is a compilation of other regs.  */
  const char *fetch;		/* Instruction sequence to fetch register.  */
  const char *store;		/* Instruction sequence to store register.  */
} xtensa_register_t;

/*  For xtensa-config.c to expand to the structure above.  */
#define XTREG(index,ofs,bsz,sz,al,tnum,flg,cp,ty,gr,name,fet,sto,mas,ct,x,y) \
       {#name, ofs, (xtensa_register_type_t) (ty), \
	((xtensa_register_group_t) \
	 ((gr) | ((xtRegisterGroupNCP >> 2) << (cp + 2)))), \
	 ct, bsz, sz, al, tnum, flg, cp, mas, fet, sto},
#define XTREG_END \
  {0, 0, (xtensa_register_type_t) 0, (xtensa_register_group_t) 0,	\
   0, 0, 0, 0, -1, 0, 0, 0, 0, 0},

#define XTENSA_REGISTER_FLAGS_PRIVILEGED	0x0001
#define XTENSA_REGISTER_FLAGS_READABLE		0x0002
#define XTENSA_REGISTER_FLAGS_WRITABLE		0x0004
#define XTENSA_REGISTER_FLAGS_VOLATILE		0x0008

/*  Call-ABI for stack frame.  */

typedef enum 
{
  CallAbiDefault = 0,		/* Any 'callX' instructions; default stack.  */
  CallAbiCall0Only,		/* Only 'call0' instructions; flat stack.  */
} call_abi_t;


struct ctype_cache
{
  struct ctype_cache *next;
  int size;
  struct type *virtual_type;
};

/*  Xtensa-specific target dependencies.  */

struct gdbarch_tdep
{
  unsigned int target_flags;

  /* Spill location for TIE register files under ocd.  */

  unsigned int spill_location;
  unsigned int spill_size;

  char *unused;				/* Placeholder for compatibility.  */
  call_abi_t call_abi;			/* Calling convention.  */

  /* CPU configuration.  */

  unsigned int debug_interrupt_level;

  unsigned int icache_line_bytes;
  unsigned int dcache_line_bytes;
  unsigned int dcache_writeback;

  unsigned int isa_use_windowed_registers;
  unsigned int isa_use_density_instructions;
  unsigned int isa_use_exceptions;
  unsigned int isa_use_ext_l32r;
  unsigned int isa_max_insn_size;	/* Maximum instruction length.  */
  unsigned int debug_num_ibreaks;	/* Number of IBREAKs.  */
  unsigned int debug_num_dbreaks;

  /* Register map.  */

  xtensa_register_t* regmap;

  unsigned int num_regs;	/* Number of registers in register map.  */
  unsigned int num_nopriv_regs;	/* Number of non-privileged registers.  */
  unsigned int num_pseudo_regs;	/* Number of pseudo registers.  */
  unsigned int num_aregs;	/* Size of register file.  */
  unsigned int num_contexts;

  int ar_base;			/* Register number for AR0.  */
  int a0_base;			/* Register number for A0 (pseudo).  */
  int wb_regnum;		/* Register number for WB.  */
  int ws_regnum;		/* Register number for WS.  */
  int pc_regnum;		/* Register number for PC.  */
  int ps_regnum;		/* Register number for PS.  */
  int lbeg_regnum;		/* Register numbers for count regs.  */
  int lend_regnum;
  int lcount_regnum;
  int sar_regnum;		/* Register number of SAR.  */
  int litbase_regnum;		/* Register number of LITBASE.  */

  int interrupt_regnum;		/* Register number for interrupt.  */
  int interrupt2_regnum;	/* Register number for interrupt2.  */
  int cpenable_regnum;		/* Register number for cpenable.  */
  int debugcause_regnum;	/* Register number for debugcause.  */
  int exccause_regnum;		/* Register number for exccause.  */
  int excvaddr_regnum;		/* Register number for excvaddr.  */

  int max_register_raw_size;
  int max_register_virtual_size;
  unsigned long *fp_layout;	/* Layout of custom/TIE regs in 'FP' area.  */
  unsigned int fp_layout_bytes;	/* Size of layout information (in bytes).  */
  unsigned long *gregmap;

  /* Cached register types.  */
  struct ctype_cache *type_entries;
};

/* Macro to instantiate a gdbarch_tdep structure.  */

#define XTENSA_GDBARCH_TDEP_INSTANTIATE(rmap,spillsz)			\
  {									\
    0,				/* target_flags */			\
    -1,				/* spill_location */			\
    (spillsz),			/* spill_size */			\
    0,				/* unused */				\
    (XSHAL_ABI == XTHAL_ABI_CALL0					\
     ? CallAbiCall0Only							\
     : CallAbiDefault),		/* call_abi */				\
    XCHAL_DEBUGLEVEL,		/* debug_interrupt_level */		\
    XCHAL_ICACHE_LINESIZE,	/* icache_line_bytes */			\
    XCHAL_DCACHE_LINESIZE,	/* dcache_line_bytes */			\
    XCHAL_DCACHE_IS_WRITEBACK,  /* dcache_writeback */			\
    (XSHAL_ABI != XTHAL_ABI_CALL0),   /* isa_use_windowed_registers */	\
    XCHAL_HAVE_DENSITY,		 /* isa_use_density_instructions */	\
    XCHAL_HAVE_EXCEPTIONS,	 /* isa_use_exceptions */		\
    XSHAL_USE_ABSOLUTE_LITERALS, /* isa_use_ext_l32r */			\
    XCHAL_MAX_INSTRUCTION_SIZE,  /* isa_max_insn_size */		\
    XCHAL_NUM_IBREAK,		 /* debug_num_ibreaks */		\
    XCHAL_NUM_DBREAK,		 /* debug_num_dbreaks */		\
    rmap,			 /* regmap */				\
    0,				 /* num_regs */				\
    0,				 /* num_nopriv_regs */			\
    0,				 /* num_pseudo_regs */			\
    XCHAL_NUM_AREGS,		 /* num_aregs */			\
    XCHAL_NUM_CONTEXTS,		 /* num_contexts */			\
    -1,				 /* ar_base */				\
    -1,				 /* a0_base */				\
    -1,				 /* wb_regnum */			\
    -1,				 /* ws_regnum */			\
    -1,				 /* pc_regnum */			\
    -1,				 /* ps_regnum */			\
    -1,				 /* lbeg_regnum */			\
    -1,				 /* lend_regnum */			\
    -1,				 /* lcount_regnum */			\
    -1,				 /* sar_regnum */			\
    -1,				 /* litbase_regnum */			\
    -1,				 /* interrupt_regnum */			\
    -1,				 /* interrupt2_regnum */		\
    -1,				 /* cpenable_regnum */			\
    -1,				 /* debugcause_regnum */		\
    -1,				 /* exccause_regnum */			\
    -1,				 /* excvaddr_regnum */			\
    0,				 /* max_register_raw_size */		\
    0,				 /* max_register_virtual_size */	\
    0,				 /* fp_layout */			\
    0,				 /* fp_layout_bytes */			\
    0,				 /* gregmap */				\
  }
#define XTENSA_CONFIG_INSTANTIATE(rmap,spill_size)	\
	struct gdbarch_tdep xtensa_tdep = \
	  XTENSA_GDBARCH_TDEP_INSTANTIATE(rmap,spill_size);

#ifndef XCHAL_NUM_CONTEXTS
#define XCHAL_NUM_CONTEXTS	0
#endif
#ifndef XCHAL_HAVE_EXCEPTIONS
#define XCHAL_HAVE_EXCEPTIONS	1
#endif
#define WB_SHIFT	  2

/* We assign fixed numbers to the registers of the "current" window 
   (i.e., relative to WB).  The registers get remapped via the reg_map 
   data structure to their corresponding register in the AR register 
   file (see xtensa-tdep.c).  */

