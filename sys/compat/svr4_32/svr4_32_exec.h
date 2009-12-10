/*	$NetBSD: svr4_32_exec.h,v 1.14 2009/12/10 14:13:54 matt Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#ifndef	_SVR4_32_EXEC_H_
#define	_SVR4_32_EXEC_H_

#undef SVR4_COMPAT_SOLARIS2
#ifdef SVR4_COMPAT_SOLARIS2
# define SVR4_32_AUX_ARGSIZ howmany((sizeof(Aux32Info) * 15) + 256, \
				sizeof(netbsd32_charp))
#else
# define SVR4_32_AUX_ARGSIZ howmany(sizeof(Aux32Info) * 8, sizeof(netbsd32_charp))
#endif

int svr4_32_copyargs(struct lwp *, struct exec_package *, struct ps_strings *,
    char **, void *);

/*
 * The following is horrible; there must be a better way. I need to
 * play with brk(2) a bit more.
 */

#if defined(__sparc__) || defined(__sparc_v9__) || defined(__sparc64__)
/*
 * Here programs load at 0x00010000, so I load the interpreter far after
 * the end of the data segment.
 */
#if 1
#define SVR4_32_INTERP_ADDR	0x10000000
#else
#define SVR4_32_INTERP_ADDR	0xff3c0000U
#endif
#endif

extern struct emul emul_svr4_32;

void svr4_32_setregs(struct lwp *, struct exec_package *, vaddr_t);
vaddr_t svr4_32_vm_default_addr(struct proc *, vaddr_t, vsize_t);
int svr4_32_elf32_probe(struct lwp *, struct exec_package *, void *,
    char *, vaddr_t *);

#endif /* !_SVR4_32_EXEC_H_ */
