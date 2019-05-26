/* Target-dependent code for GDB, the GNU debugger.

   Copyright (C) 2000-2017 Free Software Foundation, Inc.

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

#ifndef PPC_TDEP_H
#define PPC_TDEP_H

struct gdbarch;
struct frame_info;
struct value;
struct regcache;
struct type;

/* From ppc-sysv-tdep.c ...  */
enum return_value_convention ppc_sysv_abi_return_value (struct gdbarch *gdbarch,
							struct value *function,
							struct type *valtype,
							struct regcache *regcache,
							gdb_byte *readbuf,
							const gdb_byte *writebuf);
enum return_value_convention ppc_sysv_abi_broken_return_value (struct gdbarch *gdbarch,
							       struct value *function,
							       struct type *valtype,
							       struct regcache *regcache,
							       gdb_byte *readbuf,
							       const gdb_byte *writebuf);
CORE_ADDR ppc_sysv_abi_push_dummy_call (struct gdbarch *gdbarch,
					struct value *function,
					struct regcache *regcache,
					CORE_ADDR bp_addr, int nargs,
					struct value **args, CORE_ADDR sp,
					int struct_return,
					CORE_ADDR struct_addr);
CORE_ADDR ppc64_sysv_abi_push_dummy_call (struct gdbarch *gdbarch,
					  struct value *function,
					  struct regcache *regcache,
					  CORE_ADDR bp_addr, int nargs,
					  struct value **args, CORE_ADDR sp,
					  int struct_return,
					  CORE_ADDR struct_addr);
enum return_value_convention ppc64_sysv_abi_return_value (struct gdbarch *gdbarch,
							  struct value *function,
							  struct type *valtype,
							  struct regcache *regcache,
							  gdb_byte *readbuf,
							  const gdb_byte *writebuf);

/* From rs6000-tdep.c...  */
int altivec_register_p (struct gdbarch *gdbarch, int regno);
int vsx_register_p (struct gdbarch *gdbarch, int regno);
int spe_register_p (struct gdbarch *gdbarch, int regno);

/* Return non-zero if the architecture described by GDBARCH has
   floating-point registers (f0 --- f31 and fpscr).  */
int ppc_floating_point_unit_p (struct gdbarch *gdbarch);

/* Return non-zero if the architecture described by GDBARCH has
   Altivec registers (vr0 --- vr31, vrsave and vscr).  */
int ppc_altivec_support_p (struct gdbarch *gdbarch);

/* Return non-zero if the architecture described by GDBARCH has
   VSX registers (vsr0 --- vsr63).  */
int vsx_support_p (struct gdbarch *gdbarch);
VEC (CORE_ADDR) *ppc_deal_with_atomic_sequence (struct regcache *regcache);


/* Register set description.  */

struct ppc_reg_offsets
{
  /* General-purpose registers.  */
  int r0_offset;
  int gpr_size; /* size for r0-31, pc, ps, lr, ctr.  */
  int xr_size;  /* size for cr, xer, mq.  */
  int pc_offset;
  int ps_offset;
  int cr_offset;
  int lr_offset;
  int ctr_offset;
  int xer_offset;
  int mq_offset;

  /* Floating-point registers.  */
  int f0_offset;
  int fpscr_offset;
  int fpscr_size;

  /* AltiVec registers.  */
  int vr0_offset;
  int vscr_offset;
  int vrsave_offset;
};

extern void ppc_supply_reg (struct regcache *regcache, int regnum,
			    const gdb_byte *regs, size_t offset, int regsize);

extern void ppc_collect_reg (const struct regcache *regcache, int regnum,
			     gdb_byte *regs, size_t offset, int regsize);

