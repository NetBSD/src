/*	$NetBSD: kern_exec.c,v 1.201 2005/06/27 17:11:21 elad Exp $	*/

/*-
 * Copyright (C) 1993, 1994, 1996 Christopher G. Demetriou
 * Copyright (C) 1992 Wolfgang Solfrank.
 * Copyright (C) 1992 TooLs GmbH.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_exec.c,v 1.201 2005/06/27 17:11:21 elad Exp $");

#include "opt_ktrace.h"
#include "opt_syscall_debug.h"
#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/acct.h>
#include <sys/exec.h>
#include <sys/ktrace.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/ras.h>
#include <sys/signalvar.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscallargs.h>
#ifdef VERIFIED_EXEC
#include <sys/verified_exec.h>
#endif

#ifdef SYSTRACE
#include <sys/systrace.h>
#endif /* SYSTRACE */

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>

static int exec_sigcode_map(struct proc *, const struct emul *);

#ifdef DEBUG_EXEC
#define DPRINTF(a) uprintf a
#else
#define DPRINTF(a)
#endif /* DEBUG_EXEC */

MALLOC_DEFINE(M_EXEC, "exec", "argument lists & other mem used by exec");

/*
 * Exec function switch:
 *
 * Note that each makecmds function is responsible for loading the
 * exec package with the necessary functions for any exec-type-specific
 * handling.
 *
 * Functions for specific exec types should be defined in their own
 * header file.
 */
extern const struct execsw	execsw_builtin[];
extern int			nexecs_builtin;
static const struct execsw	**execsw = NULL;
static int			nexecs;

u_int	exec_maxhdrsz;		/* must not be static - netbsd32 needs it */

#ifdef LKM
/* list of supported emulations */
static
LIST_HEAD(emlist_head, emul_entry) el_head = LIST_HEAD_INITIALIZER(el_head);
struct emul_entry {
	LIST_ENTRY(emul_entry)	el_list;
	const struct emul	*el_emul;
	int			ro_entry;
};

/* list of dynamically loaded execsw entries */
static
LIST_HEAD(execlist_head, exec_entry) ex_head = LIST_HEAD_INITIALIZER(ex_head);
struct exec_entry {
	LIST_ENTRY(exec_entry)	ex_list;
	const struct execsw	*es;
};

/* structure used for building execw[] */
struct execsw_entry {
	struct execsw_entry	*next;
	const struct execsw	*es;
};
#endif /* LKM */

#ifdef SYSCALL_DEBUG
extern const char * const syscallnames[];
#endif
#ifdef __HAVE_SYSCALL_INTERN
void syscall_intern(struct proc *);
#else
void syscall(void);
#endif

#ifdef COMPAT_16
extern char	sigcode[], esigcode[];
struct uvm_object *emul_netbsd_object;
#endif

/* NetBSD emul struct */
const struct emul emul_netbsd = {
	"netbsd",
	NULL,		/* emulation path */
#ifndef __HAVE_MINIMAL_EMUL
	EMUL_HAS_SYS___syscall,
	NULL,
	SYS_syscall,
	SYS_NSYSENT,
#endif
	sysent,
#ifdef SYSCALL_DEBUG
	syscallnames,
#else
	NULL,
#endif
	sendsig,
	trapsignal,
	NULL,
#ifdef COMPAT_16
	sigcode,
	esigcode,
	&emul_netbsd_object,
#else
	NULL,
	NULL,
	NULL,
#endif
	setregs,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	syscall_intern,
#else
	syscall,
#endif
	NULL,
	NULL,

	uvm_default_mapaddr,
};

#ifdef LKM
/*
 * Exec lock. Used to control access to execsw[] structures.
 * This must not be static so that netbsd32 can access it, too.
 */
struct lock exec_lock;

static void link_es(struct execsw_entry **, const struct execsw *);
#endif /* LKM */

/*
 * check exec:
 * given an "executable" described in the exec package's namei info,
 * see what we can do with it.
 *
 * ON ENTRY:
 *	exec package with appropriate namei info
 *	proc pointer of exec'ing proc
 *      if verified exec enabled then flag indicating a direct exec or
 *        an indirect exec (i.e. for a shell script interpreter)
 *	NO SELF-LOCKED VNODES
 *
 * ON EXIT:
 *	error:	nothing held, etc.  exec header still allocated.
 *	ok:	filled exec package, executable's vnode (unlocked).
 *
 * EXEC SWITCH ENTRY:
 * 	Locked vnode to check, exec package, proc.
 *
 * EXEC SWITCH EXIT:
 *	ok:	return 0, filled exec package, executable's vnode (unlocked).
 *	error:	destructive:
 *			everything deallocated execept exec header.
 *		non-destructive:
 *			error code, executable's vnode (unlocked),
 *			exec header unmodified.
 */
