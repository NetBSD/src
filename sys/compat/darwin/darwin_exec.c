/*	$NetBSD: darwin_exec.c,v 1.4 2002/11/23 17:35:06 wiz Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: darwin_exec.c,v 1.4 2002/11/23 17:35:06 wiz Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/syscall.h>
#include <sys/exec_macho.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_param.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_exec.h>

#include <compat/darwin/darwin_exec.h>
#include <compat/darwin/darwin_syscall.h>

extern char sigcode[], esigcode[];
extern struct sysent darwin_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const darwin_syscallnames[];
#endif
#ifndef __HAVE_SYSCALL_INTERN
void syscall __P((void));
#else
void mach_syscall_intern __P((struct proc *));
#endif

const struct emul emul_darwin = {
	"darwin",
	"/emul/darwin",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	0,
	DARWIN_SYS_syscall,
	DARWIN_SYS_NSYSENT,
#endif
	darwin_sysent,
#ifdef SYSCALL_DEBUG
	darwin_syscallnames,
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
	NULL,
	NULL,
};

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
int
exec_darwin_copyargs(struct proc *p, struct exec_package *pack,
    struct ps_strings *arginfo, char **stackp, void *argp)
{
	struct exec_macho_emul_arg *emea;
	struct exec_macho_object_header *macho_hdr;
	char **cpp, *dp, *sp, *progname;
	size_t len;
	void *nullp = NULL;
	long argc, envc;
	int error;

	*stackp = (char *)(((unsigned long)*stackp - 1) & ~0xfUL);

	emea = (struct exec_macho_emul_arg *)pack->ep_emul_arg;
	macho_hdr = (struct exec_macho_object_header *)emea->macho_hdr;
	if ((error = copyout(&macho_hdr, *stackp, sizeof(macho_hdr))) != 0)
		return error;
	*stackp += sizeof(macho_hdr);

	cpp = (char **)*stackp;
	argc = arginfo->ps_nargvstr;
	envc = arginfo->ps_nenvstr;
	if ((error = copyout(&argc, cpp++, sizeof(argc))) != 0)
		return error;

	dp = (char *) (cpp + argc + envc + 4);

	if ((error = copyoutstr(emea->filename, dp, ARG_MAX, &len)) != 0)
		return error;
	progname = dp;
	dp += len;

	sp = argp;
	arginfo->ps_argvstr = cpp; /* remember location of argv for later */
	for (; --argc >= 0; sp += len, dp += len)
		if ((error = copyout(&dp, cpp++, sizeof(dp))) != 0 ||
		    (error = copyoutstr(sp, dp, ARG_MAX, &len)) != 0)
			return error;

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0)
		return error;

	arginfo->ps_envstr = cpp; /* remember location of envp for later */
	for (; --envc >= 0; sp += len, dp += len)
		if ((error = copyout(&dp, cpp++, sizeof(dp))) != 0 ||
		    (error = copyoutstr(sp, dp, ARG_MAX, &len)) != 0)
			return error;

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0)
		return error;

	if ((error = copyout(&progname, cpp++, sizeof(progname))) != 0)
		return error;

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0)
		return error;

	*stackp = (char *)cpp;

	/* We don't need this anymore */
	free(pack->ep_emul_arg, M_EXEC);
	pack->ep_emul_arg = NULL;

	return 0;
}

int
exec_darwin_probe(char **path) {
	*path = (char *)emul_darwin.e_path;
	return 0;
}
