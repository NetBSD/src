/*	$NetBSD: pthread_md.h,v 1.4 2005/12/24 21:11:16 perry Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

#ifndef _LIB_PTHREAD_ARM_MD_H
#define	_LIB_PTHREAD_ARM_MD_H

static inline long
pthread__sp(void)
{
	long ret;

	__asm volatile("mov %0, sp"
		: "=r" (ret));

	return (ret);
}

#define	pthread__uc_sp(ucp)	((ucp)->uc_mcontext.__gregs[_REG_SP])
#define	pthread__uc_pc(ucp)	((ucp)->uc_mcontext.__gregs[_REG_PC])

/*
 * Set initial, sane values for registers whose values aren't just
 * "don't care".
 */
#ifdef __APCS_26__
#define _INITCONTEXT_U_MD(ucp)						\
/* Set R15_MODE_USR in the PC */					\
	(ucp)->uc_mcontext.__gregs[_REG_PC] =				\
	 ((ucp)->uc_mcontext.__gregs[_REG_PC] & 0x3fffffc) | 0x0;
#else
/* Set CPSR to PSR_USE32_MODE (0x10) from arm/armreg.h */
#define _INITCONTEXT_U_MD(ucp)						\
	(ucp)->uc_mcontext.__gregs[_REG_CPSR] = 0x10;
#endif

/*
 * Usable stack space below the ucontext_t.
 *    For a good time, see comments in pthread_switch.S and
 *    ../i386/pthread_switch.S about STACK_SWITCH.
 */
#define	STACKSPACE		(6 * sizeof(long))

/*
 * Conversions between struct reg and struct mcontext.  Used by
 * libpthread_dbg.
 */

#define	PTHREAD_ARM_UCONTEXT_TO_REG(reg, uc)				\
do {									\
	int _reg_;							\
									\
	for (_reg_ = 0; _reg_ <= 12; _reg_++)				\
		(reg)->r[_reg_] =					\
		    (uc)->uc_mcontext.__gregs[_REG_R0 + _reg_];		\
	(reg)->r_sp = (uc)->uc_mcontext.__gregs[_REG_SP];		\
	(reg)->r_lr = (uc)->uc_mcontext.__gregs[_REG_LR];		\
	(reg)->r_pc = (uc)->uc_mcontext.__gregs[_REG_PC];		\
	(reg)->r_cpsr = (uc)->uc_mcontext.__gregs[_REG_CPSR];		\
} while (/*CONSTCOND*/0)

#ifdef __APCS_26__
#define PTHREAD_UCONTEXT_TO_REG(reg, uc) PTHREAD_ARM_UCONTEXT_TO_REG((reg), (uc))
#else
/* Need to signal in the CPSR that this is 32-bit ARM */
#define PTHREAD_UCONTEXT_TO_REG(reg, uc)				\
do {									\
	PTHREAD_ARM_UCONTEXT_TO_REG((reg), (uc));			\
	if ((uc)->uc_flags & _UC_USER)					\
		(reg)->r_cpsr = 0x10;					\
} while (/*CONSTCOND*/0)
#endif


#define	PTHREAD_REG_TO_UCONTEXT(uc, reg)				\
do {									\
	int _reg_;							\
									\
	for (_reg_ = 0; _reg_ <= 12; _reg_++)				\
		(uc)->uc_mcontext.__gregs[_REG_R0 + _reg_] =		\
		    (reg)->r[_reg_];					\
	(uc)->uc_mcontext.__gregs[_REG_SP] = (reg)->r_sp;		\
	(uc)->uc_mcontext.__gregs[_REG_LR] = (reg)->r_lr;		\
	(uc)->uc_mcontext.__gregs[_REG_PC] = (reg)->r_pc;		\
	(uc)->uc_mcontext.__gregs[_REG_CPSR] = (reg)->r_cpsr;		\
} while (/*CONSTCOND*/0)

/*
 * XXX Need to deal with VFP.
 */
#define	PTHREAD_UCONTEXT_TO_FPREG(freg, uc)				\
do {									\
	(freg)->fpr_fpsr = (uc)->uc_mcontext.__fpu.__fpregs.__fp_fpsr;	\
	memcpy((freg)->fpr, (uc)->uc_mcontext.__fpu.__fpregs.__fp_fr,	\
	    sizeof((freg)->fpr));					\
} while (/*CONSTCOND*/0)

#define	PTHREAD_FPREG_TO_UCONTEXT(uc, freg)				\
do {									\
	(uc)->uc_mcontext.__fpu.__fpregs.__fp_fpsr = (freg)->fpr_fpsr;	\
	memcpy((uc)->uc_mcontext.__fpu.__fpregs.__fp_fr, (freg)->fpr,	\
	    sizeof((uc)->uc_mcontext.__fpu.__fpregs.__fp_fr));		\
} while (/*CONSTCOND*/0)

#endif /* _LIB_PTHREAD_ARM_MD_H */
