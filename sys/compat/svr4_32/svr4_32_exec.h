/*	$NetBSD: svr4_32_exec.h,v 1.2.6.1 2001/08/03 04:12:49 lukem Exp $	 */

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

#ifndef	_SVR4_32_EXEC_H_
#define	_SVR4_32_EXEC_H_

#undef SVR4_COMPAT_SOLARIS2
#ifdef SVR4_COMPAT_SOLARIS2
# define SVR4_32_AUX_ARGSIZ howmany((sizeof(Aux32Info) * 15) + 256, \
				sizeof(netbsd32_charp))
#else
# define SVR4_32_AUX_ARGSIZ howmany(sizeof(Aux32Info) * 8, sizeof(netbsd32_charp))
#endif

int svr4_32_copyargs __P((struct exec_package *, struct ps_strings *,
    char **, void *));

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

#ifndef SVR4_32_INTERP_ADDR
# define SVR4_32_INTERP_ADDR	ELFDEFNNAME(NO_ADDR)
#endif

extern const struct emul emul_svr4_32;

void svr4_32_setregs __P((struct proc *, struct exec_package *, u_long));
int svr4_32_elf32_probe __P((struct proc *, struct exec_package *, void *,
    char *, vaddr_t *));

#endif /* !_SVR4_32_EXEC_H_ */
