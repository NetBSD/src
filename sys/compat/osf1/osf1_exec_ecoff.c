/* $NetBSD: osf1_exec_ecoff.c,v 1.13 2006/05/14 21:24:50 elad Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osf1_exec_ecoff.c,v 1.13 2006/05/14 21:24:50 elad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_ecoff.h>

#include <compat/osf1/osf1.h>
#include <compat/common/compat_util.h>
#include <compat/osf1/osf1_cvt.h>
#include <compat/osf1/osf1_exec.h>

struct osf1_exec_emul_arg {
	int	flags;
#define	OSF1_EXEC_EMUL_FLAGS_HAVE_LOADER	0x01

	char	exec_name[MAXPATHLEN+1];
	char	loader_name[MAXPATHLEN+1];
};

static int osf1_exec_ecoff_dynamic(struct lwp *l, struct exec_package *epp);

int
osf1_exec_ecoff_probe(struct lwp *l, struct exec_package *epp)
{
        struct ecoff_exechdr *execp = (struct ecoff_exechdr *)epp->ep_hdr;
	struct osf1_exec_emul_arg *emul_arg;
	int error;

	/* check if the binary is osf1 ecoff */
	if (execp->f.f_magic != ECOFF_MAGIC_ALPHA)
		return ENOEXEC;

	/* set up the exec package emul arg as appropriate */
	emul_arg = malloc(sizeof *emul_arg, M_TEMP, M_WAITOK);
	epp->ep_emul_arg = emul_arg;

	emul_arg->flags = 0;
	if (epp->ep_ndp->ni_segflg == UIO_SYSSPACE)
		error = copystr(epp->ep_ndp->ni_dirp, emul_arg->exec_name,
		    MAXPATHLEN + 1, NULL);
	else
		error = copyinstr(epp->ep_ndp->ni_dirp, emul_arg->exec_name,
		    MAXPATHLEN + 1, NULL);
#ifdef DIAGNOSTIC
	if (error != 0)
		panic("osf1_exec_ecoff_probe: copyinstr failed");
#endif

	/* do any special object file handling */
	switch (execp->f.f_flags & ECOFF_FLAG_OBJECT_TYPE_MASK) {
	case ECOFF_OBJECT_TYPE_SHARABLE:
		/* can't exec a shared library! */
#if 0
		uprintf("can't execute OSF/1 shared libraries\n");
#endif
		error = ENOEXEC;
		break;

	case ECOFF_OBJECT_TYPE_CALL_SHARED:
		error = osf1_exec_ecoff_dynamic(l, epp);
		break;

	default:
		/* just let the normal ECOFF handlers deal with it. */
		error = 0;
		break;
	}

	if (error) {
		free(epp->ep_emul_arg, M_TEMP);
		epp->ep_emul_arg = NULL;
		kill_vmcmds(&epp->ep_vmcmds);		/* if any */
	}

	return (error);
}

/*
 * copy arguments onto the stack in the normal way, then copy out
 * any ELF-like AUX entries used by the dynamic loading scheme.
 */
int
osf1_copyargs(l, pack, arginfo, stackp, argp)
	struct lwp *l;
	struct exec_package *pack;
	struct ps_strings *arginfo;
	char **stackp;
	void *argp;
{
	struct osf1_exec_emul_arg *emul_arg = pack->ep_emul_arg;
	struct osf1_auxv ai[OSF1_MAX_AUX_ENTRIES], *a;
	char *prognameloc, *loadernameloc;
	size_t len;
	int error;

	if ((error = copyargs(l, pack, arginfo, stackp, argp)) != 0)
		goto out;

	a = ai;
	memset(ai, 0, sizeof ai);

	prognameloc = *stackp + sizeof ai;
	if ((error = copyoutstr(emul_arg->exec_name, prognameloc,
	    MAXPATHLEN + 1, NULL)) != 0)
	    goto out;
	a->a_type = OSF1_AT_EXEC_FILENAME;
	a->a_un.a_ptr = prognameloc;
	a++;

	/*
	 * if there's a loader, push additional auxv entries on the stack.
	 */
	if (emul_arg->flags & OSF1_EXEC_EMUL_FLAGS_HAVE_LOADER) {

		loadernameloc = prognameloc + MAXPATHLEN + 1;
		if ((error = copyoutstr(emul_arg->loader_name, loadernameloc,
		    MAXPATHLEN + 1, NULL)) != 0)
			goto out;
		a->a_type = OSF1_AT_EXEC_LOADER_FILENAME;
		a->a_un.a_ptr = loadernameloc;
		a++;

		a->a_type = OSF1_AT_EXEC_LOADER_FLAGS;
		a->a_un.a_val = 0;
                if (pack->ep_vap->va_mode & S_ISUID)
                        a->a_un.a_val |= OSF1_LDR_EXEC_SETUID_F;
                if (pack->ep_vap->va_mode & S_ISGID)
                        a->a_un.a_val |= OSF1_LDR_EXEC_SETGID_F;
	        if (l->l_proc->p_flag & P_TRACED)
                        a->a_un.a_val |= OSF1_LDR_EXEC_PTRACE_F;
		a++;
	}

	a->a_type = OSF1_AT_NULL;
	a->a_un.a_val = 0;
	a++;

	len = (a - ai) * sizeof(struct osf1_auxv);
	if ((error = copyout(ai, *stackp, len)) != 0)
		goto out;
	*stackp += len;

out:
	free(pack->ep_emul_arg, M_TEMP);
	pack->ep_emul_arg = NULL;
	return error;
}