int
#ifdef VERIFIED_EXEC
check_exec(struct proc *p, struct exec_package *epp, int flag)
#else
check_exec(struct proc *p, struct exec_package *epp)
#endif
{
	int		error, i;
	struct vnode	*vp;
	struct nameidata *ndp;
	size_t		resid;

	ndp = epp->ep_ndp;
	ndp->ni_cnd.cn_nameiop = LOOKUP;
	ndp->ni_cnd.cn_flags = FOLLOW | LOCKLEAF | SAVENAME;
	/* first get the vnode */
	if ((error = namei(ndp)) != 0)
		return error;
	epp->ep_vp = vp = ndp->ni_vp;

	/* check access and type */
	if (vp->v_type != VREG) {
		error = EACCES;
		goto bad1;
	}
	if ((error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p)) != 0)
		goto bad1;

	/* get attributes */
	if ((error = VOP_GETATTR(vp, epp->ep_vap, p->p_ucred, p)) != 0)
		goto bad1;

	/* Check mount point */
	if (vp->v_mount->mnt_flag & MNT_NOEXEC) {
		error = EACCES;
		goto bad1;
	}
	if (vp->v_mount->mnt_flag & MNT_NOSUID)
		epp->ep_vap->va_mode &= ~(S_ISUID | S_ISGID);

	/* try to open it */
	if ((error = VOP_OPEN(vp, FREAD, p->p_ucred, p)) != 0)
		goto bad1;

	/* unlock vp, since we need it unlocked from here on out. */
	VOP_UNLOCK(vp, 0);


#ifdef VERIFIED_EXEC
        /* Evaluate signature for file... */
        if ((error = veriexec_verify(p, vp, epp->ep_vap, epp->ep_name,
				     flag, NULL)) != 0)
                goto bad2;
#endif

	/* now we have the file, get the exec header */
	uvn_attach(vp, VM_PROT_READ);
	error = vn_rdwr(UIO_READ, vp, epp->ep_hdr, epp->ep_hdrlen, 0,
			UIO_SYSSPACE, 0, p->p_ucred, &resid, NULL);
	if (error)
		goto bad2;
	epp->ep_hdrvalid = epp->ep_hdrlen - resid;

	/*
	 * Set up default address space limits.  Can be overridden
	 * by individual exec packages.
	 *
	 * XXX probably should be all done in the exec pakages.
	 */
	epp->ep_vm_minaddr = VM_MIN_ADDRESS;
	epp->ep_vm_maxaddr = VM_MAXUSER_ADDRESS;
	/*
	 * set up the vmcmds for creation of the process
	 * address space
	 */
	error = ENOEXEC;
	for (i = 0; i < nexecs && error != 0; i++) {
		int newerror;

		epp->ep_esch = execsw[i];
		newerror = (*execsw[i]->es_makecmds)(p, epp);
		/* make sure the first "interesting" error code is saved. */
		if (!newerror || error == ENOEXEC)
			error = newerror;

		/* if es_makecmds call was successful, update epp->ep_es */
		if (!newerror && (epp->ep_flags & EXEC_HASES) == 0)
			epp->ep_es = execsw[i];

		if (epp->ep_flags & EXEC_DESTR && error != 0)
			return error;
	}
	if (!error) {
		/* check that entry point is sane */
		if (epp->ep_entry > VM_MAXUSER_ADDRESS)
			error = ENOEXEC;

		/* check limits */
		if ((epp->ep_tsize > MAXTSIZ) ||
		    (epp->ep_dsize >
		     (u_quad_t)p->p_rlimit[RLIMIT_DATA].rlim_cur))
			error = ENOMEM;

		if (!error)
			return (0);
	}

	/*
	 * free any vmspace-creation commands,
	 * and release their references
	 */
	kill_vmcmds(&epp->ep_vmcmds);

bad2:
	/*
	 * close and release the vnode, restore the old one, free the
	 * pathname buf, and punt.
	 */
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(vp, FREAD, p->p_ucred, p);
	vput(vp);
	PNBUF_PUT(ndp->ni_cnd.cn_pnbuf);
	return error;

bad1:
	/*
	 * free the namei pathname buffer, and put the vnode
	 * (which we don't yet have open).
	 */
	vput(vp);				/* was still locked */
	PNBUF_PUT(ndp->ni_cnd.cn_pnbuf);
	return error;
}

#ifdef __MACHINE_STACK_GROWS_UP
#define STACK_PTHREADSPACE NBPG
#else
#define STACK_PTHREADSPACE 0
#endif

/*
 * exec system call
 */
