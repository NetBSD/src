/*	$NetBSD: sunos_exec.c,v 1.31 2001/05/07 09:55:14 manu Exp $	*/

/*
 * Copyright (c) 1993 Theo de Raadt
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
 * 3. The name of the author may not be used to endorse or promote products
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

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>

#include <compat/sunos/sunos.h>
#include <compat/sunos/sunos_syscall.h>

extern int nsunos_sysent;
extern struct sysent sunos_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const sunos_syscallnames[];
#endif
extern char sunos_sigcode[], sunos_esigcode[];
void syscall __P((void));

struct emul emul_sunos = {
	"sunos",
	"/emul/sunos",
	EMUL_NO_BSD_ASYNCIO_PIPE | EMUL_NO_SIGIO_ON_READ,
	NULL,
	SUNOS_SYS_syscall,
	SUNOS_SYS_MAXSYSCALL,
	sunos_sysent,
#ifdef SYSCALL_DEBUG
	sunos_syscallnames,
#else
	NULL,
#endif
	sunos_sendsig,
	sunos_sigcode,
	sunos_esigcode,
	NULL,
	NULL,
	NULL,
	syscall
};