/* Supply register REGNUM in the general-purpose register set REGSET
   from the buffer specified by GREGS and LEN to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

extern void ppc_supply_gregset (const struct regset *regset,
				struct regcache *regcache,
				int regnum, const void *gregs, size_t len);

/* Supply register REGNUM in the floating-point register set REGSET
   from the buffer specified by FPREGS and LEN to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

extern void ppc_supply_fpregset (const struct regset *regset,
				 struct regcache *regcache,
				 int regnum, const void *fpregs, size_t len);

/* Supply register REGNUM in the Altivec register set REGSET
   from the buffer specified by VRREGS and LEN to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

extern void ppc_supply_vrregset (const struct regset *regset,
				 struct regcache *regcache,
				 int regnum, const void *vrregs, size_t len);

/* Supply register REGNUM in the VSX register set REGSET
   from the buffer specified by VSXREGS and LEN to register cache
   REGCACHE.  If REGNUM is -1, do this for all registers in REGSET.  */

extern void ppc_supply_vsxregset (const struct regset *regset,
				 struct regcache *regcache,
				 int regnum, const void *vsxregs, size_t len);

/* Collect register REGNUM in the general-purpose register set
   REGSET, from register cache REGCACHE into the buffer specified by
   GREGS and LEN.  If REGNUM is -1, do this for all registers in
   REGSET.  */

extern void ppc_collect_gregset (const struct regset *regset,
				 const struct regcache *regcache,
				 int regnum, void *gregs, size_t len);

/* Collect register REGNUM in the floating-point register set
   REGSET, from register cache REGCACHE into the buffer specified by
   FPREGS and LEN.  If REGNUM is -1, do this for all registers in
   REGSET.  */

extern void ppc_collect_fpregset (const struct regset *regset,
				  const struct regcache *regcache,
				  int regnum, void *fpregs, size_t len);

/* Collect register REGNUM in the Altivec register set
   REGSET from register cache REGCACHE into the buffer specified by
   VRREGS and LEN.  If REGNUM is -1, do this for all registers in
   REGSET.  */

extern void ppc_collect_vrregset (const struct regset *regset,
				  const struct regcache *regcache,
				  int regnum, void *vrregs, size_t len);

/* Collect register REGNUM in the VSX register set
   REGSET from register cache REGCACHE into the buffer specified by
   VSXREGS and LEN.  If REGNUM is -1, do this for all registers in
   REGSET.  */

extern void ppc_collect_vsxregset (const struct regset *regset,
				  const struct regcache *regcache,
				  int regnum, void *vsxregs, size_t len);

/* Private data that this module attaches to struct gdbarch.  */

/* ELF ABI version used by the inferior.  */
enum powerpc_elf_abi
{
  POWERPC_ELF_AUTO,
  POWERPC_ELF_V1,
  POWERPC_ELF_V2,
  POWERPC_ELF_LAST
};

/* Vector ABI used by the inferior.  */
enum powerpc_vector_abi
{
  POWERPC_VEC_AUTO,
  POWERPC_VEC_GENERIC,
  POWERPC_VEC_ALTIVEC,
  POWERPC_VEC_SPE,
  POWERPC_VEC_LAST
};

struct gdbarch_tdep
  {
    int wordsize;		/* Size in bytes of fixed-point word.  */
    int soft_float;		/* Avoid FP registers for arguments?  */

    enum powerpc_elf_abi elf_abi;	/* ELF ABI version.  */

    /* How to pass vector arguments.  Never set to AUTO or LAST.  */
    enum powerpc_vector_abi vector_abi;

    int ppc_gp0_regnum;		/* GPR register 0 */
    int ppc_toc_regnum;		/* TOC register */
    int ppc_ps_regnum;	        /* Processor (or machine) status (%msr) */
    int ppc_cr_regnum;		/* Condition register */
    int ppc_lr_regnum;		/* Link register */
    int ppc_ctr_regnum;		/* Count register */
    int ppc_xer_regnum;		/* Integer exception register */

    /* Not all PPC and RS6000 variants will have the registers
       represented below.  A -1 is used to indicate that the register
       is not present in this variant.  */

    /* Floating-point registers.  */
    int ppc_fp0_regnum;         /* Floating-point register 0.  */
    int ppc_fpscr_regnum;	/* fp status and condition register.  */

    /* Multiplier-Quotient Register (older POWER architectures only).  */
    int ppc_mq_regnum;

    /* POWER7 VSX registers.  */
    int ppc_vsr0_regnum;	/* First VSX register.  */
    int ppc_vsr0_upper_regnum;  /* First right most dword vsx register.  */
    int ppc_efpr0_regnum;	/* First Extended FP register.  */

    /* Altivec registers.  */
    int ppc_vr0_regnum;		/* First AltiVec register.  */
    int ppc_vrsave_regnum;	/* Last AltiVec register.  */

    /* SPE registers.  */
    int ppc_ev0_upper_regnum;   /* First GPR upper half register.  */
    int ppc_ev0_regnum;         /* First ev register.  */
    int ppc_acc_regnum;         /* SPE 'acc' register.  */
    int ppc_spefscr_regnum;     /* SPE 'spefscr' register.  */

    /* Decimal 128 registers.  */
    int ppc_dl0_regnum;		/* First Decimal128 argument register pair.  */

    /* Offset to ABI specific location where link register is saved.  */
    int lr_frame_offset;	

    /* An array of integers, such that sim_regno[I] is the simulator
       register number for GDB register number I, or -1 if the
       simulator does not implement that register.  */
    int *sim_regno;

    /* ISA-specific types.  */
    struct type *ppc_builtin_type_vec64;
    struct type *ppc_builtin_type_vec128;

    int (*ppc_syscall_record) (struct regcache *regcache);
};


