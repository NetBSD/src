/*	$NetBSD: i80200reg.h,v 1.1.2.3 2002/02/28 04:07:44 nathanw Exp $	*/

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

#define	INTCTL_FM	0x00000001	/* external FIQ# enable */
#define	INTCTL_IM	0x00000002	/* external IRQ# enable */
#define	INTCTL_PM	0x00000004	/* PMU interrupt enable */
#define	INTCTL_BM	0x00000008	/* BCU interrupt enable */

#define	INTSRC_PI	0x10000000	/* PMU interrupt */
#define	INTSRC_BM	0x20000000	/* BCU interrupt */
#define	INTSRC_II	0x40000000	/* external IRQ# */
#define	INTSRC_FI	0x80000000	/* external FIQ# */

#define	INTSTR_PS	0x00000001	/* PMU 0 = IRQ, 1 = FIQ */
#define	INTSTR_BS	0x00000002	/* BCU 0 = IRQ, 1 = FIQ */

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

#define	BCUCTL_SR	0x00000001	/* single bit error report enable */
#define	BCUCTL_SC	0x00000004	/* single bit correct enable */
#define	BCUCTL_EE	0x00000008	/* ECC enable */
#define	BCUCTL_E0	0x10000000	/* ELOG0 valid */
#define	BCUCTL_E1	0x20000000	/* ELOG1 valid */
#define	BCUCTL_EV	0x40000000	/* error overflow */
#define	BCUCTL_TP	0x80000000	/* transactions pending */

#define	BCUMOD_AF	0x00000001	/* aligned fetch */

#define	ELOGx_SYN_MASK	0x000000ff	/* ECC syndrome */
#define	ELOGx_ET_MASK	0x60000000	/* error type */
#define	ELOGx_ET_SB	0x00000000	/* single-bit */
#define	ELOGx_ET_MB	0x20000000	/* multi-bit */
#define	ELOGx_ET_BA	0x40000000	/* bus abort */
#define	ELOGx_RW	0x80000000	/* direction 0 = read 1 = write */

/*
 * Performance Monitoring Unit		(CP14)
 *
 *	CP14.0		Performance Monitor Control Register
 *	CP14.1		Clock Counter
 *	CP14.2		Performance Counter Register 0
 *	CP14.3		Performance Counter Register 1
 */

#define	PMNC_E		0x00000001	/* enable counters */
#define	PMNC_P		0x00000002	/* reset both PMNs to 0 */
#define	PMNC_C		0x00000004	/* clock counter reset */
#define	PMNC_D		0x00000008	/* clock counter / 64 */
#define	PMNC_PMN0_IE	0x00000010	/* enable PMN0 interrupt */
#define	PMNC_PMN1_IE	0x00000020	/* enable PMN1 interrupt */
#define	PMNC_CC_IE	0x00000040	/* enable clock counter interrupt */
#define	PMNC_PMN0_IF	0x00000100	/* PMN0 overflow/interrupt */
#define	PMNC_PMN1_IF	0x00000200	/* PMN1 overflow/interrupt */
#define	PMNC_CC_IF	0x00000400	/* clock counter overflow/interrupt */
#define	PMNC_EVCNT0_MASK 0x000ff000	/* event to count for PMN0 */
#define	PMNC_EVCNT0_SHIFT 12
#define	PMNC_EVCNT1_MASK 0x0ff00000	/* event to count for PMN1 */
#define	PMNC_EVCNT1_SHIFT 20

#endif /* _ARM_XSCALE_I80200REG_H_ */
