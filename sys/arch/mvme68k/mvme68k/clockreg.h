/*	$NetBSD: clockreg.h,v 1.4 2000/03/18 22:33:06 scw Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)clockreg.h	8.1 (Berkeley) 6/11/93
 */
#ifndef _MVME68K_CLOCKREG_H
#define _MVME68K_CLOCKREG_H

/*
 * Mostek MK48T02 clock register offsets
 */
#define MK48TREG_CSR	0	/* control register */
#define MK48TREG_SEC	1	/* seconds (0..59; BCD) */
#define MK48TREG_MIN	2	/* minutes (0..59; BCD) */
#define MK48TREG_HOUR	3	/* hour (0..23; BCD) */
#define MK48TREG_WDAY	4	/* weekday (1..7) */
#define MK48TREG_MDAY	5	/* day in month (1..31; BCD) */
#define MK48TREG_MONTH	6	/* month (1..12; BCD) */
#define MK48TREG_YEAR	7	/* year (0..99; BCD) */
#define MK48T_REGSIZE	8

/* bits in MK48TREG_CSR */
#define	CLK_WRITE	0x80		/* want to write */
#define	CLK_READ	0x40		/* want to read (freeze clock) */

/*
 * Sun chose the year `68' as their base count, so that
 * cl_year==0 means 1968.
 */
#define	YEAR0	68

/*
 * interrupt level for clock
 */
#define CLOCK_LEVEL 5

#endif /* _MVME68K_CLOCKREG_H */
