/*	$NetBSD: param.h,v 1.53.18.2 2003/09/09 21:22:34 lukem Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: Utah Hdr: machparam.h 1.16 92/12/20
 *	from: @(#)param.h	8.1 (Berkeley) 6/10/93
 */

#ifndef	_MACHINE_PARAM_H_
#define	_MACHINE_PARAM_H_

/*
 * Machine dependent constants for both the
 * Sun3 and Sun3X series.
 */
#define	_MACHINE	sun3
#define	MACHINE		"sun3"

#define	PGSHIFT		13		/* LOG2(NBPG) */

#ifdef MSGBUFSIZE
#error "MSGBUFSIZE is not user-adjustable for this arch"
#endif
#define MSGBUFOFF	0
#define MSGBUFSIZE	(NBPG)

/* This is needed by ps (actually USPACE). */
#define	UPAGES		2		/* pages of u-area */

#if defined(_KERNEL) || defined(_STANDALONE)
#ifdef	_SUN3_
#include <arch/sun3/include/param3.h>
#endif	/* SUN3 */
#ifdef	_SUN3X_
#include <arch/sun3/include/param3x.h>
#endif	/* SUN3X */
#endif	/* _KERNEL */

#include <m68k/param.h>

/*
 * Minimum and maximum sizes of the kernel malloc arena in PAGE_SIZE-sized
 * logical pages.
 */
#define	NKMEMPAGES_MIN_DEFAULT	((3 * 1024 * 1024) >> PAGE_SHIFT)
#define	NKMEMPAGES_MAX_DEFAULT	((3 * 1024 * 1024) >> PAGE_SHIFT)

#if defined(_KERNEL) && !defined(_LOCORE)

#include <machine/intr.h>

extern void _delay __P((unsigned));
#define delay(us)	_delay((us)<<8)
#define	DELAY(n)	delay(n)

#undef	MAXPHYS
#define	MAXPHYS		(32 * 1024)	/* max raw I/O transfer size */

#endif	/* _KERNEL && !_LOCORE */
#endif	/* !_MACHINE_PARAM_H_ */
