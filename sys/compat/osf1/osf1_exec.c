/* $NetBSD: osf1_exec.c,v 1.2 1999/04/27 01:45:03 cgd Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_ecoff.h>
#include <sys/signalvar.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_exec.h>
#include <compat/osf1/osf1_syscall.h>

struct osf1_exec_emul_arg {
	int	flags;
#define	OSF1_EXEC_EMUL_FLAGS_HAVE_LOADER	0x01

	char	exec_name[MAXPATHLEN+1];
	char	loader_name[MAXPATHLEN+1];
};

static void *osf1_copyargs(struct exec_package *pack,
    struct ps_strings *arginfo, void *stack, void *argp);

static int osf1_exec_ecoff_dynamic(struct proc *p, struct exec_package *epp);

#define	MAX_AUX_ENTRIES		4	/* max we'll ever push (right now) */

#define	COMPAT_OSF1_EMUL_LOADER_NAME					\
    "/emul/osf1" OSF1_LDR_EXEC_DEFAULT_LOADER


extern struct sysent osf1_sysent[];
extern char *osf1_syscallnames[];
extern void cpu_exec_ecoff_setregs __P((struct proc *, struct exec_package *,
					u_long));
extern char sigcode[], esigcode[];

struct emul emul_osf1 = {
	"osf1",
	netbsd_to_osf1_errno,
	sendsig,
	OSF1_SYS_syscall,
	OSF1_SYS_MAXSYSCALL,
	osf1_sysent,
	osf1_syscallnames,
	MAX_AUX_ENTRIES * sizeof (struct osf1_auxv) +
	    2 * (MAXPATHLEN + 1),		/* exec name & loader name */
	osf1_copyargs,
	cpu_exec_ecoff_setregs,
	sigcode,
	esigcode,
};

int
osf1_exec_ecoff_hook(struct proc *p, struct exec_package *epp)
{
        struct ecoff_exechdr *execp = (struct ecoff_exechdr *)epp->ep_hdr;
	struct osf1_exec_emul_arg *emul_arg;
	int error;

	/* if we're here and we're exec-able at all, we're an OSF/1 binary */
	epp->ep_emul = &emul_osf1;

	/* set up the exec package emul arg as appropriate */
	emul_arg = malloc(sizeof *emul_arg, M_TEMP, M_WAITOK);
	epp->ep_emul_arg = emul_arg;

	emul_arg->flags = 0;
	/* XXX? includes /emul/osf1 if appropriate */
	strncpy(emul_arg->exec_name, epp->ep_ndp->ni_cnd.cn_pnbuf,
	    MAXPATHLEN + 1);

	/* do any special object file handling */
	switch (execp->f.f_flags & ECOFF_FLAG_OBJECT_TYPE_MASK) {
	case OBJECT_TYPE_SHARABLE:
		/* can't exec a shared library! */
#if 1
		uprintf("can't execute OSF/1 shared libraries\n");
#endif
		error = ENOEXEC;
		break;

	case OBJECT_TYPE_CALL_SHARED:
		error = osf1_exec_ecoff_dynamic(p, epp);
		break;

	default:
		/* just let the normal ECOFF handlers deal with it. */
		error = 0;
		break;
	}
	return (error);
}

static int
osf1_exec_ecoff_dynamic(struct proc *p, struct exec_package *epp)
{
#if 1
	uprintf("OSF/1 dynamically linked binaries not yet supported\n");
	return ENOEXEC;
#else
	not yet implemented.  DUH!
#endif
}

/*
 * copy arguments onto the stack in the normal way, then copy out
 * any ELF-like AUX entries used by the dynamic loading scheme.
 */
static void *
osf1_copyargs(pack, arginfo, stack, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	struct proc *p = curproc;			/* XXX !!! */
	struct osf1_exec_emul_arg *emul_arg = pack->ep_emul_arg;
	struct osf1_auxv ai[MAX_AUX_ENTRIES], *a;
	char *prognameloc, *loadernameloc;
	size_t len;

	stack = copyargs(pack, arginfo, stack, argp);
	if (!stack)
		goto bad;

	a = ai;
	memset(ai, 0, sizeof ai);

	prognameloc = (char *)stack + sizeof ai;
	if (copyoutstr(emul_arg->exec_name, prognameloc, MAXPATHLEN + 1, NULL))
	    goto bad;
	a->a_type = OSF1_AT_EXEC_FILENAME;
	a->a_un.a_ptr = prognameloc;
	a++;

	/*
	 * if there's a loader, push additional auxv entries on the stack.
	 */
	if (emul_arg->flags & OSF1_EXEC_EMUL_FLAGS_HAVE_LOADER) {

		loadernameloc = prognameloc + MAXPATHLEN + 1;
		if (copyoutstr(emul_arg->loader_name, loadernameloc,
		    MAXPATHLEN + 1, NULL))
			goto bad;
		a->a_type = OSF1_AT_EXEC_LOADER_FILENAME;
		a->a_un.a_ptr = loadernameloc;
		a++;

		a->a_type = OSF1_AT_EXEC_LOADER_FLAGS;
		a->a_un.a_val = 0;
                if (pack->ep_vap->va_mode & S_ISUID)
                        a->a_un.a_val |= OSF1_LDR_EXEC_SETUID_F;
                if (pack->ep_vap->va_mode & S_ISGID)
                        a->a_un.a_val |= OSF1_LDR_EXEC_SETGID_F;
	        if (p->p_flag & P_TRACED)
                        a->a_un.a_val |= OSF1_LDR_EXEC_PTRACE_F;
		a++;
	}

	a->a_type = OSF1_AT_NULL;
	a->a_un.a_val = 0;
	a++;

	len = (a - ai) * sizeof(struct osf1_auxv);
	if (copyout(ai, stack, len))
		goto bad;
	stack = (caddr_t)stack + len;

out:
	FREE(pack->ep_emul_arg, M_TEMP);
	pack->ep_emul_arg = NULL;
	return stack;

bad:
	stack = NULL;
	goto out;
}
