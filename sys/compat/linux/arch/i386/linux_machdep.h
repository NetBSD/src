/*	$NetBSD: linux_machdep.h,v 1.2 1995/04/22 20:27:03 christos Exp $	*/

/*
 * Copyright (c) 1995 Frank van der Linden
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
 *      This product includes software developed for the NetBSD Project
 *      by Frank van der Linden
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX_MACHDEP_H
#define _LINUX_MACHDEP_H

/*
 * The Linux sigcontext, pretty much a standard 386 trapframe.
 */

struct linux_sigcontext {
	int	lsc_gs;
	int	lsc_fs;
	int     lsc_es;
	int     lsc_ds;
	int     lsc_edi;
	int     lsc_esi;
	int     lsc_ebp;
	int     lsc_ebx;
	int     lsc_edx;
	int     lsc_ecx;
	int     lsc_eax;
	int     lsc_trapno;
	int     lsc_err;
	int     lsc_eip;
	int     lsc_cs;
	int     lsc_eflags;
	int     lsc_esp;
	int     lsc_ss;
	int	lsc_387;
	int	lsc_mask;
	int	lsc_cr2;
};

/*
 * We make the stack look like Linux expects it when calling a signal
 * handler, but use the BSD way of calling the handler and sigreturn().
 * This means that we need to pass the pointer to the handler too.
 * It is appended to the frame to not interfere with the rest of it.
 */

struct linux_sigframe {
	int	ls_sig;
	struct linux_sigcontext ls_sc;
	sig_t	ls_handler;
};

void linux_sendsig __P((sig_t, int, int, u_long));

#endif /* _LINUX_MACHDEP_H */
