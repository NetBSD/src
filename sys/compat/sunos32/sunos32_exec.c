/*	$NetBSD: sunos32_exec.c,v 1.15 2003/08/24 17:52:45 chs Exp $	 */

/*
 * Copyright (c) 2001 Matthew R. Green
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
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sunos32_exec.c,v 1.15 2003/08/24 17:52:45 chs Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/mount.h>

#include <compat/sunos32/sunos32.h>
#include <compat/sunos32/sunos32_syscall.h>
#include <compat/sunos32/sunos32_exec.h>

extern int nsunos32_sysent;
extern struct sysent sunos32_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const sunos32_syscallnames[];
#endif
extern char sunos_sigcode[], sunos_esigcode[];
void syscall __P((void));

struct uvm_object *emul_sunos32_object;

const struct emul emul_sunos = {
	"sunos32",
	"/emul/sunos",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	NULL,
	SUNOS32_SYS_syscall,
	SUNOS32_SYS_NSYSENT,
#endif
	sunos32_sysent,
#ifdef SYSCALL_DEBUG
	sunos32_syscallnames,
#else
	NULL,
#endif
	sunos32_sendsig,
	trapsignal,
	sunos_sigcode,
	sunos_esigcode,
	&emul_sunos32_object,
	sunos32_setregs,
	NULL,
	NULL,
	NULL,
	syscall,
	NULL,
	NULL,
};
