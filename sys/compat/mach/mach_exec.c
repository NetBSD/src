/*	$NetBSD: mach_exec.c,v 1.2.4.3 2001/09/21 22:35:20 nathanw Exp $	 */

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
	setregs,
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
int
exec_mach_copyargs(struct exec_package *pack, struct ps_strings *arginfo,
    char **stackp, void *argp)
{
	char path[MAXPATHLEN];
	size_t len;
	size_t zero = 0;
	int error;

	if ((error = copyargs(pack, arginfo, stackp, argp)) != 0) {
		DPRINTF(("mach: copyargs failed\n"));
		return error;
	}

	if ((error = copyout(&zero, *stackp, sizeof(zero))) != 0) {
		DPRINTF(("mach: copyout first zero failed\n"));
		return error;
	}
	*stackp += sizeof(zero);

	/* do the really stupid thing */
	if ((error = copyinstr(pack->ep_name, path, MAXPATHLEN, &len)) != 0) {
		DPRINTF(("mach: copyinstr %p failed\n", pack->ep_name));
		return error;
	}
	if ((error = copyout(path, *stackp, len)) != 0) {
		DPRINTF(("mach: copyout path failed\n"));
		return error;
	}
	*stackp += len;

	len = len % sizeof(zero);
	if (len) {
		if ((error = copyout(&zero, *stackp, len)) != 0) {
			DPRINTF(("mach: zero align %d failed\n", len));
			return error;
		}
		*stackp += len;
	}

	if ((error = copyout(&zero, *stackp, sizeof(zero))) != 0) {
		DPRINTF(("mach: copyout second zero failed\n"));
		return error;
	}
	*stackp += sizeof(zero);

	return 0;
}

int
exec_mach_probe(char **path) {
	*path = malloc(strlen(emul_mach.e_path) + 1, M_TEMP, M_WAITOK);
	(void)strcpy(*path, emul_mach.e_path);
	return 0;
}
