/* CPU data header for ms1.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright 1996-2005 Free Software Foundation, Inc.

This file is part of the GNU Binutils and/or GDB, the GNU debugger.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

*/

#ifndef MS1_CPU_H
#define MS1_CPU_H

#include "opcode/cgen-bitset.h"

#define CGEN_ARCH ms1

/* Given symbol S, return ms1_cgen_<S>.  */
#if defined (__STDC__) || defined (ALMOST_STDC) || defined (HAVE_STRINGIZE)
#define CGEN_SYM(s) ms1##_cgen_##s
#else
#define CGEN_SYM(s) ms1/**/_cgen_/**/s
#endif


/* Selected cpu families.  */
#define HAVE_CPU_MS1BF
#define HAVE_CPU_MS1_003BF

#define CGEN_INSN_LSB0_P 1

/* Minimum size of any insn (in bytes).  */
#define CGEN_MIN_INSN_SIZE 4

/* Maximum size of any insn (in bytes).  */
#define CGEN_MAX_INSN_SIZE 4

#define CGEN_INT_INSN_P 1

/* Maximum number of syntax elements in an instruction.  */
#define CGEN_ACTUAL_MAX_SYNTAX_ELEMENTS 40

/* CGEN_MNEMONIC_OPERANDS is defined if mnemonics have operands.
   e.g. In "b,a foo" the ",a" is an operand.  If mnemonics have operands
   we can't hash on everything up to the space.  */
#define CGEN_MNEMONIC_OPERANDS

/* Maximum number of fields in an instruction.  */
#define CGEN_ACTUAL_MAX_IFMT_OPERANDS 14

/* Enums.  */

/* Enum declaration for msys enums.  */
typedef enum insn_msys {
  MSYS_NO, MSYS_YES
} INSN_MSYS;

/* Enum declaration for opc enums.  */
typedef enum insn_opc {
  OPC_ADD = 0, OPC_ADDU = 1, OPC_SUB = 2, OPC_SUBU = 3
 , OPC_MUL = 4, OPC_AND = 8, OPC_OR = 9, OPC_XOR = 10
 , OPC_NAND = 11, OPC_NOR = 12, OPC_XNOR = 13, OPC_LDUI = 14
 , OPC_LSL = 16, OPC_LSR = 17, OPC_ASR = 18, OPC_BRLT = 24
 , OPC_BRLE = 25, OPC_BREQ = 26, OPC_JMP = 27, OPC_JAL = 28
 , OPC_BRNEQ = 29, OPC_DBNZ = 30, OPC_LDW = 32, OPC_STW = 33
 , OPC_EI = 48, OPC_DI = 49, OPC_SI = 50, OPC_RETI = 51
 , OPC_BREAK = 52, OPC_IFLUSH = 53
} INSN_OPC;

/* Enum declaration for msopc enums.  */
typedef enum insn_msopc {
  MSOPC_LDCTXT, MSOPC_LDFB, MSOPC_STFB, MSOPC_FBCB
 , MSOPC_MFBCB, MSOPC_FBCCI, MSOPC_FBRCI, MSOPC_FBCRI
 , MSOPC_FBRRI, MSOPC_MFBCCI, MSOPC_MFBRCI, MSOPC_MFBCRI
 , MSOPC_MFBRRI, MSOPC_FBCBDR, MSOPC_RCFBCB, MSOPC_MRCFBCB
 , MSOPC_CBCAST, MSOPC_DUPCBCAST, MSOPC_WFBI, MSOPC_WFB
 , MSOPC_RCRISC, MSOPC_FBCBINC, MSOPC_RCXMODE, MSOPC_INTLVR
 , MSOPC_WFBINC, MSOPC_MWFBINC, MSOPC_WFBINCR, MSOPC_MWFBINCR
 , MSOPC_FBCBINCS, MSOPC_MFBCBINCS, MSOPC_FBCBINCRS, MSOPC_MFBCBINCRS
} INSN_MSOPC;

/* Enum declaration for imm enums.  */
typedef enum insn_imm {
  IMM_NO, IMM_YES
} INSN_IMM;

