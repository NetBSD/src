/*	$NetBSD: freebsd_exec.c,v 1.4.8.4 2000/12/13 15:49:41 bouyer Exp $	*/

/*
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <machine/freebsd_machdep.h>

#include <compat/freebsd/freebsd_syscall.h>
#include <compat/freebsd/freebsd_exec.h>
#include <compat/common/compat_util.h>

extern struct sysent freebsd_sysent[];
extern const char * const freebsd_syscallnames[];
#ifdef __HAVE_SYSCALL_INTERN
void syscall_intern __P((struct proc *));
#else
void syscall __P((void));
#endif

const struct emul emul_freebsd = {
	"freebsd",
	"/emul/freebsd",
#ifndef __HAVE_MINIMAL_EMUL
	EMUL_HAS_SYS___syscall,
	NULL,
	FREEBSD_SYS_syscall,
	FREEBSD_SYS_MAXSYSCALL,
#endif
	freebsd_sysent,
	freebsd_syscallnames,
	freebsd_sendsig,
	freebsd_sigcode,
	freebsd_esigcode,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	syscall_intern,
#else
	syscall,
#endif
};
