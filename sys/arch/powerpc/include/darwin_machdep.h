/*	$NetBSD: darwin_machdep.h,v 1.2.8.3 2004/09/21 13:20:41 skrll Exp $ */

/*-
 * Copyright (c) 2002-2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#ifndef	_DARWIN_MACHDEP_H_
#define	_DARWIN_MACHDEP_H_

#include <compat/darwin/darwin_signal.h>
#include <machine/mach_machdep.h>

#define DARWIN_USRSTACK		0xbfff0000
#define DARWIN_USRSTACK32	0x00000000bfff0000L

/*
 * User context versions for newer sigreturn
 */
#define DARWIN_UCVERS_X2 1

void darwin_fork_child_return(void *);

struct darwin_mcontext {
	struct mach_ppc_exception_state es;   
	struct mach_ppc_thread_state ss;
	struct mach_ppc_float_state fs;
	struct mach_ppc_vector_state vs;			
};

struct darwin_sigframe {
	int nocopy1[30];
	/* struct darwin_mcontext without the vs field */
	struct darwin__mcontext {
		struct mach_ppc_exception_state es;
		struct mach_ppc_thread_state ss;
		struct mach_ppc_float_state fs;
	} dmc;
	int nocopy2[144];
	/* struct darwin_ucontext with some padding */
	struct darwin__ucontext {
		darwin_siginfo_t si;
		struct darwin_ucontext uctx;
	} duc;
	int nocopy3[56];
};

struct darwin_slock {
	volatile unsigned int lock_data[10];
};

#endif /* !_DARWIN_MACHDEP_H_ */