/* Enum declaration for .  */
typedef enum msys_syms {
  H_NIL_DUP = 1, H_NIL_XX = 0
} MSYS_SYMS;

/* Attributes.  */

/* Enum declaration for machine type selection.  */
typedef enum mach_attr {
  MACH_BASE, MACH_MS1, MACH_MS1_003, MACH_MAX
} MACH_ATTR;

/* Enum declaration for instruction set selection.  */
typedef enum isa_attr {
  ISA_MS1, ISA_MAX
} ISA_ATTR;

/* Number of architecture variants.  */
#define MAX_ISAS  1
#define MAX_MACHS ((int) MACH_MAX)

/* Ifield support.  */

/* Ifield attribute indices.  */

/* Enum declaration for cgen_ifld attrs.  */
typedef enum cgen_ifld_attr {
  CGEN_IFLD_VIRTUAL, CGEN_IFLD_PCREL_ADDR, CGEN_IFLD_ABS_ADDR, CGEN_IFLD_RESERVED
 , CGEN_IFLD_SIGN_OPT, CGEN_IFLD_SIGNED, CGEN_IFLD_END_BOOLS, CGEN_IFLD_START_NBOOLS = 31
 , CGEN_IFLD_MACH, CGEN_IFLD_END_NBOOLS
} CGEN_IFLD_ATTR;

/* Number of non-boolean elements in cgen_ifld_attr.  */
#define CGEN_IFLD_NBOOL_ATTRS (CGEN_IFLD_END_NBOOLS - CGEN_IFLD_START_NBOOLS - 1)

/* cgen_ifld attribute accessor macros.  */
#define CGEN_ATTR_CGEN_IFLD_MACH_VALUE(attrs) ((attrs)->nonbool[CGEN_IFLD_MACH-CGEN_IFLD_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_IFLD_VIRTUAL_VALUE(attrs) (((attrs)->bool & (1 << CGEN_IFLD_VIRTUAL)) != 0)
#define CGEN_ATTR_CGEN_IFLD_PCREL_ADDR_VALUE(attrs) (((attrs)->bool & (1 << CGEN_IFLD_PCREL_ADDR)) != 0)
#define CGEN_ATTR_CGEN_IFLD_ABS_ADDR_VALUE(attrs) (((attrs)->bool & (1 << CGEN_IFLD_ABS_ADDR)) != 0)
#define CGEN_ATTR_CGEN_IFLD_RESERVED_VALUE(attrs) (((attrs)->bool & (1 << CGEN_IFLD_RESERVED)) != 0)
#define CGEN_ATTR_CGEN_IFLD_SIGN_OPT_VALUE(attrs) (((attrs)->bool & (1 << CGEN_IFLD_SIGN_OPT)) != 0)
#define CGEN_ATTR_CGEN_IFLD_SIGNED_VALUE(attrs) (((attrs)->bool & (1 << CGEN_IFLD_SIGNED)) != 0)

/* Enum declaration for ms1 ifield types.  */
typedef enum ifield_type {
  MS1_F_NIL, MS1_F_ANYOF, MS1_F_MSYS, MS1_F_OPC
 , MS1_F_IMM, MS1_F_UU24, MS1_F_SR1, MS1_F_SR2
 , MS1_F_DR, MS1_F_DRRR, MS1_F_IMM16U, MS1_F_IMM16S
 , MS1_F_IMM16A, MS1_F_UU4A, MS1_F_UU4B, MS1_F_UU12
 , MS1_F_UU16, MS1_F_MSOPC, MS1_F_UU_26_25, MS1_F_MASK
 , MS1_F_BANKADDR, MS1_F_RDA, MS1_F_UU_2_25, MS1_F_RBBC
 , MS1_F_PERM, MS1_F_MODE, MS1_F_UU_1_24, MS1_F_WR
 , MS1_F_FBINCR, MS1_F_UU_2_23, MS1_F_XMODE, MS1_F_A23
 , MS1_F_MASK1, MS1_F_CR, MS1_F_TYPE, MS1_F_INCAMT
 , MS1_F_CBS, MS1_F_UU_1_19, MS1_F_BALL, MS1_F_COLNUM
 , MS1_F_BRC, MS1_F_INCR, MS1_F_FBDISP, MS1_F_UU_4_15
 , MS1_F_LENGTH, MS1_F_UU_1_15, MS1_F_RC, MS1_F_RCNUM
 , MS1_F_ROWNUM, MS1_F_CBX, MS1_F_ID, MS1_F_SIZE
 , MS1_F_ROWNUM1, MS1_F_UU_3_11, MS1_F_RC1, MS1_F_CCB
 , MS1_F_CBRB, MS1_F_CDB, MS1_F_ROWNUM2, MS1_F_CELL
 , MS1_F_UU_3_9, MS1_F_CONTNUM, MS1_F_UU_1_6, MS1_F_DUP
 , MS1_F_RC2, MS1_F_CTXDISP, MS1_F_MSYSFRSR2, MS1_F_BRC2
 , MS1_F_BALL2, MS1_F_MAX
} IFIELD_TYPE;

