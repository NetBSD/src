/*	$NetBSD: trap.h,v 1.3 2002/04/28 17:10:37 uch Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)trap.h	5.4 (Berkeley) 5/9/91
 */

/*
 * Trap type values
 * also known in trap.c for name strings
 */

/*
 *   SH3 version
 *
 *   T.Horiuchi Brains Corp. 05/22/1998
 */
#define	T_POWERON		0x000	/* Power on */
#define	T_RESET			0x020	/* Reset */
#define	T_TLBMISSR		0x040	/* TLB miss(Read) */
#define	T_TLBINVALIDR		0x040	/* Invalid TLB(Read) */
#define	T_TLBMISSW		0x060	/* TLB miss(Write) */
#define	T_TLBINVALIDW		0x060	/* Invalid TLB(Write) */
#define	T_INITPAGEWR		0x080	/* Initial Page write */
#define	T_TLBPRIVR		0x0a0	/* TLB privilege violation(Read) */
#define	T_TLBPRIVW		0x0c0	/* TLB privilege violation(Write) */
#define	T_ADDRESSERRR		0x0e0	/* Address Error(Read) */
#define	T_ADDRESSERRW		0x100	/* Address Error(Write) */
#define	T_TRAP			0x160	/* trapa instruction */
#define	T_INVALIDISN		0x180	/* Invalid instruction */
#define	T_INVALIDSLOT		0x1a0	/* Invalid slot instruction */
#define	T_USERBREAK		0x1e0	/* User break */
#define	T_NMI			0x1c0	/* NMI */
#define	T_EXTINT0		0x200	/* External Interrupt IRL=0 */
#define	T_EXTINT1		0x220	/* External Interrupt IRL=1 */
#define	T_EXTINT2		0x240	/* External Interrupt IRL=2 */
#define	T_EXTINT3		0x260	/* External Interrupt IRL=3 */
#define	T_EXTINT4		0x280	/* External Interrupt IRL=4 */
#define	T_EXTINT5		0x2a0	/* External Interrupt IRL=5 */
#define	T_EXTINT6		0x2c0	/* External Interrupt IRL=6 */
#define	T_EXTINT7		0x2e0	/* External Interrupt IRL=7 */
#define	T_EXTINT8		0x300	/* External Interrupt IRL=8 */
#define	T_EXTINT9		0x320	/* External Interrupt IRL=9 */
#define	T_EXTINT10		0x340	/* External Interrupt IRL=10 */
#define	T_EXTINT11		0x360	/* External Interrupt IRL=11 */
#define	T_EXTINT12		0x380	/* External Interrupt IRL=12 */
#define	T_EXTINT13		0x3a0	/* External Interrupt IRL=13 */
#define	T_EXTINT14		0x3c0	/* External Interrupt IRL=14 */
#define	T_PIN_TMU0		0x400	/* Peripheral Interrupt(TMU0) */
#define	T_PIN_TMU1		0x420	/* Peripheral Interrupt(TMU1) */
#define	T_PIN_TMU2_TUNI2	0x440	/* Peripheral Interrupt(TMU2_TUNI2) */
#define	T_PIN_TMU2_TICPI2	0x460	/* Peripheral Interrupt(TMU2_TICPI2) */
#define	T_PIN_RTC_ATI		0x480	/* Peripheral Interrupt(RTC_ATI) */
#define	T_PIN_RTC_PRI		0x4a0	/* Peripheral Interrupt(RTC_PRI) */
#define	T_PIN_RTC_CUI		0x4c0	/* Peripheral Interrupt(RTC_CUI) */
#define	T_PIN_SCI_ERI		0x4e0	/* Peripheral Interrupt(SCI_ERI) */
#define	T_PIN_SCI_RXI		0x500	/* Peripheral Interrupt(SCI_RXI) */
#define	T_PIN_SCI_TXI		0x520	/* Peripheral Interrupt(SCI_TXI) */
#define	T_PIN_SCI_TEI		0x540	/* Peripheral Interrupt(SCI_TEI) */
#define	T_PIN_WDT		0x560	/* Peripheral Interrupt(WDT) */
#define	T_PIN_REF_RCMI		0x580	/* Peripheral Interrupt(REF_RCMI) */
#define	T_PIN_REF_ROVI		0x5a0	/* Peripheral Interrupt(REF_ROVI) */

#define	T_USER			0x001	/* Trap in user mode */
