/*	$NetBSD: netbsd32_exec.h,v 1.3.8.3 2000/12/08 09:08:33 bouyer Exp $	*/

/*
 * Copyright (c) 1998 Matthew R. Green
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

#ifndef	_NETBSD32_EXEC_H_
#define	_NETBSD32_EXEC_H_

#include <compat/netbsd32/netbsd32.h>

/* from <sys/exec_aout.h> */
/*
 * Header prepended to each a.out file.
 * only manipulate the a_midmag field via the
 * N_SETMAGIC/N_GET{MAGIC,MID,FLAG} macros below.
 */
struct netbsd32_exec {
	netbsd32_u_long	a_midmag;	/* htonl(flags<<26 | mid<<16 | magic) */
	netbsd32_u_long	a_text;		/* text segment size */
	netbsd32_u_long	a_data;		/* initialized data size */
	netbsd32_u_long	a_bss;		/* uninitialized data size */
	netbsd32_u_long	a_syms;		/* symbol table size */
	netbsd32_u_long	a_entry;	/* entry point */
	netbsd32_u_long	a_trsize;	/* text relocation size */
	netbsd32_u_long	a_drsize;	/* data relocation size */
};

extern const struct emul emul_netbsd32;

#ifdef EXEC_AOUT
int exec_netbsd32_makecmds __P((struct proc *, struct exec_package *));
#endif
#ifdef EXEC_ELF32
int netbsd32_elf32_probe __P((struct proc *, struct exec_package *, void *,
    char *, vaddr_t *));
void *netbsd32_elf32_copyargs __P((struct exec_package *, struct ps_strings *,
	void *, void *));
#endif /* EXEC_ELF32 */

static __inline void *netbsd32_copyargs __P((struct exec_package *,
	struct ps_strings *, void *, void *));

/*
 * We need to copy out all pointers as 32-bit values.
 */
static __inline void *
netbsd32_copyargs(pack, arginfo, stack, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	u_int32_t *cpp = stack;
	u_int32_t dp;
	u_int32_t nullp = 0;
	char *sp;
	size_t len;
	int argc = arginfo->ps_nargvstr;
	int envc = arginfo->ps_nenvstr;

	if (copyout(&argc, cpp++, sizeof(argc)))
		return NULL;

	dp = (u_long) (cpp + argc + envc + 2 + pack->ep_esch->es_arglen);
	sp = argp;

	/* XXX don't copy them out, remap them! */
	arginfo->ps_argvstr = (char **)(u_long)cpp; /* remember location of argv for later */

	for (; --argc >= 0; sp += len, dp += len) {
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, (char *)(u_long)dp, ARG_MAX, &len))
			return NULL;
	}
	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	arginfo->ps_envstr = (char **)(u_long)cpp; /* remember location of envp for later */

	for (; --envc >= 0; sp += len, dp += len) {
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, (char *)(u_long)dp, ARG_MAX, &len))
			return NULL;
	}
	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	return cpp;
}

#endif /* !_NETBSD32_EXEC_H_ */
