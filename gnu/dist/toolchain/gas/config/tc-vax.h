/* tc-vax.h -- Header file for tc-vax.c.
   Copyright 1987, 1991, 1992, 1993, 1995, 1996, 1997, 2000
   Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#define TC_VAX 1

#define TARGET_BYTES_BIG_ENDIAN 0

#ifdef OBJ_AOUT
#ifdef TE_NetBSD
#define	TARGET_FORMAT "a.out-vax-netbsd"
#endif
#ifndef TARGET_FORMAT
#define	TARGET_FORMAT "a.out-vax-bsd"
#endif
#endif

#ifdef OBJ_VMS
#define	TARGET_FORMAT "vms-vax"
#endif

#ifdef OBJ_ELF
#define	TARGET_FORMAT "elf32-vax"
#endif

#define	BFD_ARCH	bfd_arch_vax	/* for non-BFD_ASSEMBLER */
#define	TARGET_ARCH	bfd_arch_vax	/* BFD_ASSEMBLER */

#ifdef BFD_ASSEMBLER
#define NO_RELOC	BFD_RELOC_NONE
#else
enum reloc_type
  {
     NO_RELOC, NO_RELOC2, RELOC_32, RELOC_GLOB_DAT,
     RELOC_JMP_TBL, RELOC_JMP_SLOT
  };
#define NEED_FX_R_TYPE
#endif
#define NOP_OPCODE	0x01

#define tc_aout_pre_write_hook(x)	{;}	/* not used */
#define tc_crawl_symbol_chain(a)	{;}	/* not used */
#define md_operand(x)

long md_chars_to_number PARAMS ((unsigned char *, int));

extern const struct relax_type md_relax_table[];
#define TC_GENERIC_RELAX_TABLE md_relax_table

/* This expression evaluates to false if the relocation is for a local object
   for which we still want to do the relocation at runtime.  True if we
   are willing to perform this relocation while building the .o file.  If
   the reloc is against an externally visible symbol, then the assembler
   should never do the relocation.  */

#ifdef BFD_ASSEMBLER
#define	TC_RELOC_RTSYM_LOC_FIXUP(FIX)			\
	((FIX)->fx_addsy == NULL			\
	 || (! S_IS_EXTERNAL ((FIX)->fx_addsy)		\
	     && ! S_IS_WEAK ((FIX)->fx_addsy)		\
	     && S_IS_DEFINED ((FIX)->fx_addsy)		\
	     && ! S_IS_COMMON ((FIX)->fx_addsy)))
#define	tc_fix_adjustable(FIX)				\
	(((FIX)->fx_addsy == NULL			\
	 || (!S_IS_EXTERNAL ((FIX)->fx_addsy)		\
	     && !S_IS_WEAK ((FIX)->fx_addsy)))		\
	 && (FIX)->fx_r_type != BFD_RELOC_32_PLT_PCREL	\
	 && (FIX)->fx_r_type != BFD_RELOC_32_GOT_PCREL)
#endif

/*
 * Local Variables:
 * comment-column: 0
 * fill-column: 131
 * End:
 */
