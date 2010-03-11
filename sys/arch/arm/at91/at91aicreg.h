/*	$NetBSD: at91aicreg.h,v 1.1.20.2 2010/03/11 15:02:04 yamt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _AT91AICREG_H_
#define _AT91AICREG_H_

#define	AT91AIC_BASE	0xFFFFF000UL	/* AIC BUS address		*/

#define	AIC_NIRQ	32UL		/* number of vectors		*/
#define	AIC_VEC_VALID(n)	((n) >= 0 && (n) < AIC_NIRQ)

#define	AIC_SMR(vec)	(0x000UL+(vec)*4UL)/* Source Mode Registers	*/
#define	AIC_SVR(vec)	(0x080UL+(vec)*4UL)/* Source Vectors Regs	*/
#define	AIC_IVR		0x100UL		/* 100: Interrupt Vector Reg	*/
#define	AIC_FVR		0x104UL		/* 104: Fast Interrupt Vect Reg	*/
#define	AIC_ISR		0x108UL		/* 108: Interrupt Status Reg	*/
#define	AIC_IPR		0x10CUL		/* 10c: Interrupt Pending Reg	*/
#define	AIC_IMR		0x110UL		/* 110: Interrupt Mask Reg	*/
#define	AIC_CISR	0x114UL		/* 114: Core interrupt Stat Reg	*/
#define	AIC_IECR	0x120UL		/* 120: Interrupt Enable Cmd reg*/
#define	AIC_IDCR	0x124UL		/* 124: Interrupt Dis. Cmd Reg	*/
#define	AIC_ICCR	0x128UL		/* 128: Interrupt Clear Cmd Reg	*/
#define	AIC_ISCR	0x12CUL		/* 12c: Interrupt Set Cmd Reg	*/
#define	AIC_EOICR	0x130UL		/* 130: End of Interrupt Vec Reg*/
#define	AIC_SPU		0x134UL		/* 134: Spurious Int. Vec Reg	*/
#define	AIC_DCR		0x138UL		/* 138: Debug Control Reg	*/
#define	AIC_FFER	0x140UL		/* 140: Fast Forcing Enable	*/
#define	AIC_FFDR	0x144UL		/* 144: Fast Forcing Disable	*/
#define	AIC_FFSR	0x148UL		/* 148: Fast Forcing Status	*/

/* Source Mode Register bits: */
#define	AIC_SMR_SRCTYPE		0x60
#define	AIC_SMR_SRCTYPE_LVL_LO	0x00
#define	AIC_SMR_SRCTYPE_FALLING	0x20
#define	AIC_SMR_SRCTYPE_LEVEL	0x00
#define	AIC_SMR_SRCTYPE_EDGE	0x20
#define	AIC_SMR_SRCTYPE_LVL_HI	0x40
#define	AIC_SMR_SRCTYPE_RISING	0x60
#define	AIC_SMR_PRIOR		0x7
#define	AIC_SMR_PRIOR_SHIFT	0

/* Debug Control Register: */
#define	AIC_DEBUG_GMSK		0x2	/* 1= mask all interrupts (?)	*/
#define	AIC_DEBUG_PROT		0x1	/* 1 = protection mode enabled	*/

#endif	// _AT91AICREG_H_

