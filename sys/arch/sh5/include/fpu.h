/*	$NetBSD: fpu.h,v 1.2 2003/10/05 09:57:47 scw Exp $	*/

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
 * Definitions for the SH5's Floating Point Unit
 */

#ifndef _SH5_FPU_H
#define _SH5_FPU_H

/*
 * Bits in the Floating Point Status and Control Register
 */

/* Rounding mode */
#define	SH5_FPSCR_RM_MASK		0x00001
#define	SH5_FPSCR_RM_TO_NEAREST		0x00000
#define	SH5_FPSCR_RM_TO_ZERO		0x00001

/* Sticky Flags */
#define	SH5_FPSCR_FLAG_I		0x00004	/* Inexact */
#define	SH5_FPSCR_FLAG_U		0x00008	/* Underflow */
#define	SH5_FPSCR_FLAG_O		0x00010	/* Overflow */
#define	SH5_FPSCR_FLAG_Z		0x00020	/* Divide by Zero */
#define	SH5_FPSCR_FLAG_V		0x00040	/* Invalid */
#define	SH5_FPSCR_FLAG_MASK		0x0007c

/* Enable Flags */
#define	SH5_FPSCR_ENABLE_I		0x00080	/* Inexact */
#define	SH5_FPSCR_ENABLE_U		0x00100	/* Underflow */
#define	SH5_FPSCR_ENABLE_O		0x00200	/* Overflow */
#define	SH5_FPSCR_ENABLE_Z		0x00400	/* Divide by Zero */
#define	SH5_FPSCR_ENABLE_V		0x00800	/* Invalid */
#define	SH5_FPSCR_ENABLE_MASK		0x00f80

/* Cause Flags */
#define	SH5_FPSCR_CAUSE_I		0x01000	/* Inexact */
#define	SH5_FPSCR_CAUSE_U		0x02000	/* Underflow */
#define	SH5_FPSCR_CAUSE_O		0x04000	/* Overflow */
#define	SH5_FPSCR_CAUSE_Z		0x08000	/* Divide by Zero */
#define	SH5_FPSCR_CAUSE_V		0x10000	/* Invalid */
#define	SH5_FPSCR_CAUSE_E		0x20000	/* FPU Error */
#define	SH5_FPSCR_CAUSE_MASK		0x3f000

/* Treatment of denormalised source operands */
#define	SH5_FPSCR_DN_MASK		0x40000
#define	SH5_FPSCR_DN_RAISE		0x00000 /* Raise an exception */
#define	SH5_FPSCR_DN_FLUSH_ZERO		0x40000 /* Flush to zero */

#ifdef _KERNEL
extern u_int32_t sh5_getfpscr(void);
extern void sh5_setfpscr(u_int32_t);
#endif

#endif /* _SH5_FPU_H */
