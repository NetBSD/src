/*	$NetBSD: dicreg.h,v 1.1 2011/03/11 03:26:37 bsh Exp $ */

/*
 * Copyright (c) 2010, 2011 Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_MPCORE_DICREG_H
#define _ARM_MPCORE_DICREG_H

#include <sys/cdefs.h>

/*
 * CPU Interrupt Interface Registers (per core)
 */
#define	CII_CONTROL	0x00
#define	 CII_CONTROL_ENABLE	__BIT(0)

#define	CII_PRIMASK	0x04
#define	 CII_PRIMASK_SHIFT	4
#define	 CII_PRIMASK_MASK 	__BITS(CII_PRIMASK_SHIFT, 7)
#define	 CII_PRIMASK_MASKALL	(0 << CII_PRIMASK_SHIFT)
#define	 CII_PRIMASK_MASKNONE	(0x0f << CII_PRIMASK_SHIFT)
/* NOTE: interrupt level 15 is always masked. */


#define	CII_BINPOINT	0x08

#define	CII_INTACK	0x0c
#define	 CII_INTACK_CPUSRC_SHIFT	10
#define	 CII_INTACK_CPUSRC_MASK  	__BITS(CII_INTACK_CPUSRC_SHIFT, 12)
#define	 CII_INTACK_INTID_SHIFT  	0
#define	 CII_INTACK_INTID_MASK  	__BITS(CII_INTACK_INTID_SHIFT, 9)

#define	CII_EOI  	0x10

#define	CII_RUNPRI	0x14	/* Running Priority */

#define	CII_PENDPRI	0x18	/* highest Pending Interrupt */ 

/*
 * Interrupt Disctibutor (global)
 */
#define	DIC_CONTROL	0x000
#define	 DIC_CONTROL_ENABLE	__BIT(0)

#define	DIC_TYPE	0x004
#define	 DIC_TYPE_NCPUS_SHIFT	5
#define	 DIC_TYPE_NCPUS_MASK	__BITS(DIC_TYPE_NCPUS_SHIFT,7)
#define	 DIC_TYPE_NLINES_SHIFT	0
#define	 DIC_TYPE_NLINES_MASK	__BITS(DIC_TYPE_NLINES_SHIFT,4)

#define	DIC_ENSET(n)		(0x100 + (n)*sizeof (uint32_t))	/* n: 0..7 */
#define	DIC_ENCLEAR(n)		(0x180 + (n)*sizeof (uint32_t))
#define	DIC_PENDSET(n)  	(0x200 + (n)*sizeof (uint32_t))
#define	DIC_PENDCLEAR(n)	(0x280 + (n)*sizeof (uint32_t))
#define	DIC_ACTIVE(n)		(0x300 + (n)*sizeof (uint32_t))

#define	DIC_PRIORITY(n)  	(0x400 + (n)*sizeof (uint32_t))	/* n: 0..63 */
#define	DIC_TARGET(n)		(0x800 + (n)*sizeof (uint32_t))

#define	DIC_CONFIG(n)		(0xc00 + (n)*sizeof (uint32_t))	/* n: 0..15 */

#define	DIC_LINELEVEL(n)	(0xd00 + (n)*sizeof (uint32_t))

#define	DIC_SOFTINT	0xf00

#endif	/* _ARM_MPCORE_DICREG_H */
