/*	$NetBSD: mmureg.h,v 1.7 2002/02/17 20:55:52 uch Exp $	*/

/*-
 * Copyright (C) 1999 SAITOH Masanobu.  All rights reserved.
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
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_MMUREG_H__
#define _SH3_MMUREG_H__

#ifdef _KERNEL
/*
 * MMU
 */

#if !defined(SH4)

/* SH3 definitions */

#define SHREG_PTEH	(*(volatile unsigned long *)	0xFFFFFFF0)
#define SHREG_PTEL	(*(volatile unsigned long *)	0xFFFFFFF4)
#define SHREG_TTB	(*(volatile unsigned long *)	0xFFFFFFF8)
#define SHREG_TEA	(*(volatile unsigned long *)	0xFFFFFFFC)
#define SHREG_MMUCR	(*(volatile unsigned long *)	0xFFFFFFE0)

#else /* !SH4 */

/* SH4 definitions */

#define SHREG_PTEH	(*(volatile unsigned long *)	0xff000000)
#define SHREG_PTEL	(*(volatile unsigned long *)	0xff000004)
#define SHREG_PTEA	(*(volatile unsigned long *)	0xff000034)
#define SHREG_TTB	(*(volatile unsigned long *)	0xff000008)
#define SHREG_TEA	(*(volatile unsigned long *)	0xff00000c)
#define SHREG_MMUCR	(*(volatile unsigned long *)	0xff000010)

#endif /* !SH4 */

#if !defined(SH4)

/* SH3 definitions */
#define MMUCR_AT	0x0001	/* address traslation enable */
#define MMUCR_IX	0x0002	/* index mode */
#define MMUCR_TF	0x0004	/* TLB flush */
#define MMUCR_SV	0x0100	/* single virtual space mode */

#else /* !SH4 */

/* SH4 definitions */
#define MMUCR_AT	0x0001	/* address traslation enable */
#define MMUCR_TI	0x0004	/* TLB Invaliate */
#define MMUCR_SV	0x0100	/* single virtual space mode */
#define MMUCR_SQMD	0x0200  /* Store Queue mode */

#define MMUCR_VALIDBITS 0xfcfcff05	/* XXX */

/* alias */
#define MMUCR_TF	MMUCR_TI

#endif /* !SH4 */

#endif /* _KERNEL */
#endif /* !_SH3_MMUREG_H__ */
