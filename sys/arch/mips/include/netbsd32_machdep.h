/*	$NetBSD: netbsd32_machdep.h,v 1.2 2009/12/14 00:46:05 matt Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _MACHINE_NETBSD32_H_
#define _MACHINE_NETBSD32_H_

#include <sys/types.h>

/*
 * On MIPS, pointers are signed.
 */
#define	NETBSD32_POINTER_TYPE			int32_t
typedef struct { NETBSD32_POINTER_TYPE i32; }	netbsd32_pointer_t;

#define NETBSD32_INT64_ALIGN

typedef netbsd32_pointer_t			netbsd32_sigcontextp_t;

/*
 * The sigcode is ABI neutral.
 */
#define	netbsd32_sigcode			sigcode
#define	netbsd32_esigcode			esigcode

/*
 * cpu_upcall knows about COMPAT_NETBSD32
 * syscall_intern and setregs don't about COMPAT_NETBSD32.
 */
#define	netbsd32_syscall_intern			syscall_intern
#define	netbsd32_setregs			setregs
#define	netbsd32_cpu_upcall			cpu_upcall

/* <mips/sysarch.h> */
struct mips_cacheflush_args32 {
	netbsd32_intptr_t va;
	netbsd32_size_t nbytes;
	int whichcache;
};

struct mips_cachectl_args32 {
	netbsd32_intptr_t va;
	netbsd32_size_t nbytes;
	int ctl;
};

/* <mips/frame.h> */
struct saframe32 {
	/* first 4 arguments passed in registers on entry to upcallcode */
	int			sa_type;	/* A0 */
	netbsd32_pointer_t	sa_sas;		/* A1 */
	int			sa_events;	/* A2 */
	int			sa_interrupted;	/* A3 */
	netbsd32_pointer_t 	sa_arg;
	netbsd32_pointer_t	sa_upcall;
};

#endif /* _MACHINE_NETBSD32_H_ */
