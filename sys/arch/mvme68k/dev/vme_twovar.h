/*	$NetBSD: vme_twovar.h,v 1.1 1999/02/20 00:12:01 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef __mvme68k_vme_twovar_h
#define __mvme68k_vme_twovar_h

#define VME2_VECTOR_BASE	(0x60u)
#define	VME2_VECTOR_MIN		0x08
#define VME2_VECTOR_MAX		0x1f

#define VME2_VEC_SOFT0		0x08
#define VME2_VEC_SOFT1		0x09
#define VME2_VEC_SOFT2		0x0a
#define VME2_VEC_SOFT3		0x0b
#define VME2_VEC_SOFT4		0x0c
#define VME2_VEC_SOFT5		0x0d
#define VME2_VEC_SOFT6		0x0e
#define VME2_VEC_SOFT7		0x0f
#define VME2_VEC_GCSRLM0	0x10
#define VME2_VEC_GCSRLM1	0x11
#define VME2_VEC_GCSRSIG0	0x12
#define VME2_VEC_GCSRSIG1	0x13
#define VME2_VEC_GCSRSIG2	0x14
#define VME2_VEC_GCSRSIG3	0x15
#define VME2_VEC_DMAC		0x16
#define VME2_VEC_VIA		0x17
#define VME2_VEC_TT1		0x18
#define VME2_VEC_TT2		0x19
#define VME2_VEC_IRQ1		0x1a
#define VME2_VEC_PARITY_ERROR	0x1b
#define VME2_VEC_MWP_ERROR	0x1c
#define VME2_VEC_SYSFAIL	0x1d
#define VME2_VEC_ABORT		0x1e
#define VME2_VEC_ACFAIL		0x1f

extern void vmetwo_intr_establish __P((int, void (*) __P((void *)),
					int, void *));
extern void vmetwo_intr_disestablish __P((int));

extern struct vme_two_lcsr *sys_vme_two;
extern struct vme_two_gcsr *sys_vme_two_gcsr;

#endif /* __mvme68k_vme_twovar_h */
