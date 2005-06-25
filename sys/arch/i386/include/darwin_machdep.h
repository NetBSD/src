/*	$NetBSD: darwin_machdep.h,v 1.4 2005/06/25 06:29:16 christos Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas
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

void darwin_fork_child_return(void *);

struct darwin_i386_sigcontext {
	int		sc_onstack;
	int		sc_mask;
	unsigned int	sc_eax;
	unsigned int	sc_ebx;
	unsigned int	sc_ecx;
	unsigned int	sc_edx;
	unsigned int	sc_edi;
	unsigned int	sc_esi;
	unsigned int	sc_ebp;
	unsigned int	sc_esp;
	unsigned int	sc_ss;
	unsigned int	sc_eflags;
	unsigned int	sc_eip;
	unsigned int	sc_cs;
	unsigned int	sc_ds;
	unsigned int	sc_es;
	unsigned int	sc_fs;
	unsigned int	sc_gs;
};


struct darwin_sigframe {
	int	retaddr;
	sig_t	catcher;
	int	sigstyle;
	int	sig;
	int	code;
	struct darwin_i386_sigcontext * scp;
};

struct darwin_slock {
	int	dummy;
};

/* XXX probably wrong */
#define DARWIN_USRSTACK      USRSTACK
#define DARWIN_USRSTACK32    USRSTACK

/*
 * Commpage stuff.
 */
#define DARWIN_COMMPAGE_BASE 0xbfff9000
#define DARWIN_COMMPAGE_LEN  0x00007000	/* 7 pages */

#define DARWIN_COMMPAGE_VERSION 1

#define DARWIN_CAP_MMX		0x00000001
#define DARWIN_CAP_SSE		0x00000002
#define DARWIN_CAP_SSE2		0x00000004
#define DARWIN_CAP_PENTIUM4	0x00000008
#define DARWIN_CAP_CACHE32	0x00000010
#define DARWIN_CAP_CACHE64	0x00000020
#define DARWIN_CAP_CACHE128	0x00000040
#define DARWIN_CAP_UP		0x00008000
#define DARWIN_CAP_NCPUMASK	0x00ff0000
#define DARWIN_CAP_NCPUSHIFT	16

#endif /* !_DARWIN_MACHDEP_H_ */
