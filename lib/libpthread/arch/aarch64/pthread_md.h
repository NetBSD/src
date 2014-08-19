/* $NetBSD: pthread_md.h,v 1.1.4.2 2014/08/20 00:02:20 tls Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _LIB_PTHREAD_AARCH64_MD_H
#define	_LIB_PTHREAD_AARCH64_MD_H

static inline uintptr_t
pthread__sp(void)
{
	uintptr_t ret;

	__asm __volatile("mov %0, sp" : "=r" (ret));

	return ret;
}

#define pthread__smt_pause()	__asm __volatile("wfe") /* wfe */
#define pthread__smt_wake()	__asm __volatile("sev") /* sev */

#define	pthread__uc_sp(ucp)	((ucp)->uc_mcontext.__gregs[_REG_SP])

/*
 * Set initial, sane values for registers whose values aren't just
 * "don't care".
 */
#define _INITCONTEXT_U_MD(ucp)	\
	(ucp)->uc_mcontext.__gregs[_REG_SPSR] = 0;

#endif /* _LIB_PTHREAD_AARCH64_MD_H */
