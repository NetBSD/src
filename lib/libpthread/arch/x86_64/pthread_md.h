/*	$NetBSD: pthread_md.h,v 1.1 2003/01/30 02:10:32 fvdl Exp $	*/

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
 *
 * Adapted for x86_64 by fvdl@netbsd.org
 */

#ifndef _LIB_PTHREAD_X86_64_MD_H
#define _LIB_PTHREAD_X86_64_MD_H

#include <sys/ucontext.h>

static __inline long
pthread__sp(void)
{
	long ret;
	__asm("movq %%rsp, %0" : "=g" (ret));

	return ret;
}

#define pthread__uc_sp(ucp) ((ucp)->uc_mcontext.__gregs[_REG_URSP])
#define pthread__uc_pc(ucp) ((ucp)->uc_mcontext.__gregs[_REG_RIP])

/*
 * Set initial, sane values for registers whose values aren't just
 * "don't care".
 * 0x23 is GSEL(GUDATA_SEL, SEL_UPL), and
 * 0x1b is GSEL(GUCODE_SEL, SEL_UPL).
 * 0x202 is PSL_USERSET.
 */
#define _INITCONTEXT_U_MD(ucp)						\
	(ucp)->uc_mcontext.__gregs[_REG_GS] = 0x23,			\
	(ucp)->uc_mcontext.__gregs[_REG_FS] = 0x23,			\
	(ucp)->uc_mcontext.__gregs[_REG_ES] = 0x23,			\
	(ucp)->uc_mcontext.__gregs[_REG_DS] = 0x23,			\
	(ucp)->uc_mcontext.__gregs[_REG_CS] = 0x1b,			\
	(ucp)->uc_mcontext.__gregs[_REG_SS] = 0x23,			\
	(ucp)->uc_mcontext.__gregs[_REG_RFL] = 0x202;

/*
 * Usable stack space below the ucontext_t. 
 * See comment in pthread_switch.S about STACK_SWITCH.
 */
#define STACKSPACE	64	/* room for 8 long values */

/*
 * Conversions between struct reg and struct mcontext. Used by
 * libpthread_dbg.
 */

#define PTHREAD_UCONTEXT_TO_REG(reg, uc) do {				\
	(reg)->r_gs  = (uc)->uc_mcontext.__gregs[_REG_GS];		\
	(reg)->r_fs  = (uc)->uc_mcontext.__gregs[_REG_FS];		\
	(reg)->r_es  = (uc)->uc_mcontext.__gregs[_REG_ES];		\
	(reg)->r_ds  = (uc)->uc_mcontext.__gregs[_REG_DS];		\
	(reg)->r_rdi = (uc)->uc_mcontext.__gregs[_REG_RDI];		\
	(reg)->r_rsi = (uc)->uc_mcontext.__gregs[_REG_RSI];		\
	(reg)->r_rbp = (uc)->uc_mcontext.__gregs[_REG_RBP];		\
	(reg)->r_rbx = (uc)->uc_mcontext.__gregs[_REG_RBX];		\
	(reg)->r_rdx = (uc)->uc_mcontext.__gregs[_REG_RDX];		\
	(reg)->r_rcx = (uc)->uc_mcontext.__gregs[_REG_RCX];		\
	(reg)->r_rax = (uc)->uc_mcontext.__gregs[_REG_RAX];		\
	(reg)->r_r8  = (uc)->uc_mcontext.__gregs[_REG_R8];		\
	(reg)->r_r9  = (uc)->uc_mcontext.__gregs[_REG_R9];		\
	(reg)->r_r10 = (uc)->uc_mcontext.__gregs[_REG_R10];		\
	(reg)->r_r11 = (uc)->uc_mcontext.__gregs[_REG_R11];		\
	(reg)->r_r12 = (uc)->uc_mcontext.__gregs[_REG_R12];		\
	(reg)->r_r13 = (uc)->uc_mcontext.__gregs[_REG_R13];		\
	(reg)->r_r14 = (uc)->uc_mcontext.__gregs[_REG_R14];		\
	(reg)->r_r15 = (uc)->uc_mcontext.__gregs[_REG_R15];		\
	(reg)->r_rip = (uc)->uc_mcontext.__gregs[_REG_RIP];		\
	(reg)->r_cs  = (uc)->uc_mcontext.__gregs[_REG_CS];		\
	(reg)->r_rflags  = (uc)->uc_mcontext.__gregs[_REG_RFL];		\
	(reg)->r_rsp = (uc)->uc_mcontext.__gregs[_REG_URSP];		\
	(reg)->r_ss  = (uc)->uc_mcontext.__gregs[_REG_SS];		\
	} while (/*CONSTCOND*/0)

