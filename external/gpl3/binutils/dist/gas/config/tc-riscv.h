/* tc-riscv.h -- header file for tc-riscv.c.
   Copyright (C) 2011-2018 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (andrew@sifive.com).
   Based on MIPS target.

   This file is part of GAS.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#ifndef TC_RISCV
#define TC_RISCV

#include "opcode/riscv.h"

struct frag;
struct expressionS;

#define TARGET_BYTES_BIG_ENDIAN 0

#define TARGET_ARCH bfd_arch_riscv

#define WORKING_DOT_WORD	1
#define LOCAL_LABELS_FB 	1

/* Symbols named FAKE_LABEL_NAME are emitted when generating DWARF, so make
   sure FAKE_LABEL_NAME is printable.  It still must be distinct from any
   real label name.  So, append a space, which other labels can't contain.  */
#define FAKE_LABEL_NAME ".L0 "
/* Changing the special character in FAKE_LABEL_NAME requires changing
   FAKE_LABEL_CHAR too.  */
#define FAKE_LABEL_CHAR ' '

#define md_relax_frag(segment, fragp, stretch) \
  riscv_relax_frag (segment, fragp, stretch)
extern int riscv_relax_frag (asection *, struct frag *, long);

#define md_section_align(seg,size)	(size)
#define md_undefined_symbol(name)	(0)
#define md_operand(x)

extern bfd_boolean riscv_frag_align_code (int);
#define md_do_align(N, FILL, LEN, MAX, LABEL)				\
  if ((N) != 0 && !(FILL) && !need_pass_2 && subseg_text_p (now_seg))	\
    {									\
      if (riscv_frag_align_code (N))					\
	goto LABEL;							\
    }

extern void riscv_handle_align (fragS *);
#define HANDLE_ALIGN riscv_handle_align

#define MAX_MEM_FOR_RS_ALIGN_CODE 7

/* The ISA of the target may change based on command-line arguments.  */
#define TARGET_FORMAT riscv_target_format()
extern const char * riscv_target_format (void);

#define md_after_parse_args() riscv_after_parse_args()
extern void riscv_after_parse_args (void);

#define md_parse_long_option(arg) riscv_parse_long_option (arg)
extern int riscv_parse_long_option (const char *);

#define md_pre_output_hook riscv_pre_output_hook()
extern void riscv_pre_output_hook (void);

/* Let the linker resolve all the relocs due to relaxation.  */
#define tc_fix_adjustable(fixp) 0
#define md_allow_local_subtract(l,r,s) 0

/* Values passed to md_apply_fix don't include symbol values.  */
#define MD_APPLY_SYM_VALUE(FIX) 0

/* Global syms must not be resolved, to support ELF shared libraries.  */
#define EXTERN_FORCE_RELOC			\
  (OUTPUT_FLAVOR == bfd_target_elf_flavour)

/* Postpone text-section label subtraction calculation until linking, since
   linker relaxations might change the deltas.  */
#define TC_FORCE_RELOCATION_SUB_SAME(FIX, SEG)	\
  (GENERIC_FORCE_RELOCATION_SUB_SAME (FIX, SEG)	\
   || ((SEG)->flags & SEC_CODE) != 0)
#define TC_FORCE_RELOCATION_SUB_LOCAL(FIX, SEG) 1
#define TC_VALIDATE_FIX_SUB(FIX, SEG) 1
#define TC_FORCE_RELOCATION_LOCAL(FIX) 1
#define DIFF_EXPR_OK 1

extern void riscv_pop_insert (void);
#define md_pop_insert()		riscv_pop_insert ()

#define TARGET_USE_CFIPOP 1

#define tc_cfi_frame_initial_instructions riscv_cfi_frame_initial_instructions
extern void riscv_cfi_frame_initial_instructions (void);

#define tc_regname_to_dw2regnum tc_riscv_regname_to_dw2regnum
extern int tc_riscv_regname_to_dw2regnum (char *);

#define DWARF2_DEFAULT_RETURN_COLUMN X_RA

/* Even on RV64, use 4-byte alignment, as F registers may be only 32 bits.  */
#define DWARF2_CIE_DATA_ALIGNMENT -4

#define elf_tc_final_processing riscv_elf_final_processing
extern void riscv_elf_final_processing (void);

/* Adjust debug_line after relaxation.  */
#define DWARF2_USE_FIXED_ADVANCE_PC 1

#endif /* TC_RISCV */
