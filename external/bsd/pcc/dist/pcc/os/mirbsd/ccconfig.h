/* Id: ccconfig.h,v 1.9 2012/08/05 14:35:00 ragge Exp  */	
/* $NetBSD: ccconfig.h,v 1.1.1.4.20.1 2014/08/10 07:10:08 tls Exp $ */
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
 * Configuration for pcc on a MirOS BSD (i386 or sparc) target
 */

/* === mi part === */

/* cpp MI defines */
#define CPPADD			{		\
	"-D__MirBSD__",				\
	"-D__OpenBSD__",			\
	"-D__unix__",				\
	"-D__ELF__",				\
	NULL					\
}

/* for dynamically linked binaries */
#define DYNLINKER		{		\
	"-dynamic-linker",			\
	"/usr/libexec/ld.so",			\
	NULL					\
}

#define CRTEND_T	"crtend.o"
#define DEFLIBS		{ "-lc", NULL }

/* === md part === */

#if defined(mach_i386)
#define CPPMDADD		{		\
	"-D__i386__",				\
	"-D__i386",				\
	"-Di386",				\
	NULL,					\
}
#elif defined(mach_sparc)
#error pcc does not support sparc yet
#else
#error this architecture is not supported by MirOS BSD
#endif