/* ARGSUSED */
int
sys_execve(struct lwp *l, void *v, register_t *retval)
{
	struct sys_execve_args /* {
		syscallarg(const char *)	path;
		syscallarg(char * const *)	argp;
		syscallarg(char * const *)	envp;
	} */ *uap = v;
	int			error;
	u_int			i;
	struct exec_package	pack;
	struct nameidata	nid;
	struct vattr		attr;
	struct proc		*p;
	struct ucred		*cred;
	char			*argp;
	char * const		*cpp;
	char			*dp, *sp;
	long			argc, envc;
	size_t			len;
	char			*stack;
	struct ps_strings	arginfo;
	struct vmspace		*vm;
	char			**tmpfap;
	int			szsigcode;
	struct exec_vmcmd	*base_vcp;
	int			oldlwpflags;
#ifdef SYSTRACE
	int			wassugid = ISSET(p->p_flag, P_SUGID);
	char			pathbuf[MAXPATHLEN];
	size_t			pathbuflen;
#endif /* SYSTRACE */

	/* Disable scheduler activation upcalls. */
	oldlwpflags = l->l_flag & (L_SA | L_SA_UPCALL);
	if (l->l_flag & L_SA)
		l->l_flag &= ~(L_SA | L_SA_UPCALL);

	p = l->l_proc;
	/*
	 * Lock the process and set the P_INEXEC flag to indicate that
	 * it should be left alone until we're done here.  This is
	 * necessary to avoid race conditions - e.g. in ptrace() -
	 * that might allow a local user to illicitly obtain elevated
	 * privileges.
	 */
	p->p_flag |= P_INEXEC;

	cred = p->p_ucred;
	base_vcp = NULL;
	/*
	 * Init the namei data to point the file user's program name.
	 * This is done here rather than in check_exec(), so that it's
	 * possible to override this settings if any of makecmd/probe
	 * functions call check_exec() recursively - for example,
	 * see exec_script_makecmds().
	 */
#ifdef SYSTRACE
	if (ISSET(p->p_flag, P_SYSTRACE))
		systrace_execve0(p);

	error = copyinstr(SCARG(uap, path), pathbuf, sizeof(pathbuf),
			  &pathbuflen);
	if (error)
		goto clrflg;

	NDINIT(&nid, LOOKUP, NOFOLLOW, UIO_SYSSPACE, pathbuf, p);
#else
	NDINIT(&nid, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);
#endif /* SYSTRACE */

	/*
	 * initialize the fields of the exec package.
	 */
#ifdef SYSTRACE
	pack.ep_name = pathbuf;
#else
	pack.ep_name = SCARG(uap, path);
#endif /* SYSTRACE */
	pack.ep_hdr = malloc(exec_maxhdrsz, M_EXEC, M_WAITOK);
	pack.ep_hdrlen = exec_maxhdrsz;
	pack.ep_hdrvalid = 0;
	pack.ep_ndp = &nid;
	pack.ep_emul_arg = NULL;
	pack.ep_vmcmds.evs_cnt = 0;
	pack.ep_vmcmds.evs_used = 0;
	pack.ep_vap = &attr;
	pack.ep_flags = 0;

#ifdef LKM
	lockmgr(&exec_lock, LK_SHARED, NULL);
#endif

	/* see if we can run it. */
#ifdef VERIFIED_EXEC
        if ((error = check_exec(p, &pack, VERIEXEC_DIRECT)) != 0)
#else
        if ((error = check_exec(p, &pack)) != 0)
#endif
		goto freehdr;

	/* XXX -- THE FOLLOWING SECTION NEEDS MAJOR CLEANUP */

	/* allocate an argument buffer */
	argp = (char *) uvm_km_alloc(exec_map, NCARGS, 0,
	    UVM_KMF_PAGEABLE|UVM_KMF_WAITVA);
#ifdef DIAGNOSTIC
	if (argp == (vaddr_t) 0)
		panic("execve: argp == NULL");
#endif
	dp = argp;
	argc = 0;

	/* copy the fake args list, if there's one, freeing it as we go */
	if (pack.ep_flags & EXEC_HASARGL) {
		tmpfap = pack.ep_fa;
		while (*tmpfap != NULL) {
			char *cp;

			cp = *tmpfap;
			while (*cp)
				*dp++ = *cp++;
			dp++;

			FREE(*tmpfap, M_EXEC);
			tmpfap++; argc++;
		}
		FREE(pack.ep_fa, M_EXEC);
		pack.ep_flags &= ~EXEC_HASARGL;
	}

	/* Now get argv & environment */
	if (!(cpp = SCARG(uap, argp))) {
		error = EINVAL;
		goto bad;
	}

	if (pack.ep_flags & EXEC_SKIPARG)
		cpp++;

	while (1) {
		len = argp + ARG_MAX - dp;
		if ((error = copyin(cpp, &sp, sizeof(sp))) != 0)
			goto bad;
		if (!sp)
			break;
		if ((error = copyinstr(sp, dp, len, &len)) != 0) {
			if (error == ENAMETOOLONG)
				error = E2BIG;
			goto bad;
		}
#ifdef KTRACE
		if (KTRPOINT(p, KTR_EXEC_ARG))
			ktrkmem(p, KTR_EXEC_ARG, dp, len - 1);
#endif
		dp += len;
		cpp++;
		argc++;
	}

	envc = 0;
	/* environment need not be there */
	if ((cpp = SCARG(uap, envp)) != NULL ) {
		while (1) {
			len = argp + ARG_MAX - dp;
			if ((error = copyin(cpp, &sp, sizeof(sp))) != 0)
				goto bad;
			if (!sp)
				break;
			if ((error = copyinstr(sp, dp, len, &len)) != 0) {
				if (error == ENAMETOOLONG)
					error = E2BIG;
				goto bad;
			}
#ifdef KTRACE
			if (KTRPOINT(p, KTR_EXEC_ENV))
				ktrkmem(p, KTR_EXEC_ENV, dp, len - 1);
#endif
			dp += len;
			cpp++;
			envc++;
		}
	}

	dp = (char *) ALIGN(dp);

	szsigcode = pack.ep_es->es_emul->e_esigcode -
	    pack.ep_es->es_emul->e_sigcode;

	/* Now check if args & environ fit into new stack */
	if (pack.ep_flags & EXEC_32)
		len = ((argc + envc + 2 + pack.ep_es->es_arglen) *
		    sizeof(int) + sizeof(int) + dp + STACKGAPLEN +
		    szsigcode + sizeof(struct ps_strings) + STACK_PTHREADSPACE)
		    - argp;
	else
		len = ((argc + envc + 2 + pack.ep_es->es_arglen) *
		    sizeof(char *) + sizeof(int) + dp + STACKGAPLEN +
		    szsigcode + sizeof(struct ps_strings) + STACK_PTHREADSPACE)
		    - argp;

	len = ALIGN(len);	/* make the stack "safely" aligned */

	if (len > pack.ep_ssize) { /* in effect, compare to initial limit */
		error = ENOMEM;
		goto bad;
	}

	/* Get rid of other LWPs/ */
	p->p_flag |= P_WEXIT; /* XXX hack. lwp-exit stuff wants to see it. */
	exit_lwps(l);
	p->p_flag &= ~P_WEXIT;
	KDASSERT(p->p_nlwps == 1);

	/* This is now LWP 1 */
	l->l_lid = 1;
	p->p_nlwpid = 1;

	/* Release any SA state. */
	if (p->p_sa)
		sa_release(p);

	/* Remove POSIX timers */
	timers_free(p, TIMERS_POSIX);

	/* adjust "active stack depth" for process VSZ */
	pack.ep_ssize = len;	/* maybe should go elsewhere, but... */

	/*
	 * Do whatever is necessary to prepare the address space
	 * for remapping.  Note that this might replace the current
	 * vmspace with another!
	 */
	uvmspace_exec(l, pack.ep_vm_minaddr, pack.ep_vm_maxaddr);

	/* record proc's vnode, for use by procfs and others */
        if (p->p_textvp)
                vrele(p->p_textvp);
	VREF(pack.ep_vp);
	p->p_textvp = pack.ep_vp;

	/* Now map address space */
	vm = p->p_vmspace;
	vm->vm_taddr = (caddr_t) pack.ep_taddr;
	vm->vm_tsize = btoc(pack.ep_tsize);
	vm->vm_daddr = (caddr_t) pack.ep_daddr;
	vm->vm_dsize = btoc(pack.ep_dsize);
	vm->vm_ssize = btoc(pack.ep_ssize);
	vm->vm_maxsaddr = (caddr_t) pack.ep_maxsaddr;
	vm->vm_minsaddr = (caddr_t) pack.ep_minsaddr;

	/* create the new process's VM space by running the vmcmds */
#ifdef DIAGNOSTIC
	if (pack.ep_vmcmds.evs_used == 0)
		panic("execve: no vmcmds");
#endif
	for (i = 0; i < pack.ep_vmcmds.evs_used && !error; i++) {
		struct exec_vmcmd *vcp;

		vcp = &pack.ep_vmcmds.evs_cmds[i];
		if (vcp->ev_flags & VMCMD_RELATIVE) {
#ifdef DIAGNOSTIC
			if (base_vcp == NULL)
				panic("execve: relative vmcmd with no base");
			if (vcp->ev_flags & VMCMD_BASE)
				panic("execve: illegal base & relative vmcmd");
#endif
			vcp->ev_addr += base_vcp->ev_addr;
		}
		error = (*vcp->ev_proc)(p, vcp);
#ifdef DEBUG_EXEC
		if (error) {
			int j;
			struct exec_vmcmd *vp = &pack.ep_vmcmds.evs_cmds[0];
			for (j = 0; j <= i; j++)
				uprintf(
			    "vmcmd[%d] = %#lx/%#lx fd@%#lx prot=0%o flags=%d\n",
				    j, vp[j].ev_addr, vp[j].ev_len,
				    vp[j].ev_offset, vp[j].ev_prot,
				    vp[j].ev_flags);
		}
#endif /* DEBUG_EXEC */
		if (vcp->ev_flags & VMCMD_BASE)
			base_vcp = vcp;
	}

	/* free the vmspace-creation commands, and release their references */
	kill_vmcmds(&pack.ep_vmcmds);

	vn_lock(pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(pack.ep_vp, FREAD, cred, p);
	vput(pack.ep_vp);

	/* if an error happened, deallocate and punt */
	if (error) {
		DPRINTF(("execve: vmcmd %i failed: %d\n", i - 1, error));
		goto exec_abort;
	}

	/* remember information about the process */
	arginfo.ps_nargvstr = argc;
	arginfo.ps_nenvstr = envc;

	stack = (char *)STACK_ALLOC(STACK_GROW(vm->vm_minsaddr,
		STACK_PTHREADSPACE + sizeof(struct ps_strings) + szsigcode),
		len - (sizeof(struct ps_strings) + szsigcode));
#ifdef __MACHINE_STACK_GROWS_UP
	/*
	 * The copyargs call always copies into lower addresses
	 * first, moving towards higher addresses, starting with
	 * the stack pointer that we give.  When the stack grows
	 * down, this puts argc/argv/envp very shallow on the
	 * stack, right at the first user stack pointer, and puts
	 * STACKGAPLEN very deep in the stack.  When the stack
	 * grows up, the situation is reversed.
	 *
	 * Normally, this is no big deal.  But the ld_elf.so _rtld()
	 * function expects to be called with a single pointer to
	 * a region that has a few words it can stash values into,
	 * followed by argc/argv/envp.  When the stack grows down,
	 * it's easy to decrement the stack pointer a little bit to
	 * allocate the space for these few words and pass the new
	 * stack pointer to _rtld.  When the stack grows up, however,
	 * a few words before argc is part of the signal trampoline, XXX
	 * so we have a problem.
	 *
	 * Instead of changing how _rtld works, we take the easy way
	 * out and steal 32 bytes before we call copyargs.  This
	 * space is effectively stolen from STACKGAPLEN.
	 */
	stack += 32;
#endif /* __MACHINE_STACK_GROWS_UP */

	/* Now copy argc, args & environ to new stack */
	error = (*pack.ep_es->es_copyargs)(p, &pack, &arginfo, &stack, argp);
	if (error) {
		DPRINTF(("execve: copyargs failed %d\n", error));
		goto exec_abort;
	}
	/* Move the stack back to original point */
	stack = (char *)STACK_GROW(vm->vm_minsaddr, len);

	/* fill process ps_strings info */
	p->p_psstr = (struct ps_strings *)
	    STACK_ALLOC(STACK_GROW(vm->vm_minsaddr, STACK_PTHREADSPACE),
	    sizeof(struct ps_strings));
	p->p_psargv = offsetof(struct ps_strings, ps_argvstr);
	p->p_psnargv = offsetof(struct ps_strings, ps_nargvstr);
	p->p_psenv = offsetof(struct ps_strings, ps_envstr);
	p->p_psnenv = offsetof(struct ps_strings, ps_nenvstr);

	/* copy out the process's ps_strings structure */
	if ((error = copyout(&arginfo, (char *)p->p_psstr,
	    sizeof(arginfo))) != 0) {
		DPRINTF(("execve: ps_strings copyout %p->%p size %ld failed\n",
		       &arginfo, (char *)p->p_psstr, (long)sizeof(arginfo)));
		goto exec_abort;
	}

	stopprofclock(p);	/* stop profiling */
	fdcloseexec(p);		/* handle close on exec */
	execsigs(p);		/* reset catched signals */

	l->l_ctxlink = NULL;	/* reset ucontext link */

	/* set command name & other accounting info */
	len = min(nid.ni_cnd.cn_namelen, MAXCOMLEN);
	memcpy(p->p_comm, nid.ni_cnd.cn_nameptr, len);
	p->p_comm[len] = 0;
	p->p_acflag &= ~AFORK;

	p->p_flag |= P_EXEC;
	if (p->p_flag & P_PPWAIT) {
		p->p_flag &= ~P_PPWAIT;
		wakeup((caddr_t) p->p_pptr);
	}

	/*
	 * deal with set[ug]id.
	 * MNT_NOSUID has already been used to disable s[ug]id.
	 */
	if ((p->p_flag & P_TRACED) == 0 &&

	    (((attr.va_mode & S_ISUID) != 0 &&
	      p->p_ucred->cr_uid != attr.va_uid) ||

	     ((attr.va_mode & S_ISGID) != 0 &&
	      p->p_ucred->cr_gid != attr.va_gid))) {
		/*
		 * Mark the process as SUGID before we do
		 * anything that might block.
		 */
		p_sugid(p);

		/* Make sure file descriptors 0..2 are in use. */
		if ((error = fdcheckstd(p)) != 0)
			goto exec_abort;

		p->p_ucred = crcopy(cred);
#ifdef KTRACE
		/*
		 * If process is being ktraced, turn off - unless
		 * root set it.
		 */
		if (p->p_tracep && !(p->p_traceflag & KTRFAC_ROOT))
			ktrderef(p);
#endif
		if (attr.va_mode & S_ISUID)
			p->p_ucred->cr_uid = attr.va_uid;
		if (attr.va_mode & S_ISGID)
			p->p_ucred->cr_gid = attr.va_gid;
	} else
		p->p_flag &= ~P_SUGID;
	p->p_cred->p_svuid = p->p_ucred->cr_uid;
	p->p_cred->p_svgid = p->p_ucred->cr_gid;

#if defined(__HAVE_RAS)
	/*
	 * Remove all RASs from the address space.
	 */
	ras_purgeall(p);
#endif

	doexechooks(p);

	uvm_km_free(exec_map, (vaddr_t) argp, NCARGS, UVM_KMF_PAGEABLE);

	PNBUF_PUT(nid.ni_cnd.cn_pnbuf);

	/* notify others that we exec'd */
	KNOTE(&p->p_klist, NOTE_EXEC);

	/* setup new registers and do misc. setup. */
	(*pack.ep_es->es_emul->e_setregs)(l, &pack, (u_long) stack);
	if (pack.ep_es->es_setregs)
		(*pack.ep_es->es_setregs)(l, &pack, (u_long) stack);

	/* map the process's signal trampoline code */
	if (exec_sigcode_map(p, pack.ep_es->es_emul))
		goto exec_abort;

	if (p->p_flag & P_TRACED)
		psignal(p, SIGTRAP);

	free(pack.ep_hdr, M_EXEC);

	/*
	 * Call emulation specific exec hook. This can setup per-process
	 * p->p_emuldata or do any other per-process stuff an emulation needs.
	 *
	 * If we are executing process of different emulation than the
	 * original forked process, call e_proc_exit() of the old emulation
	 * first, then e_proc_exec() of new emulation. If the emulation is
	 * same, the exec hook code should deallocate any old emulation
	 * resources held previously by this process.
	 */
	if (p->p_emul && p->p_emul->e_proc_exit
	    && p->p_emul != pack.ep_es->es_emul)
		(*p->p_emul->e_proc_exit)(p);

	/*
	 * Call exec hook. Emulation code may NOT store reference to anything
	 * from &pack.
	 */
        if (pack.ep_es->es_emul->e_proc_exec)
                (*pack.ep_es->es_emul->e_proc_exec)(p, &pack);

	/* update p_emul, the old value is no longer needed */
	p->p_emul = pack.ep_es->es_emul;

	/* ...and the same for p_execsw */
	p->p_execsw = pack.ep_es;

#ifdef __HAVE_SYSCALL_INTERN
	(*p->p_emul->e_syscall_intern)(p);
#endif
#ifdef KTRACE
	if (KTRPOINT(p, KTR_EMUL))
		ktremul(p);
#endif

#ifdef LKM
	lockmgr(&exec_lock, LK_RELEASE, NULL);
#endif
	p->p_flag &= ~P_INEXEC;

	if (p->p_flag & P_STOPEXEC) {
		int s;

		sigminusset(&contsigmask, &p->p_sigctx.ps_siglist);
		SCHED_LOCK(s);
		p->p_pptr->p_nstopchild++;
		p->p_stat = SSTOP;
		l->l_stat = LSSTOP;
		p->p_nrlwps--;
		mi_switch(l, NULL);
		SCHED_ASSERT_UNLOCKED();
		splx(s);
	}

#ifdef SYSTRACE
	if (ISSET(p->p_flag, P_SYSTRACE) &&
	    wassugid && !ISSET(p->p_flag, P_SUGID))
		systrace_execve1(pathbuf, p);
#endif /* SYSTRACE */

	return (EJUSTRETURN);

 bad:
	p->p_flag &= ~P_INEXEC;
	/* free the vmspace-creation commands, and release their references */
	kill_vmcmds(&pack.ep_vmcmds);
	/* kill any opened file descriptor, if necessary */
	if (pack.ep_flags & EXEC_HASFD) {
		pack.ep_flags &= ~EXEC_HASFD;
		(void) fdrelease(p, pack.ep_fd);
	}
	/* close and put the exec'd file */
	vn_lock(pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(pack.ep_vp, FREAD, cred, p);
	vput(pack.ep_vp);
	PNBUF_PUT(nid.ni_cnd.cn_pnbuf);
	uvm_km_free(exec_map, (vaddr_t) argp, NCARGS, UVM_KMF_PAGEABLE);

 freehdr:
	free(pack.ep_hdr, M_EXEC);

#ifdef SYSTRACE
 clrflg:
#endif /* SYSTRACE */
	l->l_flag |= oldlwpflags;
	p->p_flag &= ~P_INEXEC;
#ifdef LKM
	lockmgr(&exec_lock, LK_RELEASE, NULL);
#endif

	return error;

 exec_abort:
	p->p_flag &= ~P_INEXEC;
#ifdef LKM
	lockmgr(&exec_lock, LK_RELEASE, NULL);
#endif

	/*
	 * the old process doesn't exist anymore.  exit gracefully.
	 * get rid of the (new) address space we have created, if any, get rid
	 * of our namei data and vnode, and exit noting failure
	 */
	uvm_deallocate(&vm->vm_map, VM_MIN_ADDRESS,
		VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS);
	if (pack.ep_emul_arg)
		FREE(pack.ep_emul_arg, M_TEMP);
	PNBUF_PUT(nid.ni_cnd.cn_pnbuf);
	uvm_km_free(exec_map, (vaddr_t) argp, NCARGS, UVM_KMF_PAGEABLE);
	free(pack.ep_hdr, M_EXEC);
	exit1(l, W_EXITCODE(error, SIGABRT));

	/* NOTREACHED */
	return 0;
}


int
copyargs(struct proc *p, struct exec_package *pack, struct ps_strings *arginfo,
    char **stackp, void *argp)
{
	char	**cpp, *dp, *sp;
	size_t	len;
	void	*nullp;
	long	argc, envc;
	int	error;

	cpp = (char **)*stackp;
	nullp = NULL;
	argc = arginfo->ps_nargvstr;
	envc = arginfo->ps_nenvstr;
	if ((error = copyout(&argc, cpp++, sizeof(argc))) != 0)
		return error;

	dp = (char *) (cpp + argc + envc + 2 + pack->ep_es->es_arglen);
	sp = argp;

	/* XXX don't copy them out, remap them! */
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

	*stackp = (char *)cpp;
	return 0;
}

#ifdef LKM
/*
 * Find an emulation of given name in list of emulations.
 * Needs to be called with the exec_lock held.
 */
const struct emul *
emul_search(const char *name)
{
	struct emul_entry *it;

	LIST_FOREACH(it, &el_head, el_list) {
		if (strcmp(name, it->el_emul->e_name) == 0)
			return it->el_emul;
	}

	return NULL;
}

/*
 * Add an emulation to list, if it's not there already.
 */
int
emul_register(const struct emul *emul, int ro_entry)
{
	struct emul_entry	*ee;
	int			error;

	error = 0;
	lockmgr(&exec_lock, LK_SHARED, NULL);

	if (emul_search(emul->e_name)) {
		error = EEXIST;
		goto out;
	}

	MALLOC(ee, struct emul_entry *, sizeof(struct emul_entry),
		M_EXEC, M_WAITOK);
	ee->el_emul = emul;
	ee->ro_entry = ro_entry;
	LIST_INSERT_HEAD(&el_head, ee, el_list);

 out:
	lockmgr(&exec_lock, LK_RELEASE, NULL);
	return error;
}

/*
 * Remove emulation with name 'name' from list of supported emulations.
 */
int
emul_unregister(const char *name)
{
	const struct proclist_desc *pd;
	struct emul_entry	*it;
	int			i, error;
	struct proc		*ptmp;

	error = 0;
	lockmgr(&exec_lock, LK_SHARED, NULL);

	LIST_FOREACH(it, &el_head, el_list) {
		if (strcmp(it->el_emul->e_name, name) == 0)
			break;
	}

	if (!it) {
		error = ENOENT;
		goto out;
	}

	if (it->ro_entry) {
		error = EBUSY;
		goto out;
	}

	/* test if any execw[] entry is still using this */
	for(i=0; i < nexecs; i++) {
		if (execsw[i]->es_emul == it->el_emul) {
			error = EBUSY;
			goto out;
		}
	}

	/*
	 * Test if any process is running under this emulation - since
	 * emul_unregister() is running quite sendomly, it's better
	 * to do expensive check here than to use any locking.
	 */
	proclist_lock_read();
	for (pd = proclists; pd->pd_list != NULL && !error; pd++) {
		PROCLIST_FOREACH(ptmp, pd->pd_list) {
			if (ptmp->p_emul == it->el_emul) {
				error = EBUSY;
				break;
			}
		}
	}
	proclist_unlock_read();

	if (error)
		goto out;


	/* entry is not used, remove it */
	LIST_REMOVE(it, el_list);
	FREE(it, M_EXEC);

 out:
	lockmgr(&exec_lock, LK_RELEASE, NULL);
	return error;
}

/*
 * Add execsw[] entry.
 */
int
exec_add(struct execsw *esp, const char *e_name)
{
	struct exec_entry	*it;
	int			error;

	error = 0;
	lockmgr(&exec_lock, LK_EXCLUSIVE, NULL);

	if (!esp->es_emul) {
		esp->es_emul = emul_search(e_name);
		if (!esp->es_emul) {
			error = ENOENT;
			goto out;
		}
	}

	LIST_FOREACH(it, &ex_head, ex_list) {
		/* assume tuple (makecmds, probe_func, emulation) is unique */
		if (it->es->es_makecmds == esp->es_makecmds
		    && it->es->u.elf_probe_func == esp->u.elf_probe_func
		    && it->es->es_emul == esp->es_emul) {
			error = EEXIST;
			goto out;
		}
	}

	/* if we got here, the entry doesn't exist yet */
	MALLOC(it, struct exec_entry *, sizeof(struct exec_entry),
		M_EXEC, M_WAITOK);
	it->es = esp;
	LIST_INSERT_HEAD(&ex_head, it, ex_list);

	/* update execsw[] */
	exec_init(0);

 out:
	lockmgr(&exec_lock, LK_RELEASE, NULL);
	return error;
}

/*
 * Remove execsw[] entry.
 */
int
exec_remove(const struct execsw *esp)
{
	struct exec_entry	*it;
	int			error;

	error = 0;
	lockmgr(&exec_lock, LK_EXCLUSIVE, NULL);

	LIST_FOREACH(it, &ex_head, ex_list) {
		/* assume tuple (makecmds, probe_func, emulation) is unique */
		if (it->es->es_makecmds == esp->es_makecmds
		    && it->es->u.elf_probe_func == esp->u.elf_probe_func
		    && it->es->es_emul == esp->es_emul)
			break;
	}
	if (!it) {
		error = ENOENT;
		goto out;
	}

	/* remove item from list and free resources */
	LIST_REMOVE(it, ex_list);
	FREE(it, M_EXEC);

	/* update execsw[] */
	exec_init(0);

 out:
	lockmgr(&exec_lock, LK_RELEASE, NULL);
	return error;
}

static void
link_es(struct execsw_entry **listp, const struct execsw *esp)
{
	struct execsw_entry *et, *e1;

	MALLOC(et, struct execsw_entry *, sizeof(struct execsw_entry),
			M_TEMP, M_WAITOK);
	et->next = NULL;
	et->es = esp;
	if (*listp == NULL) {
		*listp = et;
		return;
	}

	switch(et->es->es_prio) {
	case EXECSW_PRIO_FIRST:
		/* put new entry as the first */
		et->next = *listp;
		*listp = et;
		break;
	case EXECSW_PRIO_ANY:
		/* put new entry after all *_FIRST and *_ANY entries */
		for(e1 = *listp; e1->next
			&& e1->next->es->es_prio != EXECSW_PRIO_LAST;
			e1 = e1->next);
		et->next = e1->next;
		e1->next = et;
		break;
	case EXECSW_PRIO_LAST:
		/* put new entry as the last one */
		for(e1 = *listp; e1->next; e1 = e1->next);
		e1->next = et;
		break;
	default:
#ifdef DIAGNOSTIC
		panic("execw[] entry with unknown priority %d found",
			et->es->es_prio);
#endif
		break;
	}
}

/*
 * Initialize exec structures. If init_boot is true, also does necessary
 * one-time initialization (it's called from main() that way).
 * Once system is multiuser, this should be called with exec_lock held,
 * i.e. via exec_{add|remove}().
 */
int
exec_init(int init_boot)
{
	const struct execsw	**new_es, * const *old_es;
	struct execsw_entry	*list, *e1;
	struct exec_entry	*e2;
	int			i, es_sz;

	if (init_boot) {
		/* do one-time initializations */
		lockinit(&exec_lock, PWAIT, "execlck", 0, 0);

		/* register compiled-in emulations */
		for(i=0; i < nexecs_builtin; i++) {
			if (execsw_builtin[i].es_emul)
				emul_register(execsw_builtin[i].es_emul, 1);
		}
#ifdef DIAGNOSTIC
		if (i == 0)
			panic("no emulations found in execsw_builtin[]");
#endif
	}

	/*
	 * Build execsw[] array from builtin entries and entries added
	 * at runtime.
	 */
	list = NULL;
	for(i=0; i < nexecs_builtin; i++)
		link_es(&list, &execsw_builtin[i]);

	/* Add dynamically loaded entries */
	es_sz = nexecs_builtin;
	LIST_FOREACH(e2, &ex_head, ex_list) {
		link_es(&list, e2->es);
		es_sz++;
	}

	/*
	 * Now that we have sorted all execw entries, create new execsw[]
	 * and free no longer needed memory in the process.
	 */
	new_es = malloc(es_sz * sizeof(struct execsw *), M_EXEC, M_WAITOK);
	for(i=0; list; i++) {
		new_es[i] = list->es;
		e1 = list->next;
		FREE(list, M_TEMP);
		list = e1;
	}

	/*
	 * New execsw[] array built, now replace old execsw[] and free
	 * used memory.
	 */
	old_es = execsw;
	execsw = new_es;
	nexecs = es_sz;
	if (old_es)
		/*XXXUNCONST*/
		free(__UNCONST(old_es), M_EXEC);

	/*
	 * Figure out the maximum size of an exec header.
	 */
	exec_maxhdrsz = 0;
	for (i = 0; i < nexecs; i++) {
		if (execsw[i]->es_hdrsz > exec_maxhdrsz)
			exec_maxhdrsz = execsw[i]->es_hdrsz;
	}

	return 0;
}
#endif

#ifndef LKM
/*
 * Simplified exec_init() for kernels without LKMs. Only initialize
 * exec_maxhdrsz and execsw[].
 */
int
exec_init(int init_boot)
{
	int i;

#ifdef DIAGNOSTIC
	if (!init_boot)
		panic("exec_init(): called with init_boot == 0");
#endif

	/* do one-time initializations */
	nexecs = nexecs_builtin;
	execsw = malloc(nexecs*sizeof(struct execsw *), M_EXEC, M_WAITOK);

	/*
	 * Fill in execsw[] and figure out the maximum size of an exec header.
	 */
	exec_maxhdrsz = 0;
	for(i=0; i < nexecs; i++) {
		execsw[i] = &execsw_builtin[i];
		if (execsw_builtin[i].es_hdrsz > exec_maxhdrsz)
			exec_maxhdrsz = execsw_builtin[i].es_hdrsz;
	}

	return 0;

}
#endif /* !LKM */

static int
exec_sigcode_map(struct proc *p, const struct emul *e)
{
	vaddr_t va;
	vsize_t sz;
	int error;
	struct uvm_object *uobj;

	sz = (vaddr_t)e->e_esigcode - (vaddr_t)e->e_sigcode;

	if (e->e_sigobject == NULL || sz == 0) {
		return 0;
	}

	/*
	 * If we don't have a sigobject for this emulation, create one.
	 *
	 * sigobject is an anonymous memory object (just like SYSV shared
	 * memory) that we keep a permanent reference to and that we map
	 * in all processes that need this sigcode. The creation is simple,
	 * we create an object, add a permanent reference to it, map it in
	 * kernel space, copy out the sigcode to it and unmap it.
	 * We map it with PROT_READ|PROT_EXEC into the process just
	 * the way sys_mmap() would map it.
	 */

	uobj = *e->e_sigobject;
	if (uobj == NULL) {
		uobj = uao_create(sz, 0);
		(*uobj->pgops->pgo_reference)(uobj);
		va = vm_map_min(kernel_map);
		if ((error = uvm_map(kernel_map, &va, round_page(sz),
		    uobj, 0, 0,
		    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
		    UVM_INH_SHARE, UVM_ADV_RANDOM, 0)))) {
			printf("kernel mapping failed %d\n", error);
			(*uobj->pgops->pgo_detach)(uobj);
			return (error);
		}
		memcpy((void *)va, e->e_sigcode, sz);
#ifdef PMAP_NEED_PROCWR
		pmap_procwr(&proc0, va, sz);
#endif
		uvm_unmap(kernel_map, va, va + round_page(sz));
		*e->e_sigobject = uobj;
	}

	/* Just a hint to uvm_map where to put it. */
	va = e->e_vm_default_addr(p, (vaddr_t)p->p_vmspace->vm_daddr,
	    round_page(sz));

#ifdef __alpha__
	/*
	 * Tru64 puts /sbin/loader at the end of user virtual memory,
	 * which causes the above calculation to put the sigcode at
	 * an invalid address.  Put it just below the text instead.
	 */
	if (va == (vaddr_t)vm_map_max(&p->p_vmspace->vm_map)) {
		va = (vaddr_t)p->p_vmspace->vm_taddr - round_page(sz);
	}
#endif

	(*uobj->pgops->pgo_reference)(uobj);
	error = uvm_map(&p->p_vmspace->vm_map, &va, round_page(sz),
			uobj, 0, 0,
			UVM_MAPFLAG(UVM_PROT_RX, UVM_PROT_RX, UVM_INH_SHARE,
				    UVM_ADV_RANDOM, 0));
	if (error) {
		(*uobj->pgops->pgo_detach)(uobj);
		return (error);
	}
	p->p_sigctx.ps_sigcode = (void *)va;
	return (0);
}
