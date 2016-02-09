/*	Id: ccconfig.h,v 1.2 2015/01/08 06:31:11 plunky Exp 	*/	
/*	$NetBSD: ccconfig.h,v 1.1.1.1 2016/02/09 20:29:21 plunky Exp $	*/

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
#define CPPADD	{ "-D__ANDROID__", "-D__linux__", "-D__ELF__", NULL, }

#define CRT0		"crt1.o"
#define GCRT0		"gcrt1.o"
#define DYNLINKLIB	"/system/bin/linker"

#define STARTLABEL "_start"

#if defined(mach_i386)
#define CPPMDADD	{ "-D__i386__", NULL, }
#elif defined(mach_amd64)
#define CPPMDADD	{ "-D__x86_64__", "-D__x86_64", "-D__amd64__", \
	"-D__amd64", "-D__LP64__", "-D_LP64", NULL, }
#define	DEFLIBDIRS	{ "/usr/lib64/", 0 }
#elif defined(mach_mips)
#define CPPMDADD { "-D__mips__", NULL, }
#elif defined(mach_arm)
#define CPPMDADD { "-D__arm__", NULL, }
#else
#error defines for arch missing
#endif


#define PCC_EARLY_ARG_CHECK	{			\
	if (match(argp, "-no-canonical-prefixes")) {	\
		continue;				\
	}						\
}

