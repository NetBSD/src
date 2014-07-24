/*	Id: ccconfig.h,v 1.20 2014/06/04 06:43:50 gmcgarry Exp 	*/	
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
#define	CPPADD		{ "-D__Darwin__", "-D__APPLE__", "-D__MACH__", "-D__APPLE_CPP__", NULL }
#define	CRT0		"crt1.o"
#define GCRT0		"gcrt1.o"
#define CRTBEGIN_T	0
#define CRTEND_T	0
#define CRTBEGIN	0
#define CRTEND		0
#define CRTBEGIN_S	"dylib1.o"
#define CRTEND_S	0
#define CRTI		0
#define CRTN		0
#define DEFLIBS		{ "-lSystem", "-lpcc", NULL }
#define DEFPROFLIBS	{ "-lSystem_profile", "-lpcc", NULL }
#define STARTLABEL "start"

#ifdef LANG_F77
#define F77LIBLIST { "-L" PCCLIBDIR, "-lF77", "-lI77", "-lm", "-lc", NULL };
#endif

/*
ld -arch ppc -weak_reference_mismatches non-weak -o a.out -lcrt1.o -lcrt2.o -L/usr/lib/gcc/powerpc-apple-darwin8/4.0.1 hello_ppc.o -lgcc -lSystemStubs -lSystem
*/

#if defined(mach_i386)
#define	CPPMDADD { "-D__i386__", "-D__LITTLE_ENDIAN__", NULL }
#elif defined(mach_powerpc)
#define	CPPMDADD { "-D__ppc__", "-D__BIG_ENDIAN__", NULL }
#elif defined(mach_amd64)
#define	CPPMDADD { "-D__x86_64__", "-D__LITTLE_ENDIAN__", NULL }
#elif define(mach_m68k)
#define	CPPMDADD { "-D__m68k__", "-D__BIG_ENDIAN__", NULL }
#else
#error defines for arch missing
#endif


/*
 * Deal with some darwin-specific args.
 */
#define	PCC_EARLY_ARG_CHECK	{						\
	if (match(argp, "-install_name")) {				\
		strlist_append(&middle_linker_flags, argp);		\
		strlist_append(&middle_linker_flags, nxtopt(0));	\
		continue;						\
	} else if (match(argp, "-compatibility_version") ||		\
	    match(argp, "-current_version")) {				\
		strlist_append(&middle_linker_flags, argp);		\
		strlist_append(&middle_linker_flags, nxtopt(0));	\
		continue;						\
	} else if (strcmp(argp, "-dynamiclib") == 0) {			\
		shared = 1;						\
		continue;						\
	} else if (strcmp(argp, "-shared") == 0) {			\
		oerror(argp);						\
		continue;						\
	} else if (strncmp(argp, "-mmacosx-version-min", 20) == 0) {	\
		char tmp[10];						\
		char *p = &argp[21];					\
		int idx = 0;						\
		while (*p != 0) {					\
			if (*p != '.')					\
				tmp[idx++] = *p;			\
			p++;						\
		}							\
		while (idx < 4)						\
			tmp[idx++] = '0';				\
		tmp[idx] = 0;						\
		strlist_append(&preprocessor_flags, cat("-D__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__=", tmp)); \
		strlist_append(&middle_linker_flags, "-macosx_version_min");		\
		strlist_append(&middle_linker_flags, &argp[21]);	\
		continue;						\
	} else if (strcmp(argp, "-framework") == 0) {			\
		strlist_append(&middle_linker_flags, argp);		\
		strlist_append(&middle_linker_flags, nxtopt(0));	\
		continue;						\
	} 								\
}


