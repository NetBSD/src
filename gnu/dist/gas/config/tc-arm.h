/* This file is tc-arm.h
   Copyright (C) 1994, 1995, 1996, 1997 Free Software Foundation, Inc.
   Contributed by Richard Earnshaw (rwe@pegasus.esprit.ec.org)
	Modified by David Taylor (dtaylor@armltd.co.uk)

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

#define TC_ARM 1

#ifndef TARGET_BYTES_BIG_ENDIAN
#define TARGET_BYTES_BIG_ENDIAN 0
#endif

#define COFF_MAGIC ARMMAGIC
#define TARGET_ARCH bfd_arch_arm

#define AOUT_MACHTYPE 0

#define DIFF_EXPR_OK

#ifdef  LITTLE_ENDIAN
#undef  LITTLE_ENDIAN
#endif
#ifdef  BIG_ENDIAN
#undef  BIG_ENDIAN
#endif

#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321

#ifdef OBJ_AOUT
#ifdef TE_RISCIX
#define TARGET_FORMAT "a.out-riscix"
#else
#define ARM_BI_ENDIAN
#define TARGET_FORMAT \
  (target_big_endian ? "a.out-arm-big" : "a.out-arm-little")
#endif
#endif

#ifdef OBJ_AIF
#define TARGET_FORMAT "aif"
#endif

#ifdef OBJ_COFF
#define ARM_BI_ENDIAN
#ifdef TE_PE
#define TC_FORCE_RELOCATION(x) ((x)->fx_r_type==BFD_RELOC_RVA)
#define TARGET_FORMAT (target_big_endian ? "pe-arm-big" : "pe-arm-little")
#else
#define TARGET_FORMAT (target_big_endian ? "coff-arm-big" : "coff-arm-little")
/* Tell tc-arm.c to support runtime endian selection.  */
#endif
#endif

#define md_convert_frag(b,s,f)		{as_fatal ("arm convert_frag\n");}

extern void arm_cleanup PARAMS ((void));
extern void arm_start_line_hook PARAMS ((void));
extern void arm_frob_label PARAMS ((struct symbol *));
#define md_cleanup() arm_cleanup ()
#define md_start_line_hook() arm_start_line_hook ()
#define tc_frob_label(S) arm_frob_label (S)
/* We also need to mark assembler created symbols:  */
#define tc_frob_fake_label(S) arm_frob_label (S)
/* NOTE: The fake label creation in stabs.c:s_stab_generic() has
   deliberately not been updated to mark assembler created stabs
   symbols as Thumb.  */

#define obj_fix_adjustable(fixP) 0

/* We need to keep some local information on symbols.  */

#define TC_SYMFIELD_TYPE unsigned int
#define ARM_GET_FLAG(s)   	((s)->sy_tc)
#define ARM_SET_FLAG(s,v) 	((s)->sy_tc |= (v))
#define ARM_RESET_FLAG(s,v) 	((s)->sy_tc &= ~(v))

#define ARM_FLAG_THUMB 		(1 << 0)	/* The symbol is a Thumb symbol rather than an Arm symbol.  */
#define ARM_FLAG_INTERWORK 	(1 << 1)	/* The symbol is attached to code that suppports interworking.  */
#define THUMB_FLAG_FUNC		(1 << 2)	/* The symbol is attached to the start of a Thumb function.  */

#define ARM_IS_THUMB(s)		(ARM_GET_FLAG (s) & ARM_FLAG_THUMB)
#define ARM_IS_INTERWORK(s)	(ARM_GET_FLAG (s) & ARM_FLAG_INTERWORK)
#define THUMB_IS_FUNC(s)	(ARM_GET_FLAG (s) & THUMB_FLAG_FUNC)

#define ARM_SET_THUMB(s,t)      ((t) ? ARM_SET_FLAG (s, ARM_FLAG_THUMB)     : ARM_RESET_FLAG (s, ARM_FLAG_THUMB))
#define ARM_SET_INTERWORK(s,t)  ((t) ? ARM_SET_FLAG (s, ARM_FLAG_INTERWORK) : ARM_RESET_FLAG (s, ARM_FLAG_INTERWORK))
#define THUMB_SET_FUNC(s,t)     ((t) ? ARM_SET_FLAG (s, THUMB_FLAG_FUNC)    : ARM_RESET_FLAG (s, THUMB_FLAG_FUNC))


#define TC_FIX_TYPE PTR
#define TC_INIT_FIX_DATA(FIXP) ((FIXP)->tc_fix_data = NULL)

#define TC_START_LABEL(C,STR) \
  (c == ':' || (c == '/' && arm_data_in_code ()))
int arm_data_in_code PARAMS ((void));

#define tc_canonicalize_symbol_name(str) \
  arm_canonicalize_symbol_name (str);
char *arm_canonicalize_symbol_name PARAMS ((char *));

#if 0	/* It isn't as simple as this */
#define tc_frob_symbol(sym,punt)	\
{	if (S_IS_LOCAL (sym))		\
	  {				\
	    punt = 1;			\
	    sym->sy_used_in_reloc = 0;	\
	  }}
#endif 

/* Finish processing the entire symbol table:  */
#define tc_adjust_symtab() arm_adjust_symtab ()
extern void arm_adjust_symtab PARAMS ((void));

#if 0
#define tc_crawl_symbol_chain(a)	{;}	/* not used */
#define tc_headers_hook(a)		{;}	/* not used */
#endif

#define tc_aout_pre_write_hook(x)	{;}	/* not used */

#define LISTING_HEADER "ARM GAS "

#define OPTIONAL_REGISTER_PREFIX '%'

#define md_operand(x)

#define TC_HANDLES_FX_DONE

#define MD_APPLY_FIX3

#define LOCAL_LABELS_FB  1

/* end of tc-arm.h */
