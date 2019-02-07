/*	$NetBSD: param.h,v 1.32 2019/02/07 04:31:49 mrg Exp $	*/
/*      $OpenBSD: param.h,v 1.9 1997/04/30 09:54:15 niklas Exp $ */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 *	from: Utah Hdr: machparam.h 1.11 89/08/14
 *	from: @(#)param.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _ARC_PARAM_H_
#define _ARC_PARAM_H_

/*
 * Machine-dependent constants (VM, etc) common across MIPS cpus
 */
#define	_MACHINE	arc
#define	MACHINE		"arc"

#include <mips/mips_param.h>

/*
 * Machine dependent constants for ARC BIOS MIPS machines:
 *	Acer Labs PICA_61
 *	Deskstation rPC44
 *	Deskstation Tyne
 *	Etc
 */
#define	KERNBASE	0x80000000	/* start of kernel virtual */
#define	BTOPKERNBASE	((u_long)KERNBASE >> PGSHIFT)

#if defined(_LOCORE) && defined(notyet)
#define	UADDR		0xffffffffffffc000	/* address of u */
#else
#define	UADDR		0xffffc000	/* address of u */
#endif
#define USPACE		(UPAGES*NBPG)	/* size of u-area in bytes */
#define	UVPN		(UADDR>>PGSHIFT)/* virtual page number of u */
#define	KERNELSTACK	(UADDR+UPAGES*NBPG)	/* top of kernel stack */

/* bytes to disk blocks */
#define	btodb(x)	((x) >> DEV_BSHIFT)
#define dbtob(x)	((x) << DEV_BSHIFT)

#ifdef _KERNEL
#ifndef _LOCORE

extern int cpuspeed;
void delay(unsigned int n);

#define	DELAY(n)	delay(n)

#include <machine/intr.h>

#endif /* _LOCORE */
#endif /* _KERNEL */

#endif /* _ARC_PARAM_H_ */