#define MAX_IFLD ((int) MS1_F_MAX)

/* Hardware attribute indices.  */

/* Enum declaration for cgen_hw attrs.  */
typedef enum cgen_hw_attr {
  CGEN_HW_VIRTUAL, CGEN_HW_CACHE_ADDR, CGEN_HW_PC, CGEN_HW_PROFILE
 , CGEN_HW_END_BOOLS, CGEN_HW_START_NBOOLS = 31, CGEN_HW_MACH, CGEN_HW_END_NBOOLS
} CGEN_HW_ATTR;

/* Number of non-boolean elements in cgen_hw_attr.  */
#define CGEN_HW_NBOOL_ATTRS (CGEN_HW_END_NBOOLS - CGEN_HW_START_NBOOLS - 1)

/* cgen_hw attribute accessor macros.  */
#define CGEN_ATTR_CGEN_HW_MACH_VALUE(attrs) ((attrs)->nonbool[CGEN_HW_MACH-CGEN_HW_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_HW_VIRTUAL_VALUE(attrs) (((attrs)->bool & (1 << CGEN_HW_VIRTUAL)) != 0)
#define CGEN_ATTR_CGEN_HW_CACHE_ADDR_VALUE(attrs) (((attrs)->bool & (1 << CGEN_HW_CACHE_ADDR)) != 0)
#define CGEN_ATTR_CGEN_HW_PC_VALUE(attrs) (((attrs)->bool & (1 << CGEN_HW_PC)) != 0)
#define CGEN_ATTR_CGEN_HW_PROFILE_VALUE(attrs) (((attrs)->bool & (1 << CGEN_HW_PROFILE)) != 0)

/* Enum declaration for ms1 hardware types.  */
typedef enum cgen_hw_type {
  HW_H_MEMORY, HW_H_SINT, HW_H_UINT, HW_H_ADDR
 , HW_H_IADDR, HW_H_SPR, HW_H_PC, HW_MAX
} CGEN_HW_TYPE;

#define MAX_HW ((int) HW_MAX)

/* Operand attribute indices.  */

/* Enum declaration for cgen_operand attrs.  */
typedef enum cgen_operand_attr {
  CGEN_OPERAND_VIRTUAL, CGEN_OPERAND_PCREL_ADDR, CGEN_OPERAND_ABS_ADDR, CGEN_OPERAND_SIGN_OPT
 , CGEN_OPERAND_SIGNED, CGEN_OPERAND_NEGATIVE, CGEN_OPERAND_RELAX, CGEN_OPERAND_SEM_ONLY
 , CGEN_OPERAND_END_BOOLS, CGEN_OPERAND_START_NBOOLS = 31, CGEN_OPERAND_MACH, CGEN_OPERAND_END_NBOOLS
} CGEN_OPERAND_ATTR;

/* Number of non-boolean elements in cgen_operand_attr.  */
#define CGEN_OPERAND_NBOOL_ATTRS (CGEN_OPERAND_END_NBOOLS - CGEN_OPERAND_START_NBOOLS - 1)