/* Constants for register set sizes.  */
enum
  {
    ppc_num_gprs = 32,		/* 32 general-purpose registers.  */
    ppc_num_fprs = 32,		/* 32 floating-point registers.  */
    ppc_num_srs = 16,		/* 16 segment registers.  */
    ppc_num_vrs = 32,		/* 32 Altivec vector registers.  */
    ppc_num_vshrs = 32,		/* 32 doublewords (dword 1 of vs0~vs31).  */
    ppc_num_vsrs = 64,		/* 64 VSX vector registers.  */
    ppc_num_efprs = 32		/* 32 Extended FP registers.  */
  };


/* Register number constants.  These are GDB internal register
   numbers; they are not used for the simulator or remote targets.
   Extra SPRs (those other than MQ, CTR, LR, XER, SPEFSCR) are given
   numbers above PPC_NUM_REGS.  So are segment registers and other
   target-defined registers.  */
enum {
  PPC_R0_REGNUM = 0,
  PPC_F0_REGNUM = 32,
  PPC_PC_REGNUM = 64,
  PPC_MSR_REGNUM = 65,
  PPC_CR_REGNUM = 66,
  PPC_LR_REGNUM = 67,
  PPC_CTR_REGNUM = 68,
  PPC_XER_REGNUM = 69,
  PPC_FPSCR_REGNUM = 70,
  PPC_MQ_REGNUM = 71,
  PPC_SPE_UPPER_GP0_REGNUM = 72,
  PPC_SPE_ACC_REGNUM = 104,
  PPC_SPE_FSCR_REGNUM = 105,
  PPC_VR0_REGNUM = 106,
  PPC_VSCR_REGNUM = 138,
  PPC_VRSAVE_REGNUM = 139,
  PPC_VSR0_UPPER_REGNUM = 140,
  PPC_VSR31_UPPER_REGNUM = 171,
  PPC_NUM_REGS
};

/* An instruction to match.  */

struct ppc_insn_pattern
{
  unsigned int mask;            /* mask the insn with this...  */
  unsigned int data;            /* ...and see if it matches this.  */
  int optional;                 /* If non-zero, this insn may be absent.  */
};

extern int ppc_insns_match_pattern (struct frame_info *frame, CORE_ADDR pc,
				    struct ppc_insn_pattern *pattern,
				    unsigned int *insns);
extern CORE_ADDR ppc_insn_d_field (unsigned int insn);

extern CORE_ADDR ppc_insn_ds_field (unsigned int insn);

extern int ppc_process_record (struct gdbarch *gdbarch,
			       struct regcache *regcache, CORE_ADDR addr);

/* Instruction size.  */
#define PPC_INSN_SIZE 4

/* Estimate for the maximum number of instrctions in a function epilogue.  */
#define PPC_MAX_EPILOGUE_INSTRUCTIONS  52

#endif /* ppc-tdep.h */
