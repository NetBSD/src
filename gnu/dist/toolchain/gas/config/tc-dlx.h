/* tc-dlx.h -- Assemble for the DLX
   Copyright 2002 Free Software Foundation, Inc.

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

/* Initially created by Kuang Hwa Lin, 3/20/2002.  */

#include "write.h" /* For the definition of fixS.  */

#define TC_DLX

#ifndef BFD_ASSEMBLER
 #error DLX support requires BFD_ASSEMBLER
#endif

#ifndef  __BFD_H_SEEN__
#include "bfd.h"
#endif

/* The target BFD architecture.  */
#define TARGET_ARCH bfd_arch_dlx
#define TARGET_FORMAT "elf32-dlx"
#define TARGET_BYTES_BIG_ENDIAN	1

#define WORKING_DOT_WORD

#define LEX_DOLLAR 1

/* #define md_operand(x) */
extern void dlx_pop_insert              PARAMS ((void));
extern int set_dlx_skip_hi16_flag       PARAMS ((int));

#define md_pop_insert()		        dlx_pop_insert ()

#define md_convert_frag(b,s,f)		as_fatal ("alpha convert_frag\n")
#define md_convert_frag(b,s,f)		as_fatal ("alpha convert_frag\n")
#define md_estimate_size_before_relax(f,s) \
			(as_fatal ("estimate_size_before_relax called"),1)

#define tc_unrecognized_line(c) dlx_unrecognized_line (c)

extern int dlx_unrecognized_line PARAMS ((int));

#define tc_headers_hook(a)		;	/* not used */
#define tc_headers_hook(a)		;	/* not used */
#define tc_crawl_symbol_chain(a)	;	/* not used */
#define tc_coff_symbol_emit_hook(a)	;	/* not used */

#define AOUT_MACHTYPE 101
#define TC_COFF_FIX2RTYPE(fix_ptr) tc_coff_fix2rtype (fix_ptr)
#define BFD_ARCH bfd_arch_dlx
#define COFF_MAGIC DLXMAGIC
/* Should the reloc be output ?
	on the 29k, this is true only if there is a symbol attatched.
	on the h8, this is allways true, since no fixup is done
        on dlx, I have no idea!! but lets keep it here just for fun.
*/
#define TC_COUNT_RELOC(x) (x->fx_addsy)
#define TC_CONS_RELOC BFD_RELOC_32_PCREL

/* We need to force out some relocations when relaxing.  */
#define TC_FORCE_RELOCATION(fix) md_dlx_force_relocation (fix)
struct fix;
extern int md_dlx_force_relocation PARAMS ((struct fix *));

#define obj_fix_adjustable(fixP) md_dlx_fix_adjustable(fixP)
struct fix;
extern boolean md_dlx_fix_adjustable PARAMS ((struct fix *));

/* This arranges for gas/write.c to not apply a relocation if
   obj_fix_adjustable() says it is not adjustable.  */
#define TC_FIX_ADJUSTABLE(fixP) obj_fix_adjustable (fixP)

#define NEED_FX_R_TYPE

/* Zero Based Segment?? sound very dangerous to me!     */
#define ZERO_BASED_SEGMENTS

/* Permit temporary numeric labels.  */
#define LOCAL_LABELS_FB 1
#ifdef  LOCAL_LABELS_DOLLAR
#undef  LOCAL_LABELS_DOLLAR
#endif
#define LOCAL_LABELS_DOLLAR 0

#define DIFF_EXPR_OK		/* .-foo gets turned into PC relative relocs */

