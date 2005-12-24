/*	$NetBSD: pthread_md.h,v 1.5 2005/12/24 21:11:16 perry Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Allen Briggs for Wasabi Systems, Inc.
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

#ifndef _LIB_PTHREAD_POWERPC_MD_H
#define _LIB_PTHREAD_POWERPC_MD_H

static inline long
pthread__sp(void)
{
	long	ret;

	__asm("mr %0,1" : "=r" (ret));

	return ret;
}

#define pthread__uc_sp(ucp) ((ucp)->uc_mcontext.__gregs[1])
#define pthread__uc_pc(ucp) ((ucp)->uc_mcontext.__gregs[34])

/*
 * Set initial, sane values for registers whose values aren't just
 * "don't care".
 * 0xd032 is PSL_USERSET from arch/powerpc/include/psl.h
 */
#define _INITCONTEXT_U_MD(ucp)						\
	(ucp)->uc_mcontext.__gregs[_REG_MSR] = 0xd032;

/*
 * Usable stack space below the ucontext_t.
 *    For a good time, see comments in pthread_switch.S and
 *    ../i386/pthread_switch.S about STACK_SWITCH.
 */
#define STACKSPACE	16	/* room for 4 integer values */

/*
 * Conversions between struct reg and struct mcontext. Used by
 * libpthread_dbg.
 */

#define PTHREAD_UCONTEXT_TO_REG(reg, uc) do {				\
	memcpy((reg)->fixreg, (uc)->uc_mcontext.__gregs, 32 * 4);	\
	(reg)->cr = (uc)->uc_mcontext.__gregs[_REG_CR];			\
	(reg)->lr = (uc)->uc_mcontext.__gregs[_REG_LR];			\
	(reg)->pc = (uc)->uc_mcontext.__gregs[_REG_PC];			\
	(reg)->ctr = (uc)->uc_mcontext.__gregs[_REG_CTR];		\
	(reg)->xer = (uc)->uc_mcontext.__gregs[_REG_XER];		\
	} while (/*CONSTCOND*/0)

#define PTHREAD_REG_TO_UCONTEXT(uc, reg) do {				\
	memcpy((uc)->uc_mcontext.__gregs, (reg)->fixreg, 32 * 4);	\
	(uc)->uc_mcontext.__gregs[_REG_CR] = (reg)->cr;			\
	(uc)->uc_mcontext.__gregs[_REG_LR] = (reg)->lr;			\
	(uc)->uc_mcontext.__gregs[_REG_PC] = (reg)->pc;			\
	(uc)->uc_mcontext.__gregs[_REG_CTR] = (reg)->ctr;		\
	(uc)->uc_mcontext.__gregs[_REG_XER] = (reg)->xer;		\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_CPU) & ~_UC_USER;       	\
	} while (/*CONSTCOND*/0)

#define PTHREAD_UCONTEXT_TO_FPREG(freg, uc) do {	       		\
	memcpy((freg)->fpreg, (uc)->uc_mcontext.__fpregs.__fpu_regs,	\
		32 * 4);	       					\
	(freg)->fpscr = (uc)->uc_mcontext.__fpregs.__fpu_fpscr;		\
	} while (/*CONSTCOND*/0)

#define PTHREAD_FPREG_TO_UCONTEXT(uc, freg) do {	       		\
	memcpy((uc)->uc_mcontext.__fpregs.__fpu_regs, (freg)->fpreg,	\
		32 * 4);						\
	(uc)->uc_mcontext.__fpregs.__fpu_fpscr = (freg)->fpscr;		\
	(uc)->uc_flags = ((uc)->uc_flags | _UC_FPU) & ~_UC_USER;       	\
	} while (/*CONSTCOND*/0)

#define PTHREAD_UCONTEXT_XREG_FLAG	_UC_POWERPC_VEC

#define PTHREAD_UCONTEXT_TO_XREG(xreg, uc) do {				\
	memcpy(((struct vreg *)(xreg))->vreg,				\
		(uc)->uc_mcontext.__vrf.__vrs,				\
		16 * _NVR);						\
	((struct vreg *)(xreg))->vscr = (uc)->uc_mcontext.__vrf.__vscr;	\
	((struct vreg *)(xreg))->vrsave = (uc)->uc_mcontext.__vrf.__vrsave; \
	} while (/*CONSTCOND*/0)
	
#define PTHREAD_XREG_TO_UCONTEXT(uc, xreg) do {				\
	memcpy((uc)->uc_mcontext.__vrf.__vrs,				\
		((struct vreg *)(xreg))->vreg,				\
		16 * _NVR);						\
	(uc)->uc_mcontext.__vrf.__vscr = ((struct vreg *)(xreg))->vscr;	\
	(uc)->uc_mcontext.__vrf.__vrsave = ((struct vreg *)(xreg))->vrsave; \
	(uc)->uc_flags = ((uc)->uc_flags | _UC_POWERPC_VEC) & ~_UC_USER; \
	} while (/*CONSTCOND*/0)

#endif /* _LIB_PTHREAD_POWERPC_MD_H */