#define PTHREAD_REG_TO_UCONTEXT(uc, reg) do {				\
	(uc)->uc_mcontext.__gregs[_REG_GS]  = (reg)->r_gs;		\
	(uc)->uc_mcontext.__gregs[_REG_FS]  = (reg)->r_fs; 		\
	(uc)->uc_mcontext.__gregs[_REG_ES]  = (reg)->r_es; 		\
	(uc)->uc_mcontext.__gregs[_REG_DS]  = (reg)->r_ds; 		\
	(uc)->uc_mcontext.__gregs[_REG_RDI] = (reg)->r_rdi; 		\
	(uc)->uc_mcontext.__gregs[_REG_RSI] = (reg)->r_rsi; 		\
	(uc)->uc_mcontext.__gregs[_REG_RBP] = (reg)->r_rbp; 		\
	(uc)->uc_mcontext.__gregs[_REG_RBX] = (reg)->r_rbx; 		\
	(uc)->uc_mcontext.__gregs[_REG_RDX] = (reg)->r_rdx; 		\
	(uc)->uc_mcontext.__gregs[_REG_RCX] = (reg)->r_rcx; 		\
	(uc)->uc_mcontext.__gregs[_REG_RAX] = (reg)->r_rax; 		\
	(uc)->uc_mcontext.__gregs[_REG_R8]  = (reg)->r_r8; 		\
	(uc)->uc_mcontext.__gregs[_REG_R9]  = (reg)->r_r9; 		\
	(uc)->uc_mcontext.__gregs[_REG_R10] = (reg)->r_r10; 		\
	(uc)->uc_mcontext.__gregs[_REG_R11] = (reg)->r_r11; 		\
	(uc)->uc_mcontext.__gregs[_REG_R12] = (reg)->r_r12; 		\
	(uc)->uc_mcontext.__gregs[_REG_R13] = (reg)->r_r13; 		\
	(uc)->uc_mcontext.__gregs[_REG_R14] = (reg)->r_r14; 		\
	(uc)->uc_mcontext.__gregs[_REG_R15] = (reg)->r_r15; 		\
	(uc)->uc_mcontext.__gregs[_REG_RIP] = (reg)->r_rip; 		\
	(uc)->uc_mcontext.__gregs[_REG_CS]  = (reg)->r_cs; 		\
	(uc)->uc_mcontext.__gregs[_REG_RFL] = (reg)->r_rflags; 		\
	(uc)->uc_mcontext.__gregs[_REG_URSP]= (reg)->r_rsp;		\
	(uc)->uc_mcontext.__gregs[_REG_SS]  = (reg)->r_ss; 		\
	/*LINTED precision loss */					\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_CPU) & ~_UC_USER; 	\
	} while (/*CONSTCOND*/0)


#define PTHREAD_UCONTEXT_TO_FPREG(freg, uc)		       		\
	(void)memcpy(&(freg)->fxstate,					\
        (uc)->uc_mcontext.__fpregs, sizeof(struct fpreg))

#define PTHREAD_FPREG_TO_UCONTEXT(uc, freg) do {       	       		\
	(void)memcpy(							\
        (uc)->uc_mcontext.__fpregs,					\
	&(freg)->fxstate, sizeof(struct fpreg));			\
	/*LINTED precision loss */					\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_FPU) & ~_UC_USER;	\
	} while (/*CONSTCOND*/0)

#endif /* _LIB_PTHREAD_X86_64_MD_H */
