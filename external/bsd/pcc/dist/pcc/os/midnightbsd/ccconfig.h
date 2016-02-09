/* Id: ccconfig.h,v 1.7 2014/12/24 08:43:28 plunky Exp  */	
/* $NetBSD: ccconfig.h,v 1.1.1.4 2016/02/09 20:29:20 plunky Exp $ */
/*-
 * Copyright (c) 2007, 2008
 *	Thorsten Glaser <tg@mirbsd.de>
 *
 * Provided that these terms and disclaimer and all copyright notices
 * are retained or reproduced in an accompanying document, permission
 * is granted to deal in this work without restriction, including un-
 * limited rights to use, publicly perform, distribute, sell, modify,
 * merge, give away, or sublicence.
 *
 * This work is provided "AS IS" and WITHOUT WARRANTY of any kind, to
 * the utmost extent permitted by applicable law, neither express nor
 * implied; without malicious intent or gross negligence. In no event
 * may a licensor, author or contributor be held liable for indirect,
 * direct, other damage, loss, or other issues arising in any way out
 * of dealing in the work, even if advised of the possibility of such
 * damage or existence of a defect, except proven that it results out
 * of said person's immediate fault when using the work as intended.
 */

/**
 * Configuration for pcc on a MidnightBSD (amd64, i386 or sparc64) target
 */

/* === mi part === */

/* cpp MI defines */
#define CPPADD			{		\
	"-D__MidnightBSD__",			\
	"-D__FreeBSD__",			\
	"-D__unix__",				\
	"-D__unix",				\
	"-Dunix",				\
	"-D__ELF__",				\
	"-D_LONGLONG",				\
	NULL					\
}

/* for dynamically linked binaries */
#define	DYNLINKLIB	"/libexec/ld-elf.so.1"

/* C run-time startup */
#define CRT0		"crt1.o"
#define GCRT0		"gcrt1.o"

#define CRTEND_T	"crtend.o"

/* === md part === */

#if defined(mach_i386)
#define CPPMDADD		{		\
	"-D__i386__",				\
	"-D__i386",				\
	"-Di386",				\
	NULL,					\
}
#elif defined(mach_sparc64)
#define CPPMDADD		{		\
	"-D__sparc64__",			\
	"-D__sparc_v9__",			\
	"-D__sparcv9",				\
	"-D__sparc__",				\
	"-D__sparc",				\
	"-Dsparc",				\
	"-D__arch64__",				\
	"-D__LP64__",				\
	"-D_LP64",				\
	NULL,					\
}
#elif defined(mach_amd64)
#error pcc does not support amd64 yet
#else
#error this architecture is not supported by MidnightBSD
#endif
