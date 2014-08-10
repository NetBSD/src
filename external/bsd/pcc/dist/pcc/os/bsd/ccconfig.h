/*	Id: ccconfig.h,v 1.3 2014/03/09 09:32:58 ragge Exp 	*/	
/*	$NetBSD: ccconfig.h,v 1.1.1.2.24.1 2014/08/10 07:10:07 tls Exp $	*/

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
#define	CPPADD	{ "-D__BSD2_11__", "-DBSD2_11", NULL }

/* host-dependent */
#define CRTBEGIN	0
#define CRTEND		0
#define CRTI		0
#define CRTN		0

#ifdef LANG_F77
#define F77LIBLIST { "-L/usr/lib", "-lF77", "-lI77", "-lm", "-lc", NULL };
#endif

#if defined(mach_pdp11)
#define	CPPMDADD { "-D__pdp11__", "-Dpdp11", NULL, }
#elif defined(mach_nova)
#define	CPPMDADD { "-D__nova__", "-Dnova", NULL, }
#else
#error defines for arch missing
#endif
