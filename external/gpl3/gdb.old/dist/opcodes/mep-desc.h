/* DO NOT EDIT!  -*- buffer-read-only: t -*- vi:set ro:  */
/* CPU data header for mep.

THIS FILE IS MACHINE GENERATED WITH CGEN.

Copyright (C) 1996-2020 Free Software Foundation, Inc.

This file is part of the GNU Binutils and/or GDB, the GNU debugger.

   This file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

*/

#ifndef MEP_CPU_H
#define MEP_CPU_H

#ifdef __cplusplus
extern "C" {
#endif

#define CGEN_ARCH mep

/* Given symbol S, return mep_cgen_<S>.  */
#define CGEN_SYM(s) mep##_cgen_##s


/* Selected cpu families.  */
#define HAVE_CPU_MEPF

#define CGEN_INSN_LSB0_P 0

/* Minimum size of any insn (in bytes).  */
#define CGEN_MIN_INSN_SIZE 2

/* Maximum size of any insn (in bytes).  */
#define CGEN_MAX_INSN_SIZE 4

#define CGEN_INT_INSN_P 1

/* Maximum number of syntax elements in an instruction.  */
#define CGEN_ACTUAL_MAX_SYNTAX_ELEMENTS 22

/* CGEN_MNEMONIC_OPERANDS is defined if mnemonics have operands.
   e.g. In "b,a foo" the ",a" is an operand.  If mnemonics have operands
   we can't hash on everything up to the space.  */
#define CGEN_MNEMONIC_OPERANDS

/* Maximum number of fields in an instruction.  */
#define CGEN_ACTUAL_MAX_IFMT_OPERANDS 10

/* Enums.  */

/* Enum declaration for major opcodes.  */
typedef enum major {
  MAJ_0, MAJ_1, MAJ_2, MAJ_3
 , MAJ_4, MAJ_5, MAJ_6, MAJ_7
 , MAJ_8, MAJ_9, MAJ_10, MAJ_11
 , MAJ_12, MAJ_13, MAJ_14, MAJ_15
} MAJOR;

/* Attributes.  */

/* Enum declaration for machine type selection.  */
typedef enum mach_attr {
  MACH_BASE, MACH_MEP, MACH_H1, MACH_C5
 , MACH_MAX
} MACH_ATTR;

/* Enum declaration for instruction set selection.  */
typedef enum isa_attr {
  ISA_MEP, ISA_EXT_CORE1, ISA_EXT_COP1_16, ISA_EXT_COP1_32
 , ISA_EXT_COP1_48, ISA_EXT_COP1_64, ISA_MAX
} ISA_ATTR;

/* Enum declaration for datatype to use for C intrinsics mapping.  */
typedef enum cdata_attr {
  CDATA_LABEL, CDATA_REGNUM, CDATA_FMAX_FLOAT, CDATA_FMAX_INT
 , CDATA_POINTER, CDATA_LONG, CDATA_ULONG, CDATA_SHORT
 , CDATA_USHORT, CDATA_CHAR, CDATA_UCHAR, CDATA_CP_DATA_BUS_INT
} CDATA_ATTR;

/* Enum declaration for datatype to use for coprocessor values.  */
typedef enum cptype_attr {
  CPTYPE_CP_DATA_BUS_INT, CPTYPE_VECT, CPTYPE_V2SI, CPTYPE_V4HI
 , CPTYPE_V8QI, CPTYPE_V2USI, CPTYPE_V4UHI, CPTYPE_V8UQI
} CPTYPE_ATTR;

/* Enum declaration for Insn's intrinsic returns void, or the first argument rather than (or in addition to) passing it..  */
typedef enum cret_attr {
  CRET_VOID, CRET_FIRST, CRET_FIRSTCOPY
} CRET_ATTR;

/* Enum declaration for .  */
typedef enum config_attr {
  CONFIG_NONE, CONFIG_DEFAULT
} CONFIG_ATTR;

/* Enum declaration for slots for which this opcode is valid - c3, p0s, p0, p1.  */
typedef enum slots_attr {
  SLOTS_CORE, SLOTS_C3, SLOTS_P0S, SLOTS_P0
 , SLOTS_P1
} SLOTS_ATTR;

/* Number of architecture variants.  */
#define MAX_ISAS  ((int) ISA_MAX)
#define MAX_MACHS ((int) MACH_MAX)

/* Ifield support.  */

/* Ifield attribute indices.  */

/* Enum declaration for cgen_ifld attrs.  */
typedef enum cgen_ifld_attr {
  CGEN_IFLD_VIRTUAL, CGEN_IFLD_PCREL_ADDR, CGEN_IFLD_ABS_ADDR, CGEN_IFLD_RESERVED
 , CGEN_IFLD_SIGN_OPT, CGEN_IFLD_SIGNED, CGEN_IFLD_END_BOOLS, CGEN_IFLD_START_NBOOLS = 31
 , CGEN_IFLD_MACH, CGEN_IFLD_ISA, CGEN_IFLD_END_NBOOLS
} CGEN_IFLD_ATTR;

/* Number of non-boolean elements in cgen_ifld_attr.  */
#define CGEN_IFLD_NBOOL_ATTRS (CGEN_IFLD_END_NBOOLS - CGEN_IFLD_START_NBOOLS - 1)

/* cgen_ifld attribute accessor macros.  */
#define CGEN_ATTR_CGEN_IFLD_MACH_VALUE(attrs) ((attrs)->nonbool[CGEN_IFLD_MACH-CGEN_IFLD_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_IFLD_ISA_VALUE(attrs) ((attrs)->nonbool[CGEN_IFLD_ISA-CGEN_IFLD_START_NBOOLS-1].bitset)
#define CGEN_ATTR_CGEN_IFLD_VIRTUAL_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_IFLD_VIRTUAL)) != 0)
#define CGEN_ATTR_CGEN_IFLD_PCREL_ADDR_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_IFLD_PCREL_ADDR)) != 0)
#define CGEN_ATTR_CGEN_IFLD_ABS_ADDR_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_IFLD_ABS_ADDR)) != 0)
#define CGEN_ATTR_CGEN_IFLD_RESERVED_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_IFLD_RESERVED)) != 0)
#define CGEN_ATTR_CGEN_IFLD_SIGN_OPT_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_IFLD_SIGN_OPT)) != 0)
#define CGEN_ATTR_CGEN_IFLD_SIGNED_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_IFLD_SIGNED)) != 0)

/* Enum declaration for mep ifield types.  */
typedef enum ifield_type {
  MEP_F_NIL, MEP_F_ANYOF, MEP_F_MAJOR, MEP_F_RN
 , MEP_F_RN3, MEP_F_RM, MEP_F_RL, MEP_F_SUB2
 , MEP_F_SUB3, MEP_F_SUB4, MEP_F_EXT, MEP_F_EXT4
 , MEP_F_EXT62, MEP_F_CRN, MEP_F_CSRN_HI, MEP_F_CSRN_LO
 , MEP_F_CSRN, MEP_F_CRNX_HI, MEP_F_CRNX_LO, MEP_F_CRNX
 , MEP_F_0, MEP_F_1, MEP_F_2, MEP_F_3
 , MEP_F_4, MEP_F_5, MEP_F_6, MEP_F_7
 , MEP_F_8, MEP_F_9, MEP_F_10, MEP_F_11
 , MEP_F_12, MEP_F_13, MEP_F_14, MEP_F_15
 , MEP_F_16, MEP_F_17, MEP_F_18, MEP_F_19
 , MEP_F_20, MEP_F_21, MEP_F_22, MEP_F_23
 , MEP_F_24, MEP_F_25, MEP_F_26, MEP_F_27
 , MEP_F_28, MEP_F_29, MEP_F_30, MEP_F_31
 , MEP_F_8S8A2, MEP_F_12S4A2, MEP_F_17S16A2, MEP_F_24S5A2N_HI
 , MEP_F_24S5A2N_LO, MEP_F_24S5A2N, MEP_F_24U5A2N_HI, MEP_F_24U5A2N_LO
 , MEP_F_24U5A2N, MEP_F_2U6, MEP_F_7U9, MEP_F_7U9A2
 , MEP_F_7U9A4, MEP_F_16S16, MEP_F_2U10, MEP_F_3U5
 , MEP_F_4U8, MEP_F_5U8, MEP_F_5U24, MEP_F_6S8
 , MEP_F_8S8, MEP_F_16U16, MEP_F_12U16, MEP_F_3U29
 , MEP_F_CDISP10, MEP_F_24U8A4N_HI, MEP_F_24U8A4N_LO, MEP_F_24U8A4N
 , MEP_F_24U8N_HI, MEP_F_24U8N_LO, MEP_F_24U8N, MEP_F_24U4N_HI
 , MEP_F_24U4N_LO, MEP_F_24U4N, MEP_F_CALLNUM, MEP_F_CCRN_HI
 , MEP_F_CCRN_LO, MEP_F_CCRN, MEP_F_C5N4, MEP_F_C5N5
 , MEP_F_C5N6, MEP_F_C5N7, MEP_F_RL5, MEP_F_12S20
 , MEP_F_C5_RNM, MEP_F_C5_RM, MEP_F_C5_16U16, MEP_F_C5_RMUIMM20
 , MEP_F_C5_RNMUIMM24, MEP_F_IVC2_2U4, MEP_F_IVC2_3U4, MEP_F_IVC2_8U4
 , MEP_F_IVC2_8S4, MEP_F_IVC2_1U6, MEP_F_IVC2_2U6, MEP_F_IVC2_3U6
 , MEP_F_IVC2_6U6, MEP_F_IVC2_5U7, MEP_F_IVC2_4U8, MEP_F_IVC2_3U9
 , MEP_F_IVC2_5U16, MEP_F_IVC2_5U21, MEP_F_IVC2_5U26, MEP_F_IVC2_1U31
 , MEP_F_IVC2_4U16, MEP_F_IVC2_4U20, MEP_F_IVC2_4U24, MEP_F_IVC2_4U28
 , MEP_F_IVC2_2U0, MEP_F_IVC2_3U0, MEP_F_IVC2_4U0, MEP_F_IVC2_5U0
 , MEP_F_IVC2_8U0, MEP_F_IVC2_8S0, MEP_F_IVC2_6U2, MEP_F_IVC2_5U3
 , MEP_F_IVC2_4U4, MEP_F_IVC2_3U5, MEP_F_IVC2_5U8, MEP_F_IVC2_4U10
 , MEP_F_IVC2_3U12, MEP_F_IVC2_5U13, MEP_F_IVC2_2U18, MEP_F_IVC2_5U18
 , MEP_F_IVC2_8U20, MEP_F_IVC2_8S20, MEP_F_IVC2_5U23, MEP_F_IVC2_2U23
 , MEP_F_IVC2_3U25, MEP_F_IVC2_IMM16P0, MEP_F_IVC2_SIMM16P0, MEP_F_IVC2_CCRN_C3HI
 , MEP_F_IVC2_CCRN_C3LO, MEP_F_IVC2_CRN, MEP_F_IVC2_CRM, MEP_F_IVC2_CCRN_H1
 , MEP_F_IVC2_CCRN_H2, MEP_F_IVC2_CCRN_LO, MEP_F_IVC2_CMOV1, MEP_F_IVC2_CMOV2
 , MEP_F_IVC2_CMOV3, MEP_F_IVC2_CCRN_C3, MEP_F_IVC2_CCRN, MEP_F_IVC2_CRNX
 , MEP_F_MAX
} IFIELD_TYPE;

#define MAX_IFLD ((int) MEP_F_MAX)

/* Hardware attribute indices.  */

/* Enum declaration for cgen_hw attrs.  */
typedef enum cgen_hw_attr {
  CGEN_HW_VIRTUAL, CGEN_HW_CACHE_ADDR, CGEN_HW_PC, CGEN_HW_PROFILE
 , CGEN_HW_IS_FLOAT, CGEN_HW_END_BOOLS, CGEN_HW_START_NBOOLS = 31, CGEN_HW_MACH
 , CGEN_HW_ISA, CGEN_HW_END_NBOOLS
} CGEN_HW_ATTR;

/* Number of non-boolean elements in cgen_hw_attr.  */
#define CGEN_HW_NBOOL_ATTRS (CGEN_HW_END_NBOOLS - CGEN_HW_START_NBOOLS - 1)

/* cgen_hw attribute accessor macros.  */
#define CGEN_ATTR_CGEN_HW_MACH_VALUE(attrs) ((attrs)->nonbool[CGEN_HW_MACH-CGEN_HW_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_HW_ISA_VALUE(attrs) ((attrs)->nonbool[CGEN_HW_ISA-CGEN_HW_START_NBOOLS-1].bitset)
#define CGEN_ATTR_CGEN_HW_VIRTUAL_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_HW_VIRTUAL)) != 0)
#define CGEN_ATTR_CGEN_HW_CACHE_ADDR_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_HW_CACHE_ADDR)) != 0)
#define CGEN_ATTR_CGEN_HW_PC_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_HW_PC)) != 0)
#define CGEN_ATTR_CGEN_HW_PROFILE_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_HW_PROFILE)) != 0)
#define CGEN_ATTR_CGEN_HW_IS_FLOAT_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_HW_IS_FLOAT)) != 0)

/* Enum declaration for mep hardware types.  */
typedef enum cgen_hw_type {
  HW_H_MEMORY, HW_H_SINT, HW_H_UINT, HW_H_ADDR
 , HW_H_IADDR, HW_H_PC, HW_H_GPR, HW_H_CSR
 , HW_H_CR64, HW_H_CR64_W, HW_H_CR, HW_H_CCR
 , HW_H_CCR_W, HW_H_CR_IVC2, HW_H_CCR_IVC2, HW_MAX
} CGEN_HW_TYPE;

#define MAX_HW ((int) HW_MAX)

/* Operand attribute indices.  */

/* Enum declaration for cgen_operand attrs.  */
typedef enum cgen_operand_attr {
  CGEN_OPERAND_VIRTUAL, CGEN_OPERAND_PCREL_ADDR, CGEN_OPERAND_ABS_ADDR, CGEN_OPERAND_SIGN_OPT
 , CGEN_OPERAND_SIGNED, CGEN_OPERAND_NEGATIVE, CGEN_OPERAND_RELAX, CGEN_OPERAND_SEM_ONLY
 , CGEN_OPERAND_RELOC_IMPLIES_OVERFLOW, CGEN_OPERAND_END_BOOLS, CGEN_OPERAND_START_NBOOLS = 31, CGEN_OPERAND_MACH
 , CGEN_OPERAND_ISA, CGEN_OPERAND_CDATA, CGEN_OPERAND_ALIGN, CGEN_OPERAND_END_NBOOLS
} CGEN_OPERAND_ATTR;

/* Number of non-boolean elements in cgen_operand_attr.  */
#define CGEN_OPERAND_NBOOL_ATTRS (CGEN_OPERAND_END_NBOOLS - CGEN_OPERAND_START_NBOOLS - 1)

/* cgen_operand attribute accessor macros.  */
#define CGEN_ATTR_CGEN_OPERAND_MACH_VALUE(attrs) ((attrs)->nonbool[CGEN_OPERAND_MACH-CGEN_OPERAND_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_OPERAND_ISA_VALUE(attrs) ((attrs)->nonbool[CGEN_OPERAND_ISA-CGEN_OPERAND_START_NBOOLS-1].bitset)
#define CGEN_ATTR_CGEN_OPERAND_CDATA_VALUE(attrs) ((attrs)->nonbool[CGEN_OPERAND_CDATA-CGEN_OPERAND_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_OPERAND_ALIGN_VALUE(attrs) ((attrs)->nonbool[CGEN_OPERAND_ALIGN-CGEN_OPERAND_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_OPERAND_VIRTUAL_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_OPERAND_VIRTUAL)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_PCREL_ADDR_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_OPERAND_PCREL_ADDR)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_ABS_ADDR_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_OPERAND_ABS_ADDR)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_SIGN_OPT_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_OPERAND_SIGN_OPT)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_SIGNED_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_OPERAND_SIGNED)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_NEGATIVE_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_OPERAND_NEGATIVE)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_RELAX_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_OPERAND_RELAX)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_SEM_ONLY_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_OPERAND_SEM_ONLY)) != 0)
#define CGEN_ATTR_CGEN_OPERAND_RELOC_IMPLIES_OVERFLOW_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_OPERAND_RELOC_IMPLIES_OVERFLOW)) != 0)

/* Enum declaration for mep operand types.  */
typedef enum cgen_operand_type {
  MEP_OPERAND_PC, MEP_OPERAND_R0, MEP_OPERAND_RN, MEP_OPERAND_RM
 , MEP_OPERAND_RL, MEP_OPERAND_RN3, MEP_OPERAND_RMA, MEP_OPERAND_RNC
 , MEP_OPERAND_RNUC, MEP_OPERAND_RNS, MEP_OPERAND_RNUS, MEP_OPERAND_RNL
 , MEP_OPERAND_RNUL, MEP_OPERAND_RN3C, MEP_OPERAND_RN3UC, MEP_OPERAND_RN3S
 , MEP_OPERAND_RN3US, MEP_OPERAND_RN3L, MEP_OPERAND_RN3UL, MEP_OPERAND_LP
 , MEP_OPERAND_SAR, MEP_OPERAND_HI, MEP_OPERAND_LO, MEP_OPERAND_MB0
 , MEP_OPERAND_ME0, MEP_OPERAND_MB1, MEP_OPERAND_ME1, MEP_OPERAND_PSW
 , MEP_OPERAND_EPC, MEP_OPERAND_EXC, MEP_OPERAND_NPC, MEP_OPERAND_DBG
 , MEP_OPERAND_DEPC, MEP_OPERAND_OPT, MEP_OPERAND_R1, MEP_OPERAND_TP
 , MEP_OPERAND_SP, MEP_OPERAND_TPR, MEP_OPERAND_SPR, MEP_OPERAND_CSRN
 , MEP_OPERAND_CSRN_IDX, MEP_OPERAND_CRN64, MEP_OPERAND_CRN, MEP_OPERAND_CRNX64
 , MEP_OPERAND_CRNX, MEP_OPERAND_CCRN, MEP_OPERAND_CCCC, MEP_OPERAND_PCREL8A2
 , MEP_OPERAND_PCREL12A2, MEP_OPERAND_PCREL17A2, MEP_OPERAND_PCREL24A2, MEP_OPERAND_PCABS24A2
 , MEP_OPERAND_SDISP16, MEP_OPERAND_SIMM16, MEP_OPERAND_UIMM16, MEP_OPERAND_CODE16
 , MEP_OPERAND_UDISP2, MEP_OPERAND_UIMM2, MEP_OPERAND_SIMM6, MEP_OPERAND_SIMM8
 , MEP_OPERAND_ADDR24A4, MEP_OPERAND_CODE24, MEP_OPERAND_CALLNUM, MEP_OPERAND_UIMM3
 , MEP_OPERAND_UIMM4, MEP_OPERAND_UIMM5, MEP_OPERAND_UDISP7, MEP_OPERAND_UDISP7A2
 , MEP_OPERAND_UDISP7A4, MEP_OPERAND_UIMM7A4, MEP_OPERAND_UIMM24, MEP_OPERAND_CIMM4
 , MEP_OPERAND_CIMM5, MEP_OPERAND_CDISP10, MEP_OPERAND_CDISP10A2, MEP_OPERAND_CDISP10A4
 , MEP_OPERAND_CDISP10A8, MEP_OPERAND_ZERO, MEP_OPERAND_RL5, MEP_OPERAND_CDISP12
 , MEP_OPERAND_C5RMUIMM20, MEP_OPERAND_C5RNMUIMM24, MEP_OPERAND_CP_FLAG, MEP_OPERAND_IVC2_CSAR0
 , MEP_OPERAND_IVC2_CC, MEP_OPERAND_IVC2_COFR0, MEP_OPERAND_IVC2_COFR1, MEP_OPERAND_IVC2_COFA0
 , MEP_OPERAND_IVC2_COFA1, MEP_OPERAND_IVC2_CSAR1, MEP_OPERAND_IVC2_ACC0_0, MEP_OPERAND_IVC2_ACC0_1
 , MEP_OPERAND_IVC2_ACC0_2, MEP_OPERAND_IVC2_ACC0_3, MEP_OPERAND_IVC2_ACC0_4, MEP_OPERAND_IVC2_ACC0_5
 , MEP_OPERAND_IVC2_ACC0_6, MEP_OPERAND_IVC2_ACC0_7, MEP_OPERAND_IVC2_ACC1_0, MEP_OPERAND_IVC2_ACC1_1
 , MEP_OPERAND_IVC2_ACC1_2, MEP_OPERAND_IVC2_ACC1_3, MEP_OPERAND_IVC2_ACC1_4, MEP_OPERAND_IVC2_ACC1_5
 , MEP_OPERAND_IVC2_ACC1_6, MEP_OPERAND_IVC2_ACC1_7, MEP_OPERAND_CROC, MEP_OPERAND_CRQC
 , MEP_OPERAND_CRPC, MEP_OPERAND_IVC_X_6_1, MEP_OPERAND_IVC_X_6_2, MEP_OPERAND_IVC_X_6_3
 , MEP_OPERAND_IMM3P4, MEP_OPERAND_IMM3P9, MEP_OPERAND_IMM4P8, MEP_OPERAND_IMM5P7
 , MEP_OPERAND_IMM6P6, MEP_OPERAND_IMM8P4, MEP_OPERAND_SIMM8P4, MEP_OPERAND_IMM3P5
 , MEP_OPERAND_IMM3P12, MEP_OPERAND_IMM4P4, MEP_OPERAND_IMM4P10, MEP_OPERAND_IMM5P8
 , MEP_OPERAND_IMM5P3, MEP_OPERAND_IMM6P2, MEP_OPERAND_IMM5P23, MEP_OPERAND_IMM3P25
 , MEP_OPERAND_IMM8P0, MEP_OPERAND_SIMM8P0, MEP_OPERAND_SIMM8P20, MEP_OPERAND_IMM8P20
 , MEP_OPERAND_CROP, MEP_OPERAND_CRQP, MEP_OPERAND_CRPP, MEP_OPERAND_IVC_X_0_2
 , MEP_OPERAND_IVC_X_0_3, MEP_OPERAND_IVC_X_0_4, MEP_OPERAND_IVC_X_0_5, MEP_OPERAND_IMM16P0
 , MEP_OPERAND_SIMM16P0, MEP_OPERAND_IVC2RM, MEP_OPERAND_IVC2CRN, MEP_OPERAND_IVC2CCRN
 , MEP_OPERAND_IVC2C3CCRN, MEP_OPERAND_MAX
} CGEN_OPERAND_TYPE;

/* Number of operands types.  */
#define MAX_OPERANDS 145

/* Maximum number of operands referenced by any insn.  */
#define MAX_OPERAND_INSTANCES 8

/* Insn attribute indices.  */

/* Enum declaration for cgen_insn attrs.  */
typedef enum cgen_insn_attr {
  CGEN_INSN_ALIAS, CGEN_INSN_VIRTUAL, CGEN_INSN_UNCOND_CTI, CGEN_INSN_COND_CTI
 , CGEN_INSN_SKIP_CTI, CGEN_INSN_DELAY_SLOT, CGEN_INSN_RELAXABLE, CGEN_INSN_RELAXED
 , CGEN_INSN_NO_DIS, CGEN_INSN_PBB, CGEN_INSN_OPTIONAL_BIT_INSN, CGEN_INSN_OPTIONAL_MUL_INSN
 , CGEN_INSN_OPTIONAL_DIV_INSN, CGEN_INSN_OPTIONAL_DEBUG_INSN, CGEN_INSN_OPTIONAL_LDZ_INSN, CGEN_INSN_OPTIONAL_ABS_INSN
 , CGEN_INSN_OPTIONAL_AVE_INSN, CGEN_INSN_OPTIONAL_MINMAX_INSN, CGEN_INSN_OPTIONAL_CLIP_INSN, CGEN_INSN_OPTIONAL_SAT_INSN
 , CGEN_INSN_OPTIONAL_UCI_INSN, CGEN_INSN_OPTIONAL_DSP_INSN, CGEN_INSN_OPTIONAL_CP_INSN, CGEN_INSN_OPTIONAL_CP64_INSN
 , CGEN_INSN_OPTIONAL_VLIW64, CGEN_INSN_MAY_TRAP, CGEN_INSN_VLIW_ALONE, CGEN_INSN_VLIW_NO_CORE_NOP
 , CGEN_INSN_VLIW_NO_COP_NOP, CGEN_INSN_VLIW64_NO_MATCHING_NOP, CGEN_INSN_VLIW32_NO_MATCHING_NOP, CGEN_INSN_VOLATILE
 , CGEN_INSN_END_BOOLS, CGEN_INSN_START_NBOOLS = 31, CGEN_INSN_MACH, CGEN_INSN_ISA
 , CGEN_INSN_CPTYPE, CGEN_INSN_CRET, CGEN_INSN_LATENCY, CGEN_INSN_CONFIG
 , CGEN_INSN_SLOTS, CGEN_INSN_END_NBOOLS
} CGEN_INSN_ATTR;

/* Number of non-boolean elements in cgen_insn_attr.  */
#define CGEN_INSN_NBOOL_ATTRS (CGEN_INSN_END_NBOOLS - CGEN_INSN_START_NBOOLS - 1)

/* cgen_insn attribute accessor macros.  */
#define CGEN_ATTR_CGEN_INSN_MACH_VALUE(attrs) ((attrs)->nonbool[CGEN_INSN_MACH-CGEN_INSN_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_INSN_ISA_VALUE(attrs) ((attrs)->nonbool[CGEN_INSN_ISA-CGEN_INSN_START_NBOOLS-1].bitset)
#define CGEN_ATTR_CGEN_INSN_CPTYPE_VALUE(attrs) ((attrs)->nonbool[CGEN_INSN_CPTYPE-CGEN_INSN_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_INSN_CRET_VALUE(attrs) ((attrs)->nonbool[CGEN_INSN_CRET-CGEN_INSN_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_INSN_LATENCY_VALUE(attrs) ((attrs)->nonbool[CGEN_INSN_LATENCY-CGEN_INSN_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_INSN_CONFIG_VALUE(attrs) ((attrs)->nonbool[CGEN_INSN_CONFIG-CGEN_INSN_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_INSN_SLOTS_VALUE(attrs) ((attrs)->nonbool[CGEN_INSN_SLOTS-CGEN_INSN_START_NBOOLS-1].nonbitset)
#define CGEN_ATTR_CGEN_INSN_ALIAS_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_ALIAS)) != 0)
#define CGEN_ATTR_CGEN_INSN_VIRTUAL_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_VIRTUAL)) != 0)
#define CGEN_ATTR_CGEN_INSN_UNCOND_CTI_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_UNCOND_CTI)) != 0)
#define CGEN_ATTR_CGEN_INSN_COND_CTI_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_COND_CTI)) != 0)
#define CGEN_ATTR_CGEN_INSN_SKIP_CTI_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_SKIP_CTI)) != 0)
#define CGEN_ATTR_CGEN_INSN_DELAY_SLOT_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_DELAY_SLOT)) != 0)
#define CGEN_ATTR_CGEN_INSN_RELAXABLE_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_RELAXABLE)) != 0)
#define CGEN_ATTR_CGEN_INSN_RELAXED_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_RELAXED)) != 0)
#define CGEN_ATTR_CGEN_INSN_NO_DIS_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_NO_DIS)) != 0)
#define CGEN_ATTR_CGEN_INSN_PBB_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_PBB)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_BIT_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_BIT_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_MUL_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_MUL_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_DIV_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_DIV_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_DEBUG_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_DEBUG_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_LDZ_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_LDZ_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_ABS_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_ABS_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_AVE_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_AVE_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_MINMAX_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_MINMAX_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_CLIP_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_CLIP_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_SAT_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_SAT_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_UCI_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_UCI_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_DSP_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_DSP_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_CP_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_CP_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_CP64_INSN_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_CP64_INSN)) != 0)
#define CGEN_ATTR_CGEN_INSN_OPTIONAL_VLIW64_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_OPTIONAL_VLIW64)) != 0)
#define CGEN_ATTR_CGEN_INSN_MAY_TRAP_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_MAY_TRAP)) != 0)
#define CGEN_ATTR_CGEN_INSN_VLIW_ALONE_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_VLIW_ALONE)) != 0)
#define CGEN_ATTR_CGEN_INSN_VLIW_NO_CORE_NOP_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_VLIW_NO_CORE_NOP)) != 0)
#define CGEN_ATTR_CGEN_INSN_VLIW_NO_COP_NOP_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_VLIW_NO_COP_NOP)) != 0)
#define CGEN_ATTR_CGEN_INSN_VLIW64_NO_MATCHING_NOP_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_VLIW64_NO_MATCHING_NOP)) != 0)
#define CGEN_ATTR_CGEN_INSN_VLIW32_NO_MATCHING_NOP_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_VLIW32_NO_MATCHING_NOP)) != 0)
#define CGEN_ATTR_CGEN_INSN_VOLATILE_VALUE(attrs) (((attrs)->bool_ & (1 << CGEN_INSN_VOLATILE)) != 0)

/* cgen.h uses things we just defined.  */
#include "opcode/cgen.h"

extern const struct cgen_ifld mep_cgen_ifld_table[];

/* Attributes.  */
extern const CGEN_ATTR_TABLE mep_cgen_hardware_attr_table[];
extern const CGEN_ATTR_TABLE mep_cgen_ifield_attr_table[];
extern const CGEN_ATTR_TABLE mep_cgen_operand_attr_table[];
extern const CGEN_ATTR_TABLE mep_cgen_insn_attr_table[];

/* Hardware decls.  */

extern CGEN_KEYWORD mep_cgen_opval_h_gpr;
extern CGEN_KEYWORD mep_cgen_opval_h_csr;
extern CGEN_KEYWORD mep_cgen_opval_h_cr64;
extern CGEN_KEYWORD mep_cgen_opval_h_cr;
extern CGEN_KEYWORD mep_cgen_opval_h_ccr;
extern CGEN_KEYWORD mep_cgen_opval_h_cr_ivc2;
extern CGEN_KEYWORD mep_cgen_opval_h_ccr_ivc2;

extern const CGEN_HW_ENTRY mep_cgen_hw_table[];



   #ifdef __cplusplus
   }
   #endif

#endif /* MEP_CPU_H */
