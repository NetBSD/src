/*	$NetBSD: aout_exec.c,v 1.22 2004/03/25 16:54:10 drochner Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aout_exec.c,v 1.22 2004/03/25 16:54:10 drochner Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/signalvar.h>

#include <compat/aout/aout_syscall.h>

extern struct sysent aout_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const aout_syscallnames[];
#endif
#ifdef COMPAT_16
extern char sigcode[], esigcode[];
struct uvm_object *emul_netbsd_aout_object;
#endif
#ifdef __HAVE_SYSCALL_INTERN
void syscall_intern __P((struct proc *));
#else
void syscall __P((void));
#endif

const struct emul emul_netbsd_aout = {
	"netbsd",
	"/emul/aout",
#ifndef __HAVE_MINIMAL_EMUL
	EMUL_HAS_SYS___syscall,
	NULL,
	AOUT_SYS_syscall,
	AOUT_SYS_NSYSENT,
#endif
	aout_sysent,
#ifdef SYSCALL_DEBUG
	aout_syscallnames,
#else
	NULL,
#endif
	sendsig,
	trapsignal,
	NULL,
#ifdef COMPAT_16
	sigcode,
	esigcode,
	&emul_netbsd_aout_object,
#else
	NULL,
	NULL,
	NULL,
#endif
	setregs,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	syscall_intern,
#else
	syscall,
#endif
	NULL,
	NULL,
};