/* cgen_operand attribute accessor macros.  */
#define CGEN_ATTR_CGEN_OPERAND_MACH_VALUE(attrs) ((attrs)->nonbool[CGEN_OPERAND_MACH-CGEN_OPERAND_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_OPERAND_VIRTUAL_VALUE(attrs) (((attrs)->bool & (1 << CGEN_OPERAND_VIRTUAL)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_PCREL_ADDR_VALUE(attrs) (((attrs)->bool & (1 << CGEN_OPERAND_PCREL_ADDR)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_ABS_ADDR_VALUE(attrs) (((attrs)->bool & (1 << CGEN_OPERAND_ABS_ADDR)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_SIGN_OPT_VALUE(attrs) (((attrs)->bool & (1 << CGEN_OPERAND_SIGN_OPT)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_SIGNED_VALUE(attrs) (((attrs)->bool & (1 << CGEN_OPERAND_SIGNED)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_NEGATIVE_VALUE(attrs) (((attrs)->bool & (1 << CGEN_OPERAND_NEGATIVE)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_RELAX_VALUE(attrs) (((attrs)->bool & (1 << CGEN_OPERAND_RELAX)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_SEM_ONLY_VALUE(attrs) (((attrs)->bool & (1 << CGEN_OPERAND_SEM_ONLY)) != 0)

/* Enum declaration for ms1 operand types.  */
typedef enum cgen_operand_type {
  MS1_OPERAND_PC, MS1_OPERAND_FRSR1, MS1_OPERAND_FRSR2, MS1_OPERAND_FRDR
 , MS1_OPERAND_FRDRRR, MS1_OPERAND_IMM16, MS1_OPERAND_IMM16Z, MS1_OPERAND_IMM16O
 , MS1_OPERAND_RC, MS1_OPERAND_RCNUM, MS1_OPERAND_CONTNUM, MS1_OPERAND_RBBC
 , MS1_OPERAND_COLNUM, MS1_OPERAND_ROWNUM, MS1_OPERAND_ROWNUM1, MS1_OPERAND_ROWNUM2
 , MS1_OPERAND_RC1, MS1_OPERAND_RC2, MS1_OPERAND_CBRB, MS1_OPERAND_CELL
 , MS1_OPERAND_DUP, MS1_OPERAND_CTXDISP, MS1_OPERAND_FBDISP, MS1_OPERAND_TYPE
 , MS1_OPERAND_MASK, MS1_OPERAND_BANKADDR, MS1_OPERAND_INCAMT, MS1_OPERAND_XMODE
 , MS1_OPERAND_MASK1, MS1_OPERAND_BALL, MS1_OPERAND_BRC, MS1_OPERAND_RDA
 , MS1_OPERAND_WR, MS1_OPERAND_BALL2, MS1_OPERAND_BRC2, MS1_OPERAND_PERM
 , MS1_OPERAND_A23, MS1_OPERAND_CR, MS1_OPERAND_CBS, MS1_OPERAND_INCR
 , MS1_OPERAND_LENGTH, MS1_OPERAND_CBX, MS1_OPERAND_CCB, MS1_OPERAND_CDB
 , MS1_OPERAND_MODE, MS1_OPERAND_ID, MS1_OPERAND_SIZE, MS1_OPERAND_FBINCR
 , MS1_OPERAND_MAX
} CGEN_OPERAND_TYPE;

/* Number of operands types.  */
#define MAX_OPERANDS 48

/* Maximum number of operands referenced by any insn.  */
#define MAX_OPERAND_INSTANCES 8

/* Insn attribute indices.  */

