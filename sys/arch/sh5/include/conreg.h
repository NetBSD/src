/*	$NetBSD: conreg.h,v 1.1 2002/07/05 13:31:57 scw Exp $	*/

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

/*
 * SH-5 Control and Configuration Registers
 */

#ifndef _SH5_CONREG_H
#define _SH5_CONREG_H

/*
 * Bit definitions for Status Register (sr, ssr, pssr)
 */
#define	SH5_CONREG_SR_S			(1<<1)   /* Saturation Control */
#define	SH5_CONREG_SR_IMASK_MASK	0xf	 /* Interrupt Mask */
#define	SH5_CONREG_SR_IMASK_SHIFT	4	 /* Shift for interrupt mask */
#define	SH5_CONREG_SR_IMASK_ALL		0xf0	 /* Mask all interrupts */
#define	 SH5_CONREG_SR_IMASK_IPL0	0x00	 /* IPLs 0 - 15 */
#define	 SH5_CONREG_SR_IMASK_IPL1	0x10
#define	 SH5_CONREG_SR_IMASK_IPL2	0x20
#define	 SH5_CONREG_SR_IMASK_IPL3	0x30
#define	 SH5_CONREG_SR_IMASK_IPL4	0x40
#define	 SH5_CONREG_SR_IMASK_IPL5	0x50
#define	 SH5_CONREG_SR_IMASK_IPL6	0x60
#define	 SH5_CONREG_SR_IMASK_IPL7	0x70
#define	 SH5_CONREG_SR_IMASK_IPL8	0x80
#define	 SH5_CONREG_SR_IMASK_IPL9	0x90
#define	 SH5_CONREG_SR_IMASK_IPL10	0xa0
#define	 SH5_CONREG_SR_IMASK_IPL11	0xb0
#define	 SH5_CONREG_SR_IMASK_IPL12	0xc0
#define	 SH5_CONREG_SR_IMASK_IPL13	0xd0
#define	 SH5_CONREG_SR_IMASK_IPL14	0xe0
#define	 SH5_CONREG_SR_IMASK_IPL15	0xf0
#define	SH5_CONREG_SR_Q			(1<<8)   /* State for divide step */
#define	SH5_CONREG_SR_M			(1<<9)   /* Floating point precision */
#define	SH5_CONREG_SR_CD		(1<<11)  /* Clock tick ctr disable */
#define	SH5_CONREG_SR_PR		(1<<12)  /* Floating point precision */
#define	SH5_CONREG_SR_SZ		(1<<13)  /* Floating point tran. size */
#define	SH5_CONREG_SR_FR		(1<<14)  /* Floating point reg. bank */
#define	SH5_CONREG_SR_FD		(1<<15)  /* Floating point disable */
#define	SH5_CONREG_SR_FD_SHIFT		15
#define	SH5_CONREG_SR_ASID_MASK		0xff
#define	SH5_CONREG_SR_ASID_SHIFT	16	 /* ASID Shift */
#define	SH5_CONREG_SR_WATCH		(1<<26)  /* Watchpoint enable flag */
#define	SH5_CONREG_SR_STEP		(1<<27)  /* Single-step enable flag */
#define	SH5_CONREG_SR_BL		(1<<28)  /* Block ALL exceptions */
#define	SH5_CONREG_SR_BL_SHIFT		28
#define	SH5_CONREG_SR_MD		(1<<30)  /* User/Priv Mode flag */
#define	SH5_CONREG_SR_MD_SHIFT		30
#define	SH5_CONREG_SR_MMU		(1<<31)  /* MMU Enable flag */
#define	SH5_CONREG_SR_MMU_SHIFT		31

#define	SH5_CONREG_SR_INIT		(SH5_CONREG_SR_BL | SH5_CONREG_SR_MMU)

/*
 * Bit definitions for USR register
 */
#define	SH5_CONREG_USR_GPRS_MASK	0xff	/* GP Reg dirty state mask */
#define	SH5_CONREG_USR_GPRS_SHIFT	0	/* GP Reg dirty state shift */
#define	SH5_CONREG_USR_FPRS_MASK	0xff	/* FP Reg dirty state mask */
#define	SH5_CONREG_USR_FPRS_SHIFT	8	/* FP Reg dirty state shift */

#define	SH5_GP_IS_DIRTY(usr,r)		((usr & (1 << ((r)/8))) != 0)
#define	SH5_FP_IS_DIRTY(usr,r)		((usr & (1 << (8+((r)/8)))) != 0)

#if defined(_KERNEL) && !defined(_LOCORE)
static __inline u_int
sh5_getctc(void)
{
	u_int64_t rv;

	__asm __volatile("getcon ctc, %0" : "=r"(rv));

	return ((u_int)rv);
}

extern void sh5_setasid(u_int);
#endif

#endif /* _SH5_CONREG_H */
