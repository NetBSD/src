/*	$NetBSD: ld.h,v 1.2 1998/12/17 23:36:38 pk Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Definitions and functions prototypes shared between the
 * link-editor, ld(1), and the run-time linker, ld.so(1).
 */

#define SUN_COMPAT

#include "md.h"
#include "link.h"

/* If compiled with GNU C, use the built-in alloca */
#if defined(__GNUC__) || defined(sparc)
#define alloca __builtin_alloca
#endif

/* Align to power-of-two boundary */
#define PALIGN(x,p)	(((x) +  (u_long)(p) - 1) & (-(u_long)(p)))

/* Align to machine dependent boundary */
#define MALIGN(x)	PALIGN(x,MAX_ALIGNMENT)

#ifndef nounderscore
#define ETEXT_SYM	"_etext"
#define EDATA_SYM	"_edata"
#define END_SYM		"_end"
#define DYN_SYM		"__DYNAMIC"
#define GOT_SYM		"__GLOBAL_OFFSET_TABLE_"
#define PLT_SYM		"__PROCEDURE_LINKAGE_TABLE_"
#define LPREFIX		'L'
#else
#define ETEXT_SYM	"etext"
#define EDATA_SYM	"edata"
#define END_SYM		"end"
#define DYN_SYM		"_DYNAMIC"
#define GOT_SYM		"_GLOBAL_OFFSET_TABLE_"
#define PLT_SYM		"_PROCEDURE_LINKAGE_TABLE_"
#define LPREFIX		'.'
#endif

/*
 * Default macros; these may be overridden in <arch/---/md.h>.
 * Note: we only test the existence of RELOC_ADDRESS, so
 * you should override all or nothing at all.
 *
 * See ld.c for an addtional information on their use.
 */
#ifndef RELOC_ADDRESS

#define RELOC_ADDRESS(r)		((r)->r_address)
#define RELOC_EXTERN_P(r)		((r)->r_extern)
#define RELOC_TYPE(r)			((r)->r_symbolnum)
#define RELOC_SYMBOL(r)			((r)->r_symbolnum)
#define RELOC_MEMORY_SUB_P(r)		0
#define RELOC_MEMORY_ADD_P(r)		1
#undef RELOC_ADD_EXTRA
#define RELOC_PCREL_P(r)		((r)->r_pcrel)
#define RELOC_VALUE_RIGHTSHIFT(r)	0
#if defined(RTLD) && defined(SUN_COMPAT)
#define RELOC_TARGET_SIZE(r)		(2)	/* !!!!! Sun BUG compatible */
#else
#define RELOC_TARGET_SIZE(r)		((r)->r_length)
#endif
#define RELOC_TARGET_BITPOS(r)		0
#define RELOC_TARGET_BITSIZE(r)		32

#define RELOC_JMPTAB_P(r)		((r)->r_jmptable)
#define RELOC_BASEREL_P(r)		((r)->r_baserel)
#define RELOC_RELATIVE_P(r)		((r)->r_relative)
#define RELOC_COPY_P(r)			((r)->r_copy)
#define RELOC_LAZY_P(r)			((r)->r_jmptable)

#define CHECK_GOT_RELOC(r)		((r)->r_pcrel)
#define RELOC_PIC_TYPE(r)		((r)->r_baserel? \
						PIC_TYPE_LARGE:PIC_TYPE_NONE)

#endif /* RELOC_ADDRESS */

/* A separately overridable macros */
#ifndef RELOC_INIT_SEGMENT_RELOC
#define RELOC_INIT_SEGMENT_RELOC(r)
#endif

#ifndef MAX_GOTOFF
#define MAX_GOTOFF(x)	(LONG_MAX)
#endif

#ifndef MIN_GOTOFF
#define MIN_GOTOFF(x)	(LONG_MIN)
#endif

#ifndef TEXT_START
#define TEXT_START(x)		N_TXTADDR(x)
#endif

#ifndef DATA_START
#define DATA_START(x)		N_DATADDR(x)
#endif

/* Macros dealing with the set element symbols defined in a.out.h */
#define	SET_ELEMENT_P(x)	((x) >= N_SETA && (x) <= (N_SETB|N_EXT))
#define TYPE_OF_SET_ELEMENT(x)	((x) - N_SETA + N_ABS)

#define N_ISWEAK(p)		(N_BIND(p) & BIND_WEAK)

/* Functions that must be provided by arch/md.c: */
void	md_init_header	  __P((struct exec *, int, int));
long	md_get_addend	  __P((struct relocation_info *, unsigned char *));
void	md_relocate	  __P((struct relocation_info *, long,
			       unsigned char *, int));
int	md_make_reloc	  __P((struct relocation_info *,
			       struct relocation_info *, int));
void	md_set_breakpoint __P((long, long *));
void	md_make_jmpslot	  __P((jmpslot_t *, long, long));
void	md_fix_jmpslot	  __P((jmpslot_t *, long, u_long, int));
void	md_make_jmpreloc  __P((struct relocation_info *,
			       struct relocation_info *, int));
void	md_make_gotreloc  __P((struct relocation_info *,
			       struct relocation_info *, int));
void	md_make_cpyreloc  __P((struct relocation_info *,
			       struct relocation_info *));
int	md_midcompat	  __P((struct exec *));
