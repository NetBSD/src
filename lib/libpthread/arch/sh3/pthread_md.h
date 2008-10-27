/*	$NetBSD: pthread_md.h,v 1.5 2008/10/27 00:47:22 uwe Exp $ */

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

#ifndef _LIB_PTHREAD_SH3_MD_H
#define _LIB_PTHREAD_SH3_MD_H

static inline long
pthread__sp(void)
{
	long ret;
	__asm("mov r15, %0" : "=r" (ret));

	return ret;
}

#define pthread__uc_sp(ucp) ((ucp)->uc_mcontext.__gregs[_REG_R15])
#define pthread__uc_pc(ucp) ((ucp)->uc_mcontext.__gregs[_REG_PC])

/*
 * Set initial, sane values for registers whose values aren't just
 * "don't care".
 */
#define _INITCONTEXT_U_MD(ucp)						\
	(ucp)->uc_mcontext.__gregs[_REG_SR] = 0;

/*
 * SH3 requires no extra stack space
 */
#define STACKSPACE	0


/*
 * Conversions between struct reg and struct mcontext. Used by
 * libpthread_dbg.
 */

#define PTHREAD_UCONTEXT_TO_REG(reg, uc) do {				\
	(reg)->r_spc = (uc)->uc_mcontext.__gregs[_REG_PC];		\
	(reg)->r_ssr = (uc)->uc_mcontext.__gregs[_REG_SR];		\
	(reg)->r_pr = (uc)->uc_mcontext.__gregs[_REG_PR];		\
	(reg)->r_mach = (uc)->uc_mcontext.__gregs[_REG_MACH];		\
	(reg)->r_macl = (uc)->uc_mcontext.__gregs[_REG_MACL];		\
	(reg)->r_r15 = (uc)->uc_mcontext.__gregs[_REG_R15];		\
	(reg)->r_r14 = (uc)->uc_mcontext.__gregs[_REG_R14];		\
	(reg)->r_r13 = (uc)->uc_mcontext.__gregs[_REG_R13];		\
	(reg)->r_r12 = (uc)->uc_mcontext.__gregs[_REG_R12];		\
	(reg)->r_r11 = (uc)->uc_mcontext.__gregs[_REG_R11];		\
	(reg)->r_r10 = (uc)->uc_mcontext.__gregs[_REG_R10];		\
	(reg)->r_r9 = (uc)->uc_mcontext.__gregs[_REG_R9];		\
	(reg)->r_r8 = (uc)->uc_mcontext.__gregs[_REG_R8];		\
	(reg)->r_r7 = (uc)->uc_mcontext.__gregs[_REG_R7];		\
	(reg)->r_r6 = (uc)->uc_mcontext.__gregs[_REG_R6];		\
	(reg)->r_r5 = (uc)->uc_mcontext.__gregs[_REG_R5];		\
	(reg)->r_r4 = (uc)->uc_mcontext.__gregs[_REG_R4];		\
	(reg)->r_r3 = (uc)->uc_mcontext.__gregs[_REG_R3];		\
	(reg)->r_r2 = (uc)->uc_mcontext.__gregs[_REG_R2];		\
	(reg)->r_r1 = (uc)->uc_mcontext.__gregs[_REG_R1];		\
	(reg)->r_r0 = (uc)->uc_mcontext.__gregs[_REG_R0];		\
	(reg)->r_gbr = (uc)->uc_mcontext.__gregs[_REG_GBR];		\
	} while (/*CONSTCOND*/0)

#define PTHREAD_REG_TO_UCONTEXT(uc, reg) do {				\
	(uc)->uc_mcontext.__gregs[_REG_GBR] = (reg)->r_gbr;		\
	(uc)->uc_mcontext.__gregs[_REG_PC] = (reg)->r_spc;		\
	(uc)->uc_mcontext.__gregs[_REG_SR] = (reg)->r_ssr;		\
	(uc)->uc_mcontext.__gregs[_REG_PR] = (reg)->r_pr; 		\
	(uc)->uc_mcontext.__gregs[_REG_MACH] = (reg)->r_mach;		\
	(uc)->uc_mcontext.__gregs[_REG_MACL] = (reg)->r_macl;		\
	(uc)->uc_mcontext.__gregs[_REG_R15] = (reg)->r_r15;		\
	(uc)->uc_mcontext.__gregs[_REG_R14] = (reg)->r_r14;		\
	(uc)->uc_mcontext.__gregs[_REG_R13] = (reg)->r_r13;		\
	(uc)->uc_mcontext.__gregs[_REG_R12] = (reg)->r_r12;		\
	(uc)->uc_mcontext.__gregs[_REG_R11] = (reg)->r_r11;		\
	(uc)->uc_mcontext.__gregs[_REG_R10] = (reg)->r_r10;		\
	(uc)->uc_mcontext.__gregs[_REG_R9] = (reg)->r_r9;		\
	(uc)->uc_mcontext.__gregs[_REG_R8] = (reg)->r_r8;		\
	(uc)->uc_mcontext.__gregs[_REG_R7] = (reg)->r_r7;		\
	(uc)->uc_mcontext.__gregs[_REG_R6] = (reg)->r_r6;		\
	(uc)->uc_mcontext.__gregs[_REG_R5] = (reg)->r_r5;		\
	(uc)->uc_mcontext.__gregs[_REG_R4] = (reg)->r_r4;		\
	(uc)->uc_mcontext.__gregs[_REG_R3] = (reg)->r_r3;		\
	(uc)->uc_mcontext.__gregs[_REG_R2] = (reg)->r_r2;		\
	(uc)->uc_mcontext.__gregs[_REG_R1] = (reg)->r_r1;		\
	(uc)->uc_mcontext.__gregs[_REG_R0] = (reg)->r_r0;		\
									\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_CPU) & ~_UC_USER;	\
	} while (/*CONSTCOND*/0)

#if 0 /* no struct fpreg!!! */
#define PTHREAD_UCONTEXT_TO_FPREG(freg, uc)       			\
	memcpy((freg), &(uc)->uc_mcontext.__fpregs, sizeof(*(freg)));

#define PTHREAD_FPREG_TO_UCONTEXT(uc, freg) do {       	       		\
	memcpy(&(uc)->uc_mcontext.__fpregs, (freg), sizeof(*(freg)));	\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_FPU) & ~_UC_USER;       	\
	} while (/*CONSTCOND*/0)

#else  /* SBUBS */
#define PTHREAD_UCONTEXT_TO_FPREG(freg, uc)
#define PTHREAD_FPREG_TO_UCONTEXT(uc, freg)
#endif

/* sh3 will not go SMP */
#define	PTHREAD__ATOMIC_IS_MEMBAR

#endif /* _LIB_PTHREAD_SH3_MD_H */
