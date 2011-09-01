/*	 Id: ccconfig.h,v 1.2 2011/06/04 19:27:26 plunky Exp  */	
/*	 $NetBSD: ccconfig.h,v 1.1.1.1 2011/09/01 12:47:17 plunky Exp $ */
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

#ifndef LIBDIR
#ifdef LIBEXECDIR
#define LIBDIR		LIBEXECDIR "../lib/"
#else
#define LIBDIR		"/usr/lib/"
#endif
#endif

#ifdef ELFABI
/* common cpp predefines */
#define CPPADD	{ "-D__minix", "-D__ELF__", NULL }

/* linker stuff */
#define STARTLABEL "_start"
#define CRT0FILE LIBDIR "crt1.o"
#define CRT0FILE_PROFILE LIBDIR "gcrt1.o"
#define LIBCLIBS { "-lc", "-lpcc", NULL }

#define STARTFILES { LIBDIR "crti.o", /*LIBDIR "crtbegin.o",*/ NULL }
#define ENDFILES { /*LIBDIR "crtend.o",*/ LIBDIR "crtn.o", NULL }

#define STARTFILES_S { LIBDIR "crti.o", /*LIBDIR "crtbeginS.o",*/ NULL }
#define ENDFILES_S { /*LIBDIR "crtendS.o",*/ LIBDIR "crtn.o", NULL }

#elif defined(AOUTABI)

/* common cpp predefines */
#define CPPADD	{ "-D__minix", NULL }

/* linker stuff */
#define STARTLABEL "crtso"
#define CRT0FILE LIBDIR "crtso.o"
/* #define CRT0FILE_PROFILE "/usr/lib/pcc/gcrtso.o" */
#define LIBCLIBS { "-L" LIBDIR "pcc", \
	"-lc", "-lpcc", "-lend", NULL }
#define noSTARTFILES { LIBDIR "crti.o", LIBDIR "crtbegin.o", NULL }
#define noENDFILES { LIBDIR "crtend.o", LIBDIR "crtn.o", NULL }

#else
#error defines for ABI missing
#endif

#if defined(mach_i386)
#define CPPMDADD { "-D__i386", \
	"-D_EM_WSIZE=4", "-D_EM_PSIZE=4", "-D_EM_LSIZE=4", \
	"-D_EM_SSIZE=2", "-D_EM_FSIZE=4", "-D_EM_DSIZE=8", \
	NULL, }
#else
#error defines for arch missing
#endif
