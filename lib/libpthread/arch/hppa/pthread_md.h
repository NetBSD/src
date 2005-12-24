/*	$NetBSD: pthread_md.h,v 1.2 2005/12/24 21:11:16 perry Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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

#ifndef _LIB_PTHREAD_HPPA_MD_H
#define _LIB_PTHREAD_HPPA_MD_H

#include <machine/frame.h>

static inline long
pthread__sp(void)
{
	register long sp __asm__("r30");

	return sp;
}

#define pthread__uc_sp(ucp) ((ucp)->uc_mcontext.__gregs[_REG_SP])
#define pthread__uc_pc(ucp) ((ucp)->uc_mcontext.__gregs[_REG_PCOQH])

/*
 * Set initial, sane values for registers whose values aren't just
 * "don't care".
 */
#define _INITCONTEXT_U_MD(ucp)						\
	(ucp)->uc_mcontext.__gregs[_REG_PSW] = 0x4000f;

/*
 * Usable stack space below the ucontext_t.
 */
#define STACKSPACE	(HPPA_FRAME_SIZE)

/*
 * Conversions between struct reg and struct mcontext. Used by
 * libpthread_dbg.
 */
#include <hppa/reg.h>

#define PTHREAD_UCONTEXT_TO_REG(reg, uc)				\
do {									\
	memcpy(&(reg)->r_regs, &(uc)->uc_mcontext.__gregs,		\
	    sizeof(__greg_t) * 32);					\
} while (/*CONSTCOND*/0)

#define PTHREAD_REG_TO_UCONTEXT(uc, reg)				\
do {									\
	memcpy(&(uc)->uc_mcontext.__gregs, &(reg)->r_regs,		\
	    sizeof(__greg_t) * 32);					\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_CPU) & ~_UC_USER;       	\
} while (/*CONSTCOND*/0)

#define PTHREAD_UCONTEXT_TO_FPREG(freg, uc)       			\
do {									\
	memcpy((freg), &(uc)->uc_mcontext.__fpregs,			\
	    sizeof(struct fpreg));					\
} while (/*CONSTCOND*/0)

#define PTHREAD_FPREG_TO_UCONTEXT(uc, freg)				\
do {						       	       		\
	memcpy(&(uc)->uc_mcontext.__fpregs, (freg),			\
	    sizeof(struct fpreg));					\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_FPU) & ~_UC_USER;       	\
} while (/*CONSTCOND*/0)

#endif /* !_LIB_PTHREAD_HPPA_MD_H */
