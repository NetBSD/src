/*	$NetBSD: pthread_md.h,v 1.1.2.5 2002/04/24 05:20:48 nathanw Exp $	*/

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

#ifndef _LIB_PTHREAD_I386_MD_H
#define _LIB_PTHREAD_I386_MD_H


static __inline long
pthread__sp(void)
{
	long ret;
	__asm("movl %%esp, %0" : "=g" (ret));

	return ret;
}

#define pthread__uc_sp(ucp) ((ucp)->uc_mcontext.__gregs[_REG_UESP])
#define pthread__uc_pc(ucp) ((ucp)->uc_mcontext.__gregs[_REG_EIP])

/*
 * Usable stack space below the ucontext_t. 
 * See comment in pthread_switch.S about STACK_SWITCH.
 */
#define STACKSPACE	32	/* room for 8 integer values */

/*
 * Conversions between struct reg and struct mcontext. Used by
 * libpthread_dbg.
 */

#define PTHREAD_UCONTEXT_TO_REG(reg, uc) do {				\
	(reg)->r_gs  = (uc)->uc_mcontext.__gregs[_REG_GS];		\
	(reg)->r_fs  = (uc)->uc_mcontext.__gregs[_REG_FS];		\
	(reg)->r_es  = (uc)->uc_mcontext.__gregs[_REG_ES];		\
	(reg)->r_ds  = (uc)->uc_mcontext.__gregs[_REG_DS];		\
	(reg)->r_edi = (uc)->uc_mcontext.__gregs[_REG_EDI];		\
	(reg)->r_esi = (uc)->uc_mcontext.__gregs[_REG_ESI];		\
	(reg)->r_ebp = (uc)->uc_mcontext.__gregs[_REG_EBP];		\
	(reg)->r_ebx = (uc)->uc_mcontext.__gregs[_REG_EBX];		\
	(reg)->r_edx = (uc)->uc_mcontext.__gregs[_REG_EDX];		\
	(reg)->r_ecx = (uc)->uc_mcontext.__gregs[_REG_ECX];		\
	(reg)->r_eax = (uc)->uc_mcontext.__gregs[_REG_EAX];		\
	(reg)->r_eip = (uc)->uc_mcontext.__gregs[_REG_EIP];		\
	(reg)->r_cs  = (uc)->uc_mcontext.__gregs[_REG_CS];		\
	(reg)->r_eflags  = (uc)->uc_mcontext.__gregs[_REG_EFL];		\
	(reg)->r_esp = (uc)->uc_mcontext.__gregs[_REG_UESP];		\
	(reg)->r_ss  = (uc)->uc_mcontext.__gregs[_REG_SS];		\
	} while (/*CONSTCOND*/0)

#define PTHREAD_REG_TO_UCONTEXT(uc, reg) do {				\
	(uc)->uc_mcontext.__gregs[_REG_GS]  = (reg)->r_gs;		\
	(uc)->uc_mcontext.__gregs[_REG_FS]  = (reg)->r_fs; 		\
	(uc)->uc_mcontext.__gregs[_REG_ES]  = (reg)->r_es; 		\
	(uc)->uc_mcontext.__gregs[_REG_DS]  = (reg)->r_ds; 		\
	(uc)->uc_mcontext.__gregs[_REG_EDI] = (reg)->r_edi; 		\
	(uc)->uc_mcontext.__gregs[_REG_ESI] = (reg)->r_esi; 		\
	(uc)->uc_mcontext.__gregs[_REG_EBP] = (reg)->r_ebp; 		\
	(uc)->uc_mcontext.__gregs[_REG_EBX] = (reg)->r_ebx; 		\
	(uc)->uc_mcontext.__gregs[_REG_EDX] = (reg)->r_edx; 		\
	(uc)->uc_mcontext.__gregs[_REG_ECX] = (reg)->r_ecx; 		\
	(uc)->uc_mcontext.__gregs[_REG_EAX] = (reg)->r_eax; 		\
	(uc)->uc_mcontext.__gregs[_REG_EIP] = (reg)->r_eip; 		\
	(uc)->uc_mcontext.__gregs[_REG_CS]  = (reg)->r_cs; 		\
	(uc)->uc_mcontext.__gregs[_REG_EFL] = (reg)->r_eflags; 		\
	(uc)->uc_mcontext.__gregs[_REG_UESP]= (reg)->r_esp;		\
	(uc)->uc_mcontext.__gregs[_REG_SS]  = (reg)->r_ss; 		\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_CPU) & ~_UC_USER;       	\
	} while (/*CONSTCOND*/0)

#define PTHREAD_UCONTEXT_TO_FPREG(freg, uc)		       		\
	memcpy((freg)->__data,						\
        (uc)->uc_mcontext.__fpregs.__fp_reg_set.__fpchip_state.__fp_state, \
	sizeof(struct fpreg))

#define PTHREAD_FPREG_TO_UCONTEXT(uc, freg) do {       	       		\
	memcpy(								\
        (uc)->uc_mcontext.__fpregs.__fp_reg_set.__fpchip_state.__fp_state, \
	(freg)->__data, sizeof(struct fpreg));				\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_FPU) & ~_UC_USER;       	\
	} while (/*CONSTCOND*/0)

#endif /* _LIB_PTHREAD_I386_MD_H */
