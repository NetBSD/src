/*	Id: ccconfig.h,v 1.30 2014/04/08 19:52:27 ragge Exp 	*/	
/*	$NetBSD: ccconfig.h,v 1.1.1.5 2014/07/24 19:29:33 plunky Exp $	*/

/*
 * Copyright (c) 2004 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */

/* common cpp predefines */
#define	CPPADD	{ "-D__NetBSD__", "-D__ELF__", NULL, }

#ifdef LANG_F77
#define F77LIBLIST { "-L/usr/local/lib", "-lF77", "-lI77", "-lm", "-lc", NULL };
#endif

/* host-independent */
#define	DYNLINKER { "-dynamic-linker", "/usr/libexec/ld.elf_so", NULL }

#define CRTEND_T	"crtend.o"

#define DEFLIBS		{ "-lc", NULL }
#define DEFPROFLIBS	{ "-lc_p", NULL }
#define DEFCXXLIBS	{ "-lp++", "-lc", NULL }

#if defined(mach_amd64)
#define CPPMDADD \
	{ "-D__x86_64__", "-D__x86_64", "-D__amd64__", "-D__amd64", \
	  "-D__LP64__", "-D_LP64", NULL, }
#elif defined(mach_arm)
#define	CPPMDADD { "-D__arm__", NULL, }
#elif defined(mach_i386)
#define	CPPMDADD { "-D__i386__", NULL, }
#define	PCC_SIZE_TYPE		"unsigned int"
#define	PCC_PTRDIFF_TYPE	"int"
#elif defined(mach_m68k)
#define	CPPMDADD { "-D__mc68000__", "-D__mc68020__", "-D__m68k__", NULL, }
#undef DEFLIBS
#define DEFLIBS	{ "-lc", "-lgcc", NULL }
#define STARTLABEL "_start"
#elif defined(mach_mips)
#define	CPPMDADD { "-D__mips__", NULL, }
#elif defined(mach_pdp10)
#define CPPMDADD { "-D__pdp10__", NULL, }
#elif defined(mach_powerpc)
#define	CPPMDADD { "-D__ppc__", NULL, }
#define STARTLABEL "_start"
#elif defined(mach_vax)
#define CPPMDADD { "-D__vax__", NULL, }
#define	PCC_EARLY_SETUP { kflag = 1; }
#elif defined(mach_sparc64)
#define CPPMDADD { "-D__sparc64__", NULL, }
#else
#error defines for arch missing
#endif

#ifndef	PCC_WINT_TYPE
#define	PCC_WINT_TYPE		"int"
#endif
#ifndef	PCC_SIZE_TYPE
#define	PCC_SIZE_TYPE		"unsigned long"
#endif
#ifndef	PCC_PTRDIFF_TYPE
#define	PCC_PTRDIFF_TYPE	"long"
#endif
