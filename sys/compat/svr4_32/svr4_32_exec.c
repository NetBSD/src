/*	$NetBSD: svr4_32_exec.c,v 1.2 2001/05/07 09:55:14 manu Exp $	 */

/*-
 * Copyright (c) 1994, 2000 The NetBSD Foundation, Inc.
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

#define	ELFSIZE		32				/* XXX should die */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <machine/svr4_machdep.h>

#include <compat/svr4_32/svr4_32_types.h>
#include <compat/svr4_32/svr4_32_syscall.h>
#include <compat/svr4/svr4_errno.h>
#include <compat/svr4_32/svr4_32_signal.h>

extern char svr4_32_sigcode[], svr4_32_esigcode[];
extern struct sysent svr4_32_sysent[];
extern const char * const svr4_32_syscallnames[];
#ifndef __HAVE_SYSCALL_INTERN
void syscall __P((void));
#endif

const struct emul emul_svr4_32 = {
	"svr4_32",
	"/emul/svr4_32",
#ifndef __HAVE_MINIMAL_EMUL
	EMUL_NO_BSD_ASYNCIO_PIPE | EMUL_NO_SIGIO_ON_READ,
	native_to_svr4_errno,
	SVR4_32_SYS_syscall,
	SVR4_32_SYS_MAXSYSCALL,
#endif
	svr4_32_sysent,
	svr4_32_syscallnames,
	svr4_32_sendsig,
	svr4_32_sigcode,
	svr4_32_esigcode,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	svr4_32_syscall_intern,
#else
	syscall,
#endif
};
