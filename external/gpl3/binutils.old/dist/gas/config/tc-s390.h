/* tc-s390.h -- Header file for tc-s390.c.
   Copyright (C) 2000-2025 Free Software Foundation, Inc.
   Written by Martin Schwidefsky (schwidefsky@de.ibm.com).

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#define TC_S390

struct fix;

#define TC_FORCE_RELOCATION(FIX) tc_s390_force_relocation(FIX)
extern int tc_s390_force_relocation (struct fix *);

/* Don't resolve foo@PLT-bar to offset@PLT.  */
#define TC_FORCE_RELOCATION_SUB_SAME(FIX, SEG)	\
  (GENERIC_FORCE_RELOCATION_SUB_SAME (FIX, SEG)	\
   || TC_FORCE_RELOCATION (FIX))

#define tc_fix_adjustable(X)  tc_s390_fix_adjustable(X)
extern int tc_s390_fix_adjustable (struct fix *);

/* Values passed to md_apply_fix don't include symbol values.  */
#define MD_APPLY_SYM_VALUE(FIX) 0

/* The target BFD architecture.  */
#define TARGET_ARCH bfd_arch_s390
extern enum bfd_architecture s390_arch (void);

/* The target BFD format.  */
#define TARGET_FORMAT s390_target_format()
extern const char *s390_target_format (void);

/* Set the endianness we are using.  */
#define TARGET_BYTES_BIG_ENDIAN 1

/* Whether or not the target is big endian */
extern int target_big_endian;

/* Permit temporary numeric labels.  */
#define LOCAL_LABELS_FB 1

/* $ is used to refer to the current location.  */
/* #define DOLLAR_DOT */

/* We need to be able to make relocations involving the difference of
   two symbols.  This includes the difference of two symbols when
   one of them is undefined (this comes up in PIC code generation).
 */
#define UNDEFINED_DIFFERENCE_OK

/* foo-. gets turned into PC relative relocs */
#define DIFF_EXPR_OK

/* We don't need to handle .word strangely.  */
#define WORKING_DOT_WORD

#define md_number_to_chars           number_to_chars_bigendian

#define NOP_OPCODE 0x07

/* call md_pcrel_from_section, not md_pcrel_from */
#define MD_PCREL_FROM_SECTION(FIX, SEC) md_pcrel_from_section(FIX, SEC)

#define md_operand(x)

extern void s390_md_finish (void);
#define md_finish() s390_md_finish ()

#define TARGET_USE_CFIPOP 1

#define tc_cfi_frame_initial_instructions s390_cfi_frame_initial_instructions
extern void s390_cfi_frame_initial_instructions (void);

#define tc_regname_to_dw2regnum tc_s390_regname_to_dw2regnum
extern int tc_s390_regname_to_dw2regnum (char *regname);

extern int s390_cie_data_alignment;

#define DWARF2_LINE_MIN_INSN_LENGTH     1
#define DWARF2_DEFAULT_RETURN_COLUMN    14
#define DWARF2_CIE_DATA_ALIGNMENT       s390_cie_data_alignment

extern void s390_elf_final_processing (void);

#define elf_tc_final_processing s390_elf_final_processing

/* SFrame.  */

/* Whether SFrame stack trace info is supported.  */
extern bool s390_support_sframe_p (void);
#define support_sframe_p s390_support_sframe_p

/* The stack pointer DWARF register number for SFrame CFA tracking.  */
extern const unsigned int s390_sframe_cfa_sp_reg;
#define SFRAME_CFA_SP_REG s390_sframe_cfa_sp_reg

/* The frame pointer DWARF register number for SFrame CFA and FP tracking.  */
extern const unsigned int s390_sframe_cfa_fp_reg;
#define SFRAME_CFA_FP_REG s390_sframe_cfa_fp_reg

/* The return address DWARF register number for SFrame RA tracking.  */
extern const unsigned int s390_sframe_cfa_ra_reg;
#define SFRAME_CFA_RA_REG s390_sframe_cfa_ra_reg

/* Whether SFrame return address tracking is needed.  */
extern bool s390_sframe_ra_tracking_p (void);
#define sframe_ra_tracking_p s390_sframe_ra_tracking_p

/* The fixed offset from CFA for SFrame to recover the return address.
   (useful only when SFrame RA tracking is not needed).  */
extern offsetT s390_sframe_cfa_ra_offset (void);
#define sframe_cfa_ra_offset s390_sframe_cfa_ra_offset

/* The abi/arch identifier for SFrame.  */
unsigned char s390_sframe_get_abi_arch (void);
#define sframe_get_abi_arch s390_sframe_get_abi_arch
