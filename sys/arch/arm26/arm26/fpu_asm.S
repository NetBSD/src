/*	$NetBSD: fpu_asm.S,v 1.1 2001/01/12 13:23:49 bjh21 Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * fpu_asm.S - Floating point unit support
 */

#include <machine/asm.h>

RCSID("$NetBSD: fpu_asm.S,v 1.1 2001/01/12 13:23:49 bjh21 Exp $")

#include <machine/fpureg.h>

#include "opt_fputypes.h"

#ifdef FPU_FPA
/*
 * void fpctx_save_fpa(struct fpframe *);
 */
ENTRY(fpctx_save_fpa)
	rfs	r1
	str	r1, [r0]
	sfm	f0, 4, [r0, #(4 + 12 * 0)]
	sfm	f4, 4, [r0, #(4 + 12 * 4)]
#ifdef __APCS_26__
	movs	pc, lr
#else
	mov	pc, lr
#endif

/*
 * void fpctx_load_fpa(struct fpframe *);
 */
ENTRY(fpctx_load_fpa)
	lfm	f0, 4, [r0, #(4 + 12 * 0)]
	lfm	f4, 4, [r0, #(4 + 12 * 4)]
	ldr	r1, [r0]
	wfs	r1
#ifdef __APCS_26__
	movs	pc, lr
#else
	mov	pc, lr
#endif

/*
 * void fpu_enable_fpa(void);
 */
ENTRY(fpu_enable_fpa)
	mov	r0, #FPA_FPCR_EN
	wfc	r0
#ifdef __APCS_26__
	movs	pc, lr
#else
	mov	pc, lr
#endif
#endif

/*
 * void fpu_disable_fpa(void);
 */
ENTRY(fpu_disable_fpa)
	mov	r0, #0
	wfc	r0
#ifdef __APCS_26__
	movs	pc, lr
#else
	mov	pc, lr
#endif

#ifdef FPU_FPPC
/*
 * void fpctx_save_fppc(struct fpframe *);
 */
ENTRY(fpctx_save_fppc)
	rfs	r1
	str	r1, [r0]
	stfe	f0, [r0, #(4 + 12 * 0)]
	stfe	f1, [r0, #(4 + 12 * 1)]
	stfe	f2, [r0, #(4 + 12 * 2)]
	stfe	f3, [r0, #(4 + 12 * 3)]
	stfe	f4, [r0, #(4 + 12 * 4)]
	stfe	f5, [r0, #(4 + 12 * 5)]
	stfe	f6, [r0, #(4 + 12 * 6)]
	stfe	f7, [r0, #(4 + 12 * 7)]
#ifdef __APCS_26__
	movs	pc, lr
#else
	mov	pc, lr
#endif

/*
 * void fpctx_load_fppc(struct fpframe *);
 */
ENTRY(fpctx_load_fppc)
	ldfe	f0, [r0, #(4 + 12 * 0)]
	ldfe	f1, [r0, #(4 + 12 * 1)]
	ldfe	f2, [r0, #(4 + 12 * 2)]
	ldfe	f3, [r0, #(4 + 12 * 3)]
	ldfe	f4, [r0, #(4 + 12 * 4)]
	ldfe	f5, [r0, #(4 + 12 * 5)]
	ldfe	f6, [r0, #(4 + 12 * 6)]
	ldfe	f7, [r0, #(4 + 12 * 7)]
	ldr	r1, [r0]
	wfs	r1
#ifdef __APCS_26__
	movs	pc, lr
#else
	mov	pc, lr
#endif

/*
 * void fpu_enable_fppc(void);
 */
ENTRY(fpu_enable_fppc)
	rfc	r0
	orr	r0, r0, #FPPC_FPCR_DA
	wfc	r0
#ifdef __APCS_26__
	movs	pc, lr
#else
	mov	pc, lr
#endif
#endif

/*
 * void fpu_disable_fppc(void);
 */
ENTRY(fpu_disable_fppc)
	rfc	r0
	bic	r0, r0, #FPPC_FPCR_DA
	wfc	r0
#ifdef __APCS_26__
	movs	pc, lr
#else
	mov	pc, lr
#endif