/* Enum declaration for cgen_insn attrs.  */
typedef enum cgen_insn_attr {
  CGEN_INSN_ALIAS, CGEN_INSN_VIRTUAL, CGEN_INSN_UNCOND_CTI, CGEN_INSN_COND_CTI
 , CGEN_INSN_SKIP_CTI, CGEN_INSN_DELAY_SLOT, CGEN_INSN_RELAXABLE, CGEN_INSN_RELAXED
 , CGEN_INSN_NO_DIS, CGEN_INSN_PBB, CGEN_INSN_LOAD_DELAY, CGEN_INSN_MEMORY_ACCESS
 , CGEN_INSN_AL_INSN, CGEN_INSN_IO_INSN, CGEN_INSN_BR_INSN, CGEN_INSN_USES_FRDR
 , CGEN_INSN_USES_FRDRRR, CGEN_INSN_USES_FRSR1, CGEN_INSN_USES_FRSR2, CGEN_INSN_SKIPA
 , CGEN_INSN_END_BOOLS, CGEN_INSN_START_NBOOLS = 31, CGEN_INSN_MACH, CGEN_INSN_END_NBOOLS
} CGEN_INSN_ATTR;

/* Number of non-boolean elements in cgen_insn_attr.  */
#define CGEN_INSN_NBOOL_ATTRS (CGEN_INSN_END_NBOOLS - CGEN_INSN_START_NBOOLS - 1)

/* cgen_insn attribute accessor macros.  */
#define CGEN_ATTR_CGEN_INSN_MACH_VALUE(attrs) ((attrs)->nonbool[CGEN_INSN_MACH-CGEN_INSN_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_INSN_ALIAS_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_ALIAS)) != 0)
#define CGEN_ATTR_CGEN_INSN_VIRTUAL_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_VIRTUAL)) != 0)
#define CGEN_ATTR_CGEN_INSN_UNCOND_CTI_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_UNCOND_CTI)) != 0)
#define CGEN_ATTR_CGEN_INSN_COND_CTI_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_COND_CTI)) != 0)
#define CGEN_ATTR_CGEN_INSN_SKIP_CTI_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_SKIP_CTI)) != 0)
#define CGEN_ATTR_CGEN_INSN_DELAY_SLOT_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_DELAY_SLOT)) != 0)
#define CGEN_ATTR_CGEN_INSN_RELAXABLE_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_RELAXABLE)) != 0)
#define CGEN_ATTR_CGEN_INSN_RELAXED_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_RELAXED)) != 0)
#define CGEN_ATTR_CGEN_INSN_NO_DIS_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_NO_DIS)) != 0)
#define CGEN_ATTR_CGEN_INSN_PBB_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_PBB)) != 0)
#define CGEN_ATTR_CGEN_INSN_LOAD_DELAY_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_LOAD_DELAY)) != 0)
#define CGEN_ATTR_CGEN_INSN_MEMORY_ACCESS_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_MEMORY_ACCESS)) != 0)
#define CGEN_ATTR_CGEN_INSN_AL_INSN_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_AL_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_IO_INSN_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_IO_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_BR_INSN_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_BR_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_USES_FRDR_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_USES_FRDR)) != 0)
#define CGEN_ATTR_CGEN_INSN_USES_FRDRRR_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_USES_FRDRRR)) != 0)
#define CGEN_ATTR_CGEN_INSN_USES_FRSR1_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_USES_FRSR1)) != 0)
#define CGEN_ATTR_CGEN_INSN_USES_FRSR2_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_USES_FRSR2)) != 0)
#define CGEN_ATTR_CGEN_INSN_SKIPA_VALUE(attrs) (((attrs)->bool & (1 << CGEN_INSN_SKIPA)) != 0)

/* cgen.h uses things we just defined.  */
#include "opcode/cgen.h"

extern const struct cgen_ifld ms1_cgen_ifld_table[];

/* Attributes.  */
extern const CGEN_ATTR_TABLE ms1_cgen_hardware_attr_table[];
extern const CGEN_ATTR_TABLE ms1_cgen_ifield_attr_table[];
extern const CGEN_ATTR_TABLE ms1_cgen_operand_attr_table[];
extern const CGEN_ATTR_TABLE ms1_cgen_insn_attr_table[];

/* Hardware decls.  */

extern CGEN_KEYWORD ms1_cgen_opval_h_spr;

extern const CGEN_HW_ENTRY ms1_cgen_hw_table[];



#endif /* MS1_CPU_H */
