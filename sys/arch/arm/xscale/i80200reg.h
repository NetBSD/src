/*	$NetBSD: i80200reg.h,v 1.1 2001/12/01 05:46:19 thorpej Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_XSCALE_I80200REG_H_ 
#define _ARM_XSCALE_I80200REG_H_ 

/*
 * Register definitions for the Intel 80200 XScale processor.
 */

/*
 * Interrupt Controller Unit		(CP13)
 *
 *	CP13.0		Interrupt Control
 *	CP13.1		Interrupt Source
 *	CP13.2		Interrupt Steer
 */

#define	INTCTL_FM	(1U << 0)	/* external FIQ# enable */
#define	INTCTL_IM	(1U << 1)	/* external IRQ# enable */
#define	INTCTL_PM	(1U << 2)	/* PMU interrupt enable */
#define	INTCTL_BM	(1U << 3)	/* BCU interrupt enable */

#define	INTSRC_PI	(1U << 28)	/* PMU interrupt */
#define	INTSRC_BM	(1U << 29)	/* BCU interrupt */
#define	INTSRC_II	(1U << 30)	/* external IRQ# */
#define	INTSRC_FI	(1U << 31)	/* external FIQ# */

#define	INTSTR_PS	(1U << 0)	/* PMU 0 = IRQ, 1 = FIQ */
#define	INTSTR_BS	(1U << 1)	/* BCU 0 = IRQ, 1 = FIQ */

/*
 * Bus Controller Unit			(CP13)
 *
 *	CP13.0.1	BCU Control
 *	CP13.1.1	BCUMOD
 *	CP13.4.1	ELOG0 (ECC error log)
 *	CP13.5.1	ELOG1
 *	CP13.6.1	ECAR0 (ECC error address)
 *	CP13.7.1	ECAR1
 *	CP13.8.1	ECTST (ECC test)
 */

#define	BCUCTL_SR	(1U << 0)	/* single bit error report enable */
#define	BCUCTL_SC	(1U << 2)	/* single bit correct enable */
#define	BCUCTL_EE	(1U << 3)	/* ECC enable */
#define	BCUCTL_E0	(1U << 28)	/* ELOG0 valid */
#define	BCUCTL_E1	(1U << 29)	/* ELOG1 valid */
#define	BCUCTL_EV	(1U << 30)	/* error overflow */
#define	BCUCTL_TP	(1U << 31)	/* transactions pending */

#define	BCUMOD_AF	(1U << 0)	/* aligned fetch */

#define	ELOGx_SYN(x)	((x) & 0xff)	/* ECC syndrome */
#define	ELOGx_ET(x)	(((x) >> 29) & 3)/* error type */
#define	ELOGx_ET_SB	0		/* single-bit */
#define	ELOGx_ET_MB	1		/* multi-bit */
#define	ELOGx_ET_BA	2		/* bus abort */
#define	ELOGx_RW	(1U << 31)	/* direction 0 = read 1 = write */

/*
 * Performance Monitoring Unit		(CP14)
 *
 *	CP14.0		Performance Monitor Control Register
 *	CP14.1		Clock Counter
 *	CP14.2		Performance Counter Register 0
 *	CP14.3		Performance Counter Register 1
 */

#define	PMNC_E		(1U << 0)	/* enable counters */
#define	PMNC_P		(1U << 1)	/* reset both PMNs to 0 */
#define	PMNC_C		(1U << 2)	/* clock counter reset */
#define	PMNC_D		(1U << 3)	/* clock counter / 64 */
#define	PMNC_PMN0_IE	(1U << 4)	/* enable PMN0 interrupt */
#define	PMNC_PMN1_IE	(1U << 5)	/* enable PMN1 interrupt */
#define	PMNC_CC_IE	(1U << 6)	/* enable clock counter interrupt */
#define	PMNC_PMN0_IF	(1U << 8)	/* PMN0 overflow/interrupt */
#define	PMNC_PMN1_IF	(1U << 9)	/* PMN1 overflow/interrupt */
#define	PMNC_CC_IF	(1U << 10)	/* clock counter overflow/interrupt */
#define	PMNC_EVCNT0(x)	((x) << 12)	/* event to count for PMN0 */
#define	PMNC_EVCNT1(x)	((x) << 20)	/* event to count for PMN1 */

#endif /* _ARM_XSCALE_I80200REG_H_ */
