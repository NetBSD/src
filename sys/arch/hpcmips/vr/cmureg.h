/*	$NetBSD: cmureg.h,v 1.3.4.1 2001/10/01 12:39:18 fvdl Exp $	*/

/*-
 * Copyright (c) 1999 SATO Kazumi. All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */

/*
 *	CMU (CLock MASK UNIT) Registers.
 *		start 0x0B000060 (Vr4102-4111)
 *		start 0x0F000060 (Vr4122-4131)
 *		start 0x0A000004 (Vr4181)
 */
#define CMUNOMASK			0

#define	CMUCLKMASK		0x000	/* CMU Clock Mask Register */

/* vr4102-4121 */
#define		VR4102_CMUMSKPCIU	CMUNOMASK	/* no PCICLK */
#define		VR4102_CMUMSKFFIR	(1<<10)		/* 1 supply 48MHz to FIR */
#define		VR4102_CMUMSKSHSP	(1<<9)		/* 1 supply 18.432MHz to HSP */
#define		VR4102_CMUMSKSSIU	(1<<8)		/* 1 supply 18.432MHz to SIU */
#define		VR4102_CMUMSKDSIU	(1<<5)		/* 1 supply Tclock to DSIU */
#define		VR4102_CMUMSKCSI	CMUNOMASK	/* no CSI clock */
#define		VR4102_CMUMSKFIR	(1<<4)		/* 1 supply Tclock to FIR */
#define		VR4102_CMUMSKKIU	(1<<3)		/* 1 supply Tclock to KIU */
#define		VR4102_CMUMSKAIU	(1<<2)		/* 1 supply Tclock to AIU */
#define		VR4102_CMUMSKSIU	(1<<1)		/* 1 supply Tclock to SIU */
#define		VR4102_CMUMSKPIU	(1)		/* 1 supply Tclock to PIU */

/* vr4122-4131 */
#define		VR4122_CMUMSKPCIU	((1<<13)|(1<<7))	/* 1 supply PCICLK */
#define		VR4122_CMUMSKSCSI	(1<<12)		/* 1 supply CSI 18.432MHz clock */
#define		VR4122_CMUMSKDSIU	(1<<11)		/* 1 supply DSIU 18.432MHz clock */
#define		VR4122_CMUMSKFFIR	(1<<10)		/* 1 supply 48MHz to FIR */
#define		VR4122_CMUMSKSHSP	CMUNOMASK	/* no HSP */
#define		VR4122_CMUMSKSSIU	(1<<8)		/* 1 supply 18.432MHz to SIU */
#define		VR4122_CMUMSKCSI	(1<<6)		/* 1 supply Tclock to CSI */
#define		VR4122_CMUMSKFIR	(1<<4)		/* 1 supply Tclock to FIR */
#define		VR4122_CMUMSKKIU	CMUNOMASK	/* no KIU */
#define		VR4122_CMUMSKAIU	CMUNOMASK	/* no AIU */
#define		VR4122_CMUMSKSIU	(1<<1)		/* 1 supply Tclock to SIU */
#define		VR4122_CMUMSKPIU	CMUNOMASK	/* no PIU */

/* vr4181 */
#define		VR4181_CMUMSKPCIU	CMUNOMASK	/* no PCICLK */
#define		VR4181_CMUMSKHSP	CMUNOMASK	/* no HSP */
#define		VR4181_CMUMSKDSIU	CMUNOMASK	/* no DSIU */
#define		VR4181_CMUMSKCSI	(1<<6)		/* 1 supply PCLK to CSI */
#define		VR4181_CMUMSKFIR	CMUNOMASK	/* no FIR */
#define		VR4181_CMUMSKKIU	CMUNOMASK	/* no KIU */
#define		VR4181_CMUMSKAIU	(1<<5)		/* 1 supply PLCK to AIU */
#define		VR4181_CMUMSKPIU	(1<<4)		/* 1 supply PLCK to PIU */
#define		VR4181_CMUMSKADU	(1<<3)		/* 1 supply PLCK to ADU */
#define		VR4181_CMUMSKSSIU	(1<<2)		/* 1 supply 18.432MHz to SIU */
#define		VR4181_CMUMSKSADU	(1<<1)		/* 1 supply 18.432MHz to ADU */

#if defined SINGLE_VRIP_BASE

#ifdef VRGROUP_4102_4121
#define CMUMASK_PIU	VR4102_CMUMSKPIU
#define CMUMASK_SIU	(VR4102_CMUMSKSIU|VR4102_CMUMSKSSIU)
#define CMUMASK_AIU	VR4102_CMUMSKAIU
#define CMUMASK_KIU	VR4102_CMUMSKKIU
#define CMUMASK_FIR	(VR4102_CMUMSKFIR|VR4102_CMUMSKFFIR)
#define CMUMASK_DSIU	VR4102_CMUMSKDSIU
#define CMUMASK_HSP	VR4102_CMUMSKHSP
#define CMUMASK_CSI	VR4102_CMUMSKCSI
#define CMUMASK_PCIU	VR4102_CMUMSKPCIU
#endif /* VRGROUP_4102_4121 */

#ifdef VRGROUP_4122_4131
#define CMUMASK_PIU	VR4122_CMUMSKPIU
#define CMUMASK_SIU	(VR4122_CMUMSKSIU|VR4122_CMUMSKSSIU)
#define CMUMASK_AIU	VR4122_CMUMSKAIU
#define CMUMASK_KIU	VR4122_CMUMSKKIU
#define CMUMASK_FIR	(VR4122_CMUMSKFIR|VR4122_CMUMSKFFIR)
#define CMUMASK_DSIU	VR4122_CMUMSKDSIU
#define CMUMASK_HSP	VR4122_CMUMSKHSP
#define CMUMASK_CSI	(VR4122_CMUMSKSCSI|VR4122_CMUMSKCSI)
#define CMUMASK_PCIU	VR4122_CMUMSKPCIU
#endif /* VRGROUP_4122_4131 */

#ifdef VRGROUP_4181
#define CMUMASK_PIU	VR4181_CMUMSKPIU
#define CMUMASK_SIU	VR4181_CMUMSKSSIU
#define CMUMASK_AIU	VR4181_CMUMSKAIU
#define CMUMASK_KIU	VR4181_CMUMSKKIU
#define CMUMASK_FIR	VR4181_CMUMSKFIR
#define CMUMASK_DSIU	VR4181_CMUMSKDSIU
#define CMUMASK_HSP	VR4181_CMUMSKHSP
#define CMUMASK_CSI	VR4181_CMUMSKCSI
#define CMUMASK_PCIU	VR4181_CMUMSKPCIU
#endif /* VRGROUP_4181 */

#endif /* SINGLE_VRIP_BASE */
/* END cmureg.h */
