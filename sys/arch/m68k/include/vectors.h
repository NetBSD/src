/*	$NetBSD: vectors.h,v 1.4 2024/01/14 00:00:15 thorpej Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _M68K_VECTORS_H_
#define	_M68K_VECTORS_H_

/*
 * Vector numbers (*4 for byte offset into table).
 *
 * VECI_RIISP and VECI_RIPC are fetched using Supervisor Program
 * space.  The rest, Supervisor Data space.
 *
 * VECI_PMMU_ILLOP and VECI_PMMU_ACCESS are defined for the 68020, but
 * not used on the 68030.
 *
 * VECI_CPV and VECI_PMMU_CONF are defined for the 68020 and 68030, but
 * not used on the 68040 or 68060.
 *
 * VECI_UNIMP_FP_DATA, VECI_UNIMP_EA and VECI_UNIMP_II are defined for
 * the 68060 and reserved on all other processors.
 */
#define	VECI_RIISP		0	/* Reset Initial Interrupt SP */
#define	VECI_RIPC		1	/* Reset Initial PC */
#define	VECI_BUSERR		2	/* Bus Error */
#define	VECI_ADDRERR		3	/* Address Error */
#define	VECI_ILLINST		4	/* Illegal Instruction */
#define	VECI_ZERODIV		5	/* Zero Divide */
#define	VECI_CHK		6	/* CHK, CHK2 instruction */
#define	VECI_TRAPcc		7	/* cpTRAPcc, TRAPcc, TRAPV */
#define	VECI_PRIV		8	/* Privilege Violation */
#define	VECI_TRACE		9	/* Trace */
#define	VECI_LINE1010		10	/* Line 1010 Emulator */
#define	VECI_LINE1111		11	/* Line 1111 Emulator */
#define	VECI_rsvd12		12	/* unassigned, reserved */
#define	VECI_CPV		13	/* Coprocessor Prototol Violation */
#define	VECI_FORMATERR		14	/* Format Error */
#define	VECI_UNINT_INTR		15	/* Uninitialized Interrupt */
#define	VECI_rsvd16		16	/* unassigned, reserved */
#define	VECI_rsvd17		17	/* unassigned, reserved */
#define	VECI_rsvd18		18	/* unassigned, reserved */
#define	VECI_rsvd19		19	/* unassigned, reserved */
#define	VECI_rsvd20		20	/* unassigned, reserved */
#define	VECI_rsvd21		21	/* unassigned, reserved */
#define	VECI_rsvd22		22	/* unassigned, reserved */
#define	VECI_rsvd23		23	/* unassigned, reserved */
#define	VECI_INTRAV0		24	/* Spurious Interrupt */
#define	VECI_SPURIOUS_INTR	VECI_INTRAV0
#define	VECI_INTRAV1		25	/* Level 1 Interrupt Autovector */
#define	VECI_INTRAV2		26	/* Level 2 Interrupt Autovector */
#define	VECI_INTRAV3		27	/* Level 3 Interrupt Autovector */
#define	VECI_INTRAV4		28	/* Level 4 Interrupt Autovector */
#define	VECI_INTRAV5		29	/* Level 5 Interrupt Autovector */
#define	VECI_INTRAV6		30	/* Level 6 Interrupt Autovector */
#define	VECI_INTRAV7		31	/* Level 7 Interrupt Autovector */
#define	VECI_TRAP0		32	/* Trap #0 instruction */
#define	VECI_TRAP1		33	/* Trap #1 instruction */
#define	VECI_TRAP2		34	/* Trap #2 instruction */
#define	VECI_TRAP3		35	/* Trap #3 instruction */
#define	VECI_TRAP4		36	/* Trap #4 instruction */
#define	VECI_TRAP5		37	/* Trap #5 instruction */
#define	VECI_TRAP6		38	/* Trap #6 instruction */
#define	VECI_TRAP7		39	/* Trap #7 instruction */
#define	VECI_TRAP8		40	/* Trap #8 instruction */
#define	VECI_TRAP9		41	/* Trap #9 instruction */
#define	VECI_TRAP10		42	/* Trap #10 instruction */
#define	VECI_TRAP11		43	/* Trap #11 instruction */
#define	VECI_TRAP12		44	/* Trap #12 instruction */
#define	VECI_TRAP13		45	/* Trap #13 instruction */
#define	VECI_TRAP14		46	/* Trap #14 instruction */
#define	VECI_TRAP15		47	/* Trap #15 instruction */
#define	VECI_FP_BSUN		48	/* FPCP Branch or Set on Unordered */
#define	VECI_FP_INEX		49	/* FPCP Inexact Result */
#define	VECI_FP_DZ		50	/* FPCP Divide by Zero */
#define	VECI_FP_UNFL		51	/* FPCP Underflow */
#define	VECI_FP_OPERR		52	/* FPCP Operand Error */
#define	VECI_FP_OVFL		53	/* FPCP Overflow */
#define	VECI_FP_SNAN		54	/* FPCP Signalling NaN */
#define	VECI_UNIMP_FP_DATA	55	/* FP Unimplemented Data Type */
#define	VECI_PMMU_CONF		56	/* PMMU Configuration */
#define	VECI_PMMU_ILLOP		57	/* PMMU Illegal Operation */
#define	VECI_PMMU_ACCESS	58	/* PMMU Access Level Violation */
#define	VECI_rsvd59		59	/* unassigned, reserved */
#define	VECI_UNIMP_EA		60	/* Unimplemented Effective Address */
#define	VECI_UNIMP_II		61	/* Umimplemented Integer Instruction */
#define	VECI_rsvd62		62	/* unassigned, reserved */
#define	VECI_rsvd63		63	/* unassigned, reserved */
#define	VECI_USRVEC_START	64	/* User defined vectors (192) */

#define	NVECTORS		256
#define	NAUTOVECTORS		8
#define	NUSERVECTORS		(NVECTORS - VECI_USRVEC_START)

#define	VECI_INTRAV(ipl)	((ipl) + VECI_SPURIOUS_INTR)
#define	VECI_TRAP(x)		((x) + VECI_TRAP0)

#define	VECI_TO_VECO(x)		((x) << 2)
#define	VECO_TO_VECI(x)		((unsigned int)(x) >> 2)

#ifdef _KERNEL

extern void *vectab[NVECTORS];
extern void **saved_vbr;

void	vec_init(void);
void	vec_reset(void);
void	*vec_get_entry(int);
void	vec_set_entry(int, void *);

#endif /* _KERNEL */

#endif /* _M68K_VECTORS_H_ */
