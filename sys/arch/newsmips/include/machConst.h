/*	$NetBSD: machConst.h,v 1.3 2003/08/07 16:28:53 agc Exp $	*/
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell, and Kazumasa Utashiro of Software Research
 * Associates, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machConst.h	8.1 (Berkeley) 6/11/93
 *
 * machConst.h --
 *
 *	Machine dependent constants.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: $Header: /sprite/src/kernel/mach/ds3100.md/RCS/machConst.h,
 *	v 9.2 89/10/21 15:55:22 jhh Exp $ SPRITE (DECWRL)
 * from: $Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAddrs.h,
 *	v 1.2 89/08/15 18:28:21 rab Exp $ SPRITE (DECWRL)
 * from: $Header: /sprite/src/kernel/vm/ds3100.md/RCS/vmPmaxConst.h,
 *	v 9.1 89/09/18 17:33:00 shirriff Exp $ SPRITE (DECWRL)
 */

#ifndef _MACHCONST
#define _MACHCONST

/*#define BETWEEN(x,a,b)	((unsigned)(x) >= (a) && (unsigned)(x) < (b))*/

#define MACH_IS_UNMAPPED(x) (((unsigned int)(x)) >> 30 == 2)	/* 10xx... */
#define MACH_IS_CACHED(x)   (((unsigned int)(x)) >> 29 == 4)	/* 100x... */
#define MACH_IS_UNCACHED(x) (((unsigned int)(x)) >> 29 == 5)	/* 101x... */

#define	MACH_IS_MAPPED(x) 1	/* XXX */

#define	MACH_IS_USPACE(x)   ((unsigned int)(x) < MIPS_KSEG0_START)

#define	MACH_KERNWORK_ADDR		0x800001c0
#define	MACH_MAXMEMSIZE_ADDR		(MACH_KERNWORK_ADDR + 0 * 4)
#define	MACH_BOOTSW_ADDR		(MACH_KERNWORK_ADDR + 1 * 4)
#define	MACH_BOOTDEV_ADDR		(MACH_KERNWORK_ADDR + 2 * 4)
#define	MACH_HOWTO_ADDR			(MACH_KERNWORK_ADDR + 3 * 4)
#define	MACH_BP_ADDR			(MACH_KERNWORK_ADDR + 4 * 4)
#define	MACH_MONARG_ADDR		(MACH_KERNWORK_ADDR + 5 * 4)

#endif /* _MACHCONST */
