/*	$NetBSD: mach_exec.c,v 1.1 2001/07/14 02:11:00 christos Exp $	 */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/malloc.h>

#include <sys/syscall.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_exec.h>

extern char sigcode[], esigcode[];
extern struct sysent sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const syscallnames[];
#endif
#ifndef __HAVE_SYSCALL_INTERN
void syscall __P((void));
#else
void mach_syscall_intern __P((struct proc *));
#endif

const struct emul emul_mach = {
	"mach",
	"/emul/mach",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	0,
	SYS_syscall,
	SYS_MAXSYSCALL,
#endif
	sysent,
#ifdef SYSCALL_DEBUG
	syscallnames,
#else
	NULL,
#endif
	sendsig,
	trapsignal,
	sigcode,
	esigcode,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	mach_syscall_intern,
#else
	syscall,
#endif
};

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
void *
exec_mach_copyargs(struct exec_package *pack, struct ps_strings *arginfo,
    void *stack, void *argp)
{
	char path[MAXPATHLEN];
	size_t len;
	size_t zero = 0;

	stack = copyargs(pack, arginfo, stack, argp);
	if (!stack)
		return NULL;

	if (copyout(&zero, stack, sizeof(zero)))
		return NULL;
	stack = (char *)stack + sizeof(zero);

	/* do the really stupid thing */
	if (copyinstr(pack->ep_name, path, MAXPATHLEN, &len))
		return NULL;
	if (copyout(path, stack, len))
		return NULL;
	stack = (char *)stack + len;

	len = len % sizeof(zero);
	if (len) {
		if (copyout(&zero, stack, len))
			return NULL;
		stack = (char *)stack + len;
	}

	if (copyout(&zero, stack, sizeof(zero)))
		return NULL;
	stack = (char *)stack + sizeof(zero);

	return stack;
}

int
exec_mach_probe(char **path) {
	*path = malloc(strlen(emul_mach.e_path) + 1, M_TEMP, M_WAITOK);
	(void)strcpy(*path, emul_mach.e_path);
	return 0;
}
