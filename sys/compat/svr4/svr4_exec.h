/*	$NetBSD: svr4_exec.h,v 1.28 2009/12/10 14:13:53 matt Exp $	 */

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

#ifndef	_SVR4_EXEC_H_
#define	_SVR4_EXEC_H_

/*
 * The following is horrible; there must be a better way. I need to
 * play with brk(2) a bit more.
 */

#ifdef __i386__
/*
 * I cannot load the interpreter after the data segment because brk(2)
 * breaks. I have to load it somewhere before. Programs start at
 * 0x08000000 so I load the interpreter far before.
 */
#define SVR4_INTERP_ADDR	0x01000000
#endif

#ifdef __m68k__
/*
 * Here programs load at 0x80000000, so I load the interpreter far before.
 */
#define SVR4_INTERP_ADDR	0x01000000
#endif

#ifdef __sparc__
/*
 * Here programs load at 0x00010000, so I load the interpreter far after
 * the end of the data segment.
 */
#define SVR4_INTERP_ADDR	0x10000000
#endif

extern struct emul emul_svr4;

void svr4_setregs(struct lwp *, struct exec_package *, vaddr_t);
int svr4_elf32_probe(struct lwp *, struct exec_package *, void *,
    char *, vaddr_t *);
int svr4_elf64_probe(struct lwp *, struct exec_package *, void *,
    char *, vaddr_t *);

#endif /* !_SVR4_EXEC_H_ */
