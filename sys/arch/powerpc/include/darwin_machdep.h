/*	$NetBSD: darwin_machdep.h,v 1.3 2003/09/07 07:50:31 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

void darwin_fork_child_return(void *);

struct darwin_ppc_exception_state {
	unsigned long dar;
	unsigned long dsisr;
	unsigned long exception;
	unsigned long pad[5];
};

struct darwin_ppc_thread_state {
	unsigned int srr0;
	unsigned int srr1;
	unsigned int gpreg[32];
	unsigned int cr;
	unsigned int xer;
	unsigned int lr;
	unsigned int ctr;
	unsigned int mq;
	unsigned int vrsave;
};

struct darwin_ppc_float_state {
	double  fpregs[32];
	unsigned int fpscr_pad;
	unsigned int fpscr;
};

struct darwin_ppc_vector_state {
	unsigned long vr[32][4];
	unsigned long vscr[4];
	unsigned int pad1[4];
	unsigned int vrvalid;
	unsigned int pad2[7];
};

struct darwin_mcontext {
	struct darwin_ppc_exception_state es;   
	struct darwin_ppc_thread_state ss;
	struct darwin_ppc_float_state fs;
	struct darwin_ppc_vector_state vs;			
};

struct darwin_sigframe {
	int nocopy1[30];
	/* struct darwin_mcontext without the vs field */
	struct darwin__mcontext {
		struct darwin_ppc_exception_state es;
		struct darwin_ppc_thread_state ss;
		struct darwin_ppc_float_state fs;
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
