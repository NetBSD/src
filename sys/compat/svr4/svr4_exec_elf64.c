/*	$NetBSD: svr4_exec_elf64.c,v 1.1.6.1 2001/08/03 04:12:48 lukem Exp $	 */

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

#define	ELFSIZE		64				/* XXX should die */

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

#include <machine/cpu.h>
#include <machine/reg.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_exec.h>
#include <compat/svr4/svr4_errno.h>

int
svr4_copyargs64(pack, arginfo, stackp, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	char **stackp;
	void *argp;
{
	AuxInfo *a;
	int error;

	if ((error = elf64_copyargs(pack, arginfo, stackp, argp)) != 0)
		return error;

	a = (AuxInfo *)*stackp;
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
	*stackp = (char *)a;
	return 0;
}

int
svr4_elf64_probe(p, epp, eh, itp, pos)
	struct proc *p;
	struct exec_package *epp;
	void *eh;
	char *itp;
	vaddr_t *pos;
{
	const char *bp;
	int error;
	size_t len;

	if (itp[0]) {
		if ((error = emul_find(p, NULL, epp->ep_esch->es_emul->e_path, itp, &bp, 0)))
			return error;
		if ((error = copystr(bp, itp, MAXPATHLEN, &len)))
			return error;
		free((void *)bp, M_TEMP);
	}
	*pos = SVR4_INTERP_ADDR;
	return 0;
}
