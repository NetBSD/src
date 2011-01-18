/*	$NetBSD: altivec.h,v 1.13 2011/01/18 01:02:54 matt Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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

#ifndef	_POWERPC_ALTIVEC_H_
#define	_POWERPC_ALTIVEC_H_

#define	VSCR_SA		0x00000001	/* Saturation happended (sticky) */
#define	VSCR_NJ		0x00010000	/* Non Java-IEEE-C9X FP mode */

#ifdef _KERNEL
#include <powerpc/mcontext.h>

enum vec_op { VEC_SAVE, VEC_DISCARD, VEC_SAVE_AND_RELEASE };
struct lwp;
struct vreg;
struct trapframe;

void	vec_enable(void);
void	vec_save_cpu(enum vec_op);
void	vec_save_lwp(struct lwp *, enum vec_op);
void	vec_restore_from_mcontext(struct lwp *, const mcontext_t *);
bool	vec_save_to_mcontext(struct lwp *, mcontext_t *, unsigned int *);

void	vec_load_from_vreg(const struct vreg *);
void	vec_unload_to_vreg(struct vreg *);

int	vec_siginfo_code(const struct trapframe *);

/* OEA only */
void	vzeropage(paddr_t);
void	vcopypage(paddr_t, paddr_t);	/* dst, src */
#endif

#endif	/* _POWERPC_ALTIVEC_H_ */
