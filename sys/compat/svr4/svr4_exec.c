/*	$NetBSD: svr4_exec.c,v 1.29.10.1 1999/11/15 00:40:06 fvdl Exp $	 */

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

#define	ELFSIZE		32				/* XXX should die */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/exec_elf.h>
#include <sys/exec.h>

#include <sys/mman.h>
#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_map.h>

#include <machine/cpu.h>
#include <machine/reg.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_syscall.h>
#include <compat/svr4/svr4_exec.h>
#include <compat/svr4/svr4_errno.h>
#include <compat/svr4/svr4_signal.h>

static void *svr4_copyargs __P((struct exec_package *, struct ps_strings *,
			       void *, void *));

const char svr4_emul_path[] = "/emul/svr4";
extern char svr4_sigcode[], svr4_esigcode[];
extern struct sysent svr4_sysent[];
extern char *svr4_syscallnames[];

struct emul emul_svr4 = {
	"svr4",
	native_to_svr4_errno,
	svr4_sendsig,
	SVR4_SYS_syscall,
	SVR4_SYS_MAXSYSCALL,
	svr4_sysent,
	svr4_syscallnames,
	SVR4_AUX_ARGSIZ,
	svr4_copyargs,
	svr4_setregs,
	svr4_sigcode,
	svr4_esigcode,
};

static void *
svr4_copyargs(pack, arginfo, stack, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	AuxInfo *a;

	if (!(a = (AuxInfo *) elf32_copyargs(pack, arginfo, stack, argp)))
		return NULL;
#ifdef SVR4_COMPAT_SOLARIS2
	if (pack->ep_emul_arg) {
		a->au_type = AT_SUN_UID;
		a->au_v = p->p_ucred->cr_uid;
		a++;

		a->au_type = AT_SUN_RUID;
		a->au_v = p->p_cred->ruid;
		a++;

		a->au_type = AT_SUN_GID;
		a->au_v = p->p_ucred->cr_gid;
		a++;

		a->au_type = AT_SUN_RGID;
		a->au_v = p->p_cred->rgid;
		a++;
	}
#endif
	return a;
}

int
svr4_elf32_probe(p, epp, eh, itp, pos)
	struct proc *p;
	struct exec_package *epp;
	Elf32_Ehdr *eh;
	char *itp;
	Elf32_Addr *pos;
{
	const char *bp;
	int error;
	size_t len;

	if (itp[0]) {
		if ((error = emul_find(p, NULL, svr4_emul_path, itp, &bp, 0)))
			return error;
		if ((error = copystr(bp, itp, MAXPATHLEN, &len)))
			return error;
		free((void *)bp, M_TEMP);
	}
	epp->ep_emul = &emul_svr4;
	*pos = SVR4_INTERP_ADDR;
	return 0;
}