static int
osf1_exec_ecoff_dynamic(struct lwp *l, struct exec_package *epp)
{
	struct osf1_exec_emul_arg *emul_arg = epp->ep_emul_arg;
	struct ecoff_exechdr ldr_exechdr;
	struct nameidata nd;
	struct vnode *ldr_vp;
	struct proc *p;
        size_t resid;
	int error;

	p = l->l_proc;

	strncpy(emul_arg->loader_name, OSF1_LDR_EXEC_DEFAULT_LOADER,
		MAXPATHLEN + 1);

	/*
	 * locate the loader
	 * includes /emul/osf1 if appropriate
	 */
	error = emul_find_interp(LIST_FIRST(&p->p_lwps),
	    epp->ep_esch->es_emul->e_path, emul_arg->loader_name);
	if (error)
		return error;

	emul_arg->flags |= OSF1_EXEC_EMUL_FLAGS_HAVE_LOADER;

#if 0
	uprintf("loader is %s\n", emul_arg->loader_name);
#endif

	/*
	 * open the loader, see if it's an ECOFF executable,
	 * make sure the object type is amenable, then arrange to
	 * load it up.
	 */
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_SYSSPACE,
	    emul_arg->loader_name, l);
	if ((error = namei(&nd)) != 0)
		goto bad_no_vp;
	ldr_vp = nd.ni_vp;

	/*
	 * Basic access checks.  Reject if:
	 *	not a regular file
	 *	exec not allowed on binary
	 *	exec not allowed on mount point
	 */
	if (ldr_vp->v_type != VREG) {
		error = EACCES;
		goto badunlock;
	}

	if ((error = VOP_ACCESS(ldr_vp, VEXEC, p->p_cred, l)) != 0)
		goto badunlock;

        if (ldr_vp->v_mount->mnt_flag & MNT_NOEXEC) {
                error = EACCES;
                goto badunlock;
        }

	/*
	 * If loader's mount point disallows set-id execution,
	 * disable set-id.
	 */
        if (ldr_vp->v_mount->mnt_flag & MNT_NOSUID)
                epp->ep_vap->va_mode &= ~(S_ISUID | S_ISGID);

	VOP_UNLOCK(ldr_vp, 0);

	/*
	 * read the header, and make sure we got all of it.
	 */
        if ((error = vn_rdwr(UIO_READ, ldr_vp, (caddr_t)&ldr_exechdr,
	    sizeof ldr_exechdr, 0, UIO_SYSSPACE, 0, p->p_cred,
	    &resid, NULL)) != 0)
                goto bad;
        if (resid != 0) {
                error = ENOEXEC;
                goto bad;
	}

	/*
	 * Check the magic.  We expect it to be the native Alpha ECOFF
	 * (Digital UNIX) magic number.  Also make sure it's not a shared
	 * lib or dynamically linked executable.
	 */
	if (ldr_exechdr.f.f_magic != ECOFF_MAGIC_ALPHA) {
		error = ENOEXEC;
		goto bad;
	}
        switch (ldr_exechdr.f.f_flags & ECOFF_FLAG_OBJECT_TYPE_MASK) {
        case ECOFF_OBJECT_TYPE_SHARABLE:
        case ECOFF_OBJECT_TYPE_CALL_SHARED:
		/* can't exec shared lib or dynamically linked executable. */
		error = ENOEXEC;
		goto bad;
	}

	switch (ldr_exechdr.a.magic) {
	case ECOFF_OMAGIC:
		error = exec_ecoff_prep_omagic(l, epp, &ldr_exechdr, ldr_vp);
		break;
	case ECOFF_NMAGIC:
		error = exec_ecoff_prep_nmagic(l, epp, &ldr_exechdr, ldr_vp);
		break;
	case ECOFF_ZMAGIC:
		error = exec_ecoff_prep_zmagic(l, epp, &ldr_exechdr, ldr_vp);
		break;
	default:
		error = ENOEXEC;
	}
	if (error)
		goto bad;

	/* finally, set up the stack. */
	error = (*epp->ep_esch->es_setup_stack)(l, epp);
	if (error)
		goto bad;

	vrele(ldr_vp);
	return (0);

badunlock:
	VOP_UNLOCK(ldr_vp, 0);
bad:
	vrele(ldr_vp);
bad_no_vp:
	return (error);
}
