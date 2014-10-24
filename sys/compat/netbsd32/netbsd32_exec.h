/*	$NetBSD: netbsd32_exec.h,v 1.32 2014/10/24 21:07:55 christos Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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

#ifdef EXEC_AOUT
#include <sys/exec_aout.h>
#endif

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

extern struct emul emul_netbsd32;

#ifdef EXEC_AOUT
int netbsd32_exec_aout_prep_zmagic(struct lwp *, struct exec_package *);
int netbsd32_exec_aout_prep_nmagic(struct lwp *, struct exec_package *);
int netbsd32_exec_aout_prep_omagic(struct lwp *, struct exec_package *);
int exec_netbsd32_makecmds(struct lwp *, struct exec_package *);
#endif
#ifdef EXEC_ELF32
int netbsd32_elf32_probe(struct lwp *, struct exec_package *, void *,
    char *, vaddr_t *);
int netbsd32_elf32_probe_noteless(struct lwp *, struct exec_package *,
    void *, char *, vaddr_t *);
int netbsd32_elf32_copyargs(struct lwp *, struct exec_package *,
    struct ps_strings *, char **, void *);
#endif /* EXEC_ELF32 */

static __inline int netbsd32_copyargs(struct lwp *, struct exec_package *,
    struct ps_strings *, char **, void *);

void netbsd32_setregs (struct lwp *, struct exec_package *, vaddr_t stack);
int netbsd32_sigreturn (struct proc *, void *, register_t *);
void netbsd32_sendsig (const ksiginfo_t *, const sigset_t *);

extern char netbsd32_esigcode[], netbsd32_sigcode[];

/*
 * We need to copy out all pointers as 32-bit values.
 */
static __inline int
netbsd32_copyargs(struct lwp *l, struct exec_package *pack,
		struct ps_strings *arginfo, char **stackp, void *argp)
{
	u_int32_t *cpp = (u_int32_t *)*stackp;
	netbsd32_pointer_t dp;
	u_int32_t nullp = 0;
	char *sp;
	size_t len;
	int argc = arginfo->ps_nargvstr;
	int envc = arginfo->ps_nenvstr;
	int error;

	NETBSD32PTR32(dp, cpp + 
	    1 +				/* int argc */
	    argc +			/* char *argv[] */
	    1 +				/* \0 */
	    envc +			/* char *env[] */
	    1 +				/* \0 */
	    /* XXX auxinfo multiplied by ptr size? */
	    pack->ep_esch->es_arglen);	/* auxinfo */
	sp = argp;

	if ((error = copyout(&argc, cpp++, sizeof(argc))) != 0)
		return error;

	/* XXX don't copy them out, remap them! */
	/* remember location of argv for later */
	arginfo->ps_argvstr = (char **)(u_long)cpp;

	for (; --argc >= 0; sp += len, NETBSD32PTR32PLUS(dp, len)) {
		if ((error = copyout(&dp, cpp++, sizeof(dp))) != 0)
			return error;
		if ((error = copyoutstr(sp, NETBSD32PTR64(dp),
		    ARG_MAX, &len)) != 0)
			return error;
	}
	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0)
		return error;

	/* remember location of envp for later */
	arginfo->ps_envstr = (char **)(u_long)cpp;

	for (; --envc >= 0; sp += len, NETBSD32PTR32PLUS(dp, len)) {
		if ((error = copyout(&dp, cpp++, sizeof(dp))) != 0)
			return error;
		if ((error = copyoutstr(sp, NETBSD32PTR64(dp),
		    ARG_MAX, &len)) != 0)
			return error;
	}
	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0)
		return error;

	*stackp = (char *)cpp;
	return 0;
}

#endif /* !_NETBSD32_EXEC_H_ */
