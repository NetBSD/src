/*	$NetBSD: tmureg.h,v 1.1 2002/07/05 13:31:55 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#ifndef _SH5_TMUREG_H
#define _SH5_TMUREG_H

#define	TMU_REG_TOCR		0x00	/* Timer output control register */
#define	TMU_REG_TSTR		0x04	/* Timer start register */
#define	TMU_REG_TCOR(t)		(0x08+((t)*0x0c)) /* Timer constant registers */
#define	TMU_REG_TCNT(t)		(0x0c+((t)*0x0c)) /* Timer counter registers */
#define	TMU_REG_TCR(t)		(0x10+((t)*0x0c)) /* Timer control registers */
#define	TMU_REG_TCPR2		0x2c	/* Input capture register */

#define	TMU_REG_SIZE		0x30
#define	TMU_NTIMERS		3

/*
 * Bit definitions for TMU_REG_TOCR
 */
#define	TMU_TOCR_TCOE		0x01	/* Timer output control */

/*
 * Bit definitions for TMU_REG_TSTR
 */
#define	TMU_TSTR(t)		(1<<(t)) /* Counter `t' start */

/*
 * Bit definitions for TMU_REG_TCR
 * (For all timers)
 */
#define	TMU_TCR_TPSC_MASK	0x0007	/* Timer prescaler mask */
#define	TMU_TCR_TPSC_PDIV4	0	/* Peripheral clock / 4 */
#define	TMU_TCR_TPSC_PDIV16	1	/* Peripheral clock / 16 */
#define	TMU_TCR_TPSC_PDIV64	2	/* Peripheral clock / 64 */
#define	TMU_TCR_TPSC_PDIV256	3	/* Peripheral clock / 256 */
#define	TMU_TCR_TPSC_PDIV1024	4	/* Peripheral clock / 1024 */
#define	TMU_TCR_TPSC_RTC_OUT	6	/* On-chip RTC output clock */
#define	TMU_TCR_TPSC_EXT	7	/* External clock source */
#define	TMU_TCR_CKEG_MASK	0x0018	/* External clock edge select */
#define	TMU_TCR_CKEG_RISING	0x0000	/* Clock tick on rising edge */
#define	TMU_TCR_CKEG_FALLING	0x0008	/* Clock tick on falling edge */
#define	TMU_TCR_CKEG_BOTH	0x0010	/* Clock tick on both edges */
#define	TMU_TCR_UNIE		0x0020	/* Underflow interrupt enable */
#define	TMU_TCR_UNF		0x0100	/* Underflow occurred */

/*
 * Additional bit definitions for TMU_REG_TCR
 * (For timer 2)
 */
#define	TMU_TCR_ICPE_MASK	0x00c0	/* Input capture control */
#define	TMU_TCR_ICPE_OFF	0x0000	/* Input capture disabled */
#define	TMU_TCR_ICPE_ENOINT	0x0080	/* Input capture enabled, no irq */
#define	TMU_TCR_ICPE_EINT	0x00c0	/* Input capture enabled, irq enabled */
#define	TMU_TCR_ICPF		0x0100	/* Input capture occurred */

#endif /* _SH5_TMUREG_H */
