/* tc-rx.h - header file for Renesas RX
   Copyright 2008, 2009
   Free Software Foundation, Inc.

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

#define TC_RX

extern int target_big_endian;

#define LISTING_HEADER (target_big_endian ? "RX GAS BE" : "RX GAS LE")
#define LISTING_LHS_WIDTH 8
#define LISTING_WORD_SIZE 1

#define TARGET_ARCH bfd_arch_rx

/* Instruction bytes are big endian, data bytes can be either.  */
#define TARGET_BYTES_BIG_ENDIAN 0

#define TARGET_FORMAT (target_big_endian ? "elf32-rx-be" : "elf32-rx-le")

/* We don't need to handle .word strangely.  */
#define WORKING_DOT_WORD

/* Permit temporary numeric labels.  */
#define LOCAL_LABELS_FB 1
/* But make sure that the binutils treat them as locals.  */
#define LOCAL_LABEL_PREFIX '.'

/* Allow classic-style constants.  */
#define NUMBERS_WITH_SUFFIX 1

/* .-foo gets turned into PC relative relocs.  */
#define DIFF_EXPR_OK

#define md_end rx_md_end
extern void rx_md_end (void);

#define md_relax_frag rx_relax_frag
extern int rx_relax_frag (segT, fragS *, long);

#define TC_FRAG_TYPE struct rx_bytesT *
#define TC_FRAG_INIT rx_frag_init
extern void rx_frag_init (fragS *);

/* Call md_pcrel_from_section(), not md_pcrel_from().  */
#define MD_PCREL_FROM_SECTION(FIXP, SEC) md_pcrel_from_section (FIXP, SEC)
extern long md_pcrel_from_section (struct fix *, segT);

/* RX doesn't have a 32 bit PCREL relocations.  */
#define TC_FORCE_RELOCATION_SUB_LOCAL(FIX, SEG) 1

#define TC_VALIDATE_FIX_SUB(FIX, SEG)		\
  rx_validate_fix_sub (FIX)
extern int rx_validate_fix_sub (struct fix *);

#define TC_CONS_FIX_NEW(FRAG, WHERE, NBYTES, EXP) \
  rx_cons_fix_new (FRAG, WHERE, NBYTES, EXP)
extern void rx_cons_fix_new (fragS *, int, int, expressionS *);

#define tc_fix_adjustable(x) 0

#define md_do_align(n, fill, len, max, around)				\
  if ((n)								\
      && !need_pass_2							\
      && (!(fill)							\
	  || ((char)*(fill) == (char)0x03 && (len) == 1))		\
      && subseg_text_p (now_seg))					\
    {									\
      frag_align_code ((n), (max));					\
      goto around;							\
    }

#define MAX_MEM_FOR_RS_ALIGN_CODE 8
#define HANDLE_ALIGN(FRAG) rx_handle_align (FRAG)
extern void rx_handle_align (fragS *);

#define RELOC_EXPANSION_POSSIBLE 1
#define MAX_RELOC_EXPANSION      4

#define elf_tc_final_processing	rx_elf_final_processing
extern void rx_elf_final_processing (void);

extern bfd_boolean rx_use_conventional_section_names;
#define TEXT_SECTION_NAME	(rx_use_conventional_section_names ? ".text" : "P")
#define DATA_SECTION_NAME	(rx_use_conventional_section_names ? ".data" : "D_1")
#define BSS_SECTION_NAME	(rx_use_conventional_section_names ? ".bss"  : "B_1")

#define md_start_line_hook rx_start_line
extern void rx_start_line (void);
