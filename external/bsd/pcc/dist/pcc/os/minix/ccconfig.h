/*	 Id: ccconfig.h,v 1.6 2014/12/24 08:43:28 plunky Exp  */	
/*	 $NetBSD: ccconfig.h,v 1.1.1.2 2016/02/09 20:29:21 plunky Exp $ */
/*
 * Escrit per Antoine Leca pel projecte PCC, 2011-03.
 * Copyright (C) 2011 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Various settings that controls how the C compiler works.
 *
 * MINIX (http://www.minix3.org), both Classic a.out=PC/IX and ELF
 */

#ifdef ELFABI
/* MINIX versions 3.2 (2011) and up, using NetBSD userland. */

/* common cpp predefines */
#define CPPADD	{ "-D__minix", "-D__ELF__", NULL }

#define	DYNLINKLIB	"/libexec/ld.elf_so"

#define CRTEND_T	"crtend.o"

#define DEFLIBS		{ "-lc", NULL }
#define DEFPROFLIBS	{ "-lc_p", NULL }
#define DEFCXXLIBS	{ "-lp++", "-lc", NULL }

#if defined(mach_amd64)
#include "../inc/amd64.h"
#define	PCC_SIZE_TYPE		"unsigned long"
#define	PCC_PTRDIFF_TYPE	"long"
#elif defined(mach_arm)
#define	CPPMDADD	{ "-D__arm__", NULL, }
#elif defined(mach_i386)
#define	CPPMDADD	{ "-D__i386", "-D__i386__", NULL, }
#else
#error defines for arch missing
#endif

#ifndef	PCC_WINT_TYPE
#define	PCC_WINT_TYPE		"int"
#endif
#ifndef	PCC_SIZE_TYPE
#define	PCC_SIZE_TYPE		"unsigned int"
#endif
#ifndef	PCC_PTRDIFF_TYPE
#define	PCC_PTRDIFF_TYPE	"int"
#endif

#elif defined(AOUTABI)
/* MINIX 2 or 3.1.x, a.out-like format derived from PC/IX. */

/* common cpp predefines */
#define CPPADD	{ "-D__minix", NULL }

/* linker stuff */
#define STARTLABEL	"crtso"
#define CRT0		"crtso.o"
#ifdef notyet
#define GCRT0	"/usr/lib/pcc/gcrtso.o"
#endif

#define CRTBEGIN	0
#define CRTEND		0
#define CRTBEGIN_S	0
#define CRTEND_S	0
#define CRTBEGIN_T	0
#define CRTEND_T	0

#define CRTI		0
#define CRTN		"-lend"

#if defined(mach_i386)
#define CPPMDADD { "-D__i386", "-D__i386__", \
	"-D_EM_WSIZE=4", "-D_EM_PSIZE=4", "-D_EM_LSIZE=4", \
	"-D_EM_SSIZE=2", "-D_EM_FSIZE=4", "-D_EM_DSIZE=8", \
	NULL, }
#else
#define CPPMDADD { "-D__i86", \
	"-D_EM_WSIZE=2", "-D_EM_PSIZE=2", "-D_EM_LSIZE=4", \
	"-D_EM_SSIZE=2", "-D_EM_FSIZE=4", "-D_EM_DSIZE=8", \
	NULL, }
#endif

#else
#error defines for ABI missing
#endif
