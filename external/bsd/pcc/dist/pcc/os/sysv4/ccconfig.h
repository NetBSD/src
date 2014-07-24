/*	Id: ccconfig.h,v 1.1 2012/08/30 17:38:19 ragge Exp 	*/	
/*	$NetBSD: ccconfig.h,v 1.1.1.1 2014/07/24 19:29:37 plunky Exp $	*/

/*
 * Copyright (c) 2008 Adam Hoka.
 * Modified from sunos files slightly by Gregor Richards.
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
#define	CPPADD	{ "-Dunix", "-D__SVR4", "-D__unix", "-D__svr4__", "-D__ELF__", NULL }

/* host-dependent */
#define CRT0 "crt1.o"
#define GCRT0 "gcrt1.o"

#ifdef LANG_F77
#define F77LIBLIST { "-L/usr/local/lib", "-lF77", "-lI77", "-lm", "-lc", NULL };
#endif

+/* assumes GNU ld */
#define	DYNLINKER { "--dynamic-linker", "/usr/lib/ld.so.1", NULL }

#if defined(mach_i386)
#define	CPPMDADD { "-D__i386__", "-D__i386", NULL, }
#else
#error defines for arch missing
#endif

#define	STABS
