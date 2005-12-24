/*	$NetBSD: pthread_md.h,v 1.3 2005/12/24 21:11:17 perry Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
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

#ifndef _LIB_PTHREAD_SH5_MD_H
#define _LIB_PTHREAD_SH5_MD_H

static inline long
pthread__sp(void)
{
	register_t ret;

	__asm volatile("add r15, r63, %0" : "=g" (ret));

	return ((long)ret);
}

#define pthread__uc_sp(ucp) ((ucp)->uc_mcontext.__gregs[_REG_SP])
#define pthread__uc_pc(ucp) ((ucp)->uc_mcontext.__gregs[_REG_PC])

/*
 * SH5 requires no extra stack space
 */
#define STACKSPACE	0

/*
 * Conversions between struct reg and struct mcontext. Used by
 * libpthread_dbg.
 */

#define PTHREAD_UCONTEXT_TO_REG(reg, uc)				\
	memcpy((reg), &(uc)->uc_mcontext.__gregs[0], sizeof(*(reg)))

#define PTHREAD_REG_TO_UCONTEXT(uc, reg) do {				\
	memcpy(&(uc)->uc_mcontext.__gregs[0], (reg), sizeof(*(reg)));	\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_CPU) & ~_UC_USER;	\
	} while (/*CONSTCOND*/0)

#define PTHREAD_UCONTEXT_TO_FPREG(freg, uc)       			\
	memcpy((freg), &(uc)->uc_mcontext.__fpregs, sizeof(*(freg)));

#define PTHREAD_FPREG_TO_UCONTEXT(uc, freg) do {       	       		\
	memcpy(&(uc)->uc_mcontext.__fpregs, (freg), sizeof(*(freg)));	\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_FPU) & ~_UC_USER;       	\
	} while (/*CONSTCOND*/0)

#endif /* _LIB_PTHREAD_SH5_MD_H */
