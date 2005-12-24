/*	$NetBSD: pthread_md.h,v 1.4 2005/12/24 21:11:17 perry Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#ifndef _LIB_PTHREAD_SPARC_MD_H
#define _LIB_PTHREAD_SPARC_MD_H

/*
 * pthread__sp used for identifying thread
 */
static inline long
pthread__sp(void)
{
	long ret;

	__asm("mov %%sp, %0" : "=r" (ret));

	return ret;
}

#define pthread__uc_sp(ucp) ((ucp)->uc_mcontext.__gregs[_REG_O6])
#define pthread__uc_pc(ucp) ((ucp)->uc_mcontext.__gregs[_REG_PC])

#define STACKSPACE 96	/* min stack frame XXX */

/*
 * Conversions between struct reg and struct mcontext. Used by
 * libpthread_dbg.
 * XXX macros
 */

#define PTHREAD_UCONTEXT_TO_REG(reg, uc) do {				\
	memcpy((reg), &(uc)->uc_mcontext.__gregs, 			\
	    (_REG_Y+1)*sizeof(__greg_t));				\
	(reg)->r_global[0] = 0;						\
	memcpy(&(reg)->r_global[1], &(uc)->uc_mcontext.__gregs[_REG_G1],\
	     (_REG_O7-_REG_G1+1)*sizeof(__greg_t));			\
	} while (/*CONSTCOND*/0)

#define PTHREAD_REG_TO_UCONTEXT(uc, reg) do {				\
	memcpy(&(uc)->uc_mcontext.__gregs, (reg), 			\
	    (_REG_Y+1)*sizeof(__greg_t)); 				\
	(uc)->uc_mcontext.__gregs[_REG_Y] = (reg)->r_y;			\
	memcpy(&(uc)->uc_mcontext.__gregs[_REG_G1], &(reg)->r_global[1],\
	    (_REG_O7-_REG_G1+1)*sizeof(__greg_t));			\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_CPU) & ~_UC_USER;       	\
	} while (/*CONSTCOND*/0)

#define PTHREAD_UCONTEXT_TO_FPREG(freg, uc) do {			\
	memcpy((freg)->fr_regs,						\
	    &(uc)->uc_mcontext.__fpregs.__fpu_fr.__fpu_regs,		\
	    32*sizeof(u_int));						\
	(freg)->fr_fsr = (uc)->uc_mcontext.__fpregs.__fpu_fsr;		\
	} while (/*CONSTCOND*/0)

#define PTHREAD_FPREG_TO_UCONTEXT(uc, freg) do {       	       		\
	memcpy(&(uc)->uc_mcontext.__fpregs.__fpu_fr.__fpu_regs,		\
	    (freg)->fr_regs,						\
	    32*sizeof(u_int));						\
	(uc)->uc_mcontext.__fpregs.__fpu_fsr = (freg)->fr_fsr;		\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_FPU) & ~_UC_USER;       	\
	} while (/*CONSTCOND*/0)

#endif /* _LIB_PTHREAD_SPARC_MD_H */
