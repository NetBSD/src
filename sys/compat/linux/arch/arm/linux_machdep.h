/*	$NetBSD: linux_machdep.h,v 1.3 2002/02/15 16:47:59 christos Exp $	*/

/*-
 * Copyright (c) 1995, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _ARM_LINUX_MACHDEP_H_
#define _ARM_LINUX_MACHDEP_H_

#include <compat/linux/common/linux_signal.h>

struct linux_sigcontext {
	u_int32_t	sc_trapno;
	u_int32_t	sc_error_code;
	linux_old_sigset_t	sc_mask;
	u_int32_t	sc_r0;
	u_int32_t	sc_r1;
	u_int32_t	sc_r2;
	u_int32_t	sc_r3;
	u_int32_t	sc_r4;
	u_int32_t	sc_r5;
	u_int32_t	sc_r6;
	u_int32_t	sc_r7;
	u_int32_t	sc_r8;
	u_int32_t	sc_r9;
	u_int32_t	sc_r10;
	u_int32_t	sc_r11;
	u_int32_t	sc_r12;
	u_int32_t	sc_sp;
	u_int32_t	sc_lr;
	u_int32_t	sc_pc;
	u_int32_t	sc_cpsr;
	u_int32_t	sc_fault_address;
};

/*
 * We make the stack look like Linux expects it when calling a signal
 * handler, but use the BSD way of calling the handler and sigreturn().
 * This means that we need to pass the pointer to the handler too.
 * It is appended to the frame to not interfere with the rest of it.
 */

struct linux_sigframe {
	struct	linux_sigcontext sf_sc;
	unsigned long	sf_extramask[LINUX__NSIG_WORDS - 1];
};

#endif
