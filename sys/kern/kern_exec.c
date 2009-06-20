/*	$NetBSD: kern_exec.c,v 1.272.2.2 2009/06/20 07:20:31 yamt Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_exec.c,v 1.272.2.2 2009/06/20 07:20:31 yamt Exp $");

#include "opt_ktrace.h"
#include "opt_modular.h"
#include "opt_syscall_debug.h"
#include "veriexec.h"
#include "opt_pax.h"
#include "opt_sa.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/acct.h>
#include <sys/exec.h>
#include <sys/ktrace.h>
#include <sys/uidinfo.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/ras.h>
#include <sys/signalvar.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/kauth.h>
#include <sys/lwpctl.h>
#include <sys/pax.h>
#include <sys/cpu.h>
#include <sys/module.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>
#if NVERIEXEC > 0
#include <sys/verified_exec.h>
#endif /* NVERIEXEC > 0 */

#include <uvm/uvm_extern.h>

#include <machine/reg.h>

#include <compat/common/compat_util.h>

static int exec_sigcode_map(struct proc *, const struct emul *);

#ifdef DEBUG_EXEC
#define DPRINTF(a) uprintf a
#else
#define DPRINTF(a)
#endif /* DEBUG_EXEC */

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
static const struct execsw	**execsw = NULL;
static int			nexecs;

u_int	exec_maxhdrsz;	 /* must not be static - used by netbsd32 */

/* list of dynamically loaded execsw entries */
static LIST_HEAD(execlist_head, exec_entry) ex_head =
    LIST_HEAD_INITIALIZER(ex_head);
struct exec_entry {
	LIST_ENTRY(exec_entry)	ex_list;
	SLIST_ENTRY(exec_entry)	ex_slist;
	const struct execsw	*ex_sw;
};

#ifndef __HAVE_SYSCALL_INTERN
void	syscall(void);
#endif

#ifdef KERN_SA
static struct sa_emul saemul_netbsd = {
	sizeof(ucontext_t),
	sizeof(struct sa_t),
	sizeof(struct sa_t *),
	NULL,
	NULL,
	cpu_upcall,
	(void (*)(struct lwp *, void *))getucontext_sa,
	sa_ucsp
};
#endif /* KERN_SA */

/* NetBSD emul struct */
struct emul emul_netbsd = {
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
	NULL,
	NULL,
	NULL,
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
	NULL,
#ifdef KERN_SA
	&saemul_netbsd,
#else
	NULL,
#endif
	sizeof(ucontext_t),
	startlwp,
};

/*
 * Exec lock. Used to control access to execsw[] structures.
 * This must not be static so that netbsd32 can access it, too.
 */
krwlock_t exec_lock;

static kmutex_t sigobject_lock;

static void *
exec_pool_alloc(struct pool *pp, int flags)
{

	return (void *)uvm_km_alloc(kernel_map, NCARGS, 0,
	    UVM_KMF_PAGEABLE | UVM_KMF_WAITVA);
}

static void
exec_pool_free(struct pool *pp, void *addr)
{

	uvm_km_free(kernel_map, (vaddr_t)addr, NCARGS, UVM_KMF_PAGEABLE);
}

static struct pool exec_pool;

static struct pool_allocator exec_palloc = {
	.pa_alloc = exec_pool_alloc,
	.pa_free = exec_pool_free,
	.pa_pagesz = NCARGS
};

/*
 * check exec:
 * given an "executable" described in the exec package's namei info,
 * see what we can do with it.
 *
 * ON ENTRY:
 *	exec package with appropriate namei info
 *	lwp pointer of exec'ing lwp
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
/*ARGSUSED*/
check_exec(struct lwp *l, struct exec_package *epp)
{
	int		error, i;
	struct vnode	*vp;
	struct nameidata *ndp;
	size_t		resid;

	ndp = epp->ep_ndp;
	ndp->ni_cnd.cn_nameiop = LOOKUP;
	ndp->ni_cnd.cn_flags = FOLLOW | LOCKLEAF | SAVENAME | TRYEMULROOT;
	/* first get the vnode */
	if ((error = namei(ndp)) != 0)
		return error;
	epp->ep_vp = vp = ndp->ni_vp;

	/* check access and type */
	if (vp->v_type != VREG) {
		error = EACCES;
		goto bad1;
	}
	if ((error = VOP_ACCESS(vp, VEXEC, l->l_cred)) != 0)
		goto bad1;

	/* get attributes */
	if ((error = VOP_GETATTR(vp, epp->ep_vap, l->l_cred)) != 0)
		goto bad1;

	/* Check mount point */
	if (vp->v_mount->mnt_flag & MNT_NOEXEC) {
		error = EACCES;
		goto bad1;
	}
	if (vp->v_mount->mnt_flag & MNT_NOSUID)
		epp->ep_vap->va_mode &= ~(S_ISUID | S_ISGID);

	/* try to open it */
	if ((error = VOP_OPEN(vp, FREAD, l->l_cred)) != 0)
		goto bad1;

	/* unlock vp, since we need it unlocked from here on out. */
	VOP_UNLOCK(vp, 0);

#if NVERIEXEC > 0
	error = veriexec_verify(l, vp, ndp->ni_cnd.cn_pnbuf,
	    epp->ep_flags & EXEC_INDIR ? VERIEXEC_INDIRECT : VERIEXEC_DIRECT,
	    NULL);
	if (error)
		goto bad2;
#endif /* NVERIEXEC > 0 */

#ifdef PAX_SEGVGUARD
	error = pax_segvguard(l, vp, ndp->ni_cnd.cn_pnbuf, false);
	if (error)
		goto bad2;
#endif /* PAX_SEGVGUARD */

	/* now we have the file, get the exec header */
	error = vn_rdwr(UIO_READ, vp, epp->ep_hdr, epp->ep_hdrlen, 0,
			UIO_SYSSPACE, 0, l->l_cred, &resid, NULL);
	if (error)
		goto bad2;
	epp->ep_hdrvalid = epp->ep_hdrlen - resid;

	/*
	 * Set up default address space limits.  Can be overridden
	 * by individual exec packages.
	 *
	 * XXX probably should be all done in the exec packages.
	 */
	epp->ep_vm_minaddr = VM_MIN_ADDRESS;
	epp->ep_vm_maxaddr = VM_MAXUSER_ADDRESS;
	/*
	 * set up the vmcmds for creation of the process
	 * address space
	 */
	error = ENOEXEC;
	for (i = 0; i < nexecs; i++) {
		int newerror;

		epp->ep_esch = execsw[i];
		newerror = (*execsw[i]->es_makecmds)(l, epp);

		if (!newerror) {
			/* Seems ok: check that entry point is sane */
			if (epp->ep_entry > VM_MAXUSER_ADDRESS) {
				error = ENOEXEC;
				break;
			}

			/* check limits */
			if ((epp->ep_tsize > MAXTSIZ) ||
			    (epp->ep_dsize > (u_quad_t)l->l_proc->p_rlimit
						    [RLIMIT_DATA].rlim_cur)) {
				error = ENOMEM;
				break;
			}
			return 0;
		}

		if (epp->ep_emul_root != NULL) {
			vrele(epp->ep_emul_root);
			epp->ep_emul_root = NULL;
		}
		if (epp->ep_interp != NULL) {
			vrele(epp->ep_interp);
			epp->ep_interp = NULL;
		}

		/* make sure the first "interesting" error code is saved. */
		if (error == ENOEXEC)
			error = newerror;

		if (epp->ep_flags & EXEC_DESTR)
			/* Error from "#!" code, tidied up by recursive call */
			return error;
	}

	/* not found, error */

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
	VOP_CLOSE(vp, FREAD, l->l_cred);
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

static int
execve_fetch_element(char * const *array, size_t index, char **value)
{
	return copyin(array + index, value, sizeof(*value));
}

/*
 * exec system call
 */
/* ARGSUSED */
int
sys_execve(struct lwp *l, const struct sys_execve_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *)	path;
		syscallarg(char * const *)	argp;
		syscallarg(char * const *)	envp;
	} */

	return execve1(l, SCARG(uap, path), SCARG(uap, argp),
	    SCARG(uap, envp), execve_fetch_element);
}

/*
 * Load modules to try and execute an image that we do not understand.
 * If no execsw entries are present, we load those likely to be needed
 * in order to run native images only.  Otherwise, we autoload all
 * possible modules that could let us run the binary.  XXX lame
 */
static void
exec_autoload(void)
{
#ifdef MODULAR
	static const char * const native[] = {
		"exec_elf32",
		"exec_elf64",
		"exec_script",
		NULL
	};
	static const char * const compat[] = {
		"exec_elf32",
		"exec_elf64",
		"exec_script",
		"exec_aout",
		"exec_coff",
		"exec_ecoff",
		"compat_aoutm68k",
		"compat_freebsd",
		"compat_ibcs2",
		"compat_irix",
		"compat_linux",
		"compat_linux32",
		"compat_netbsd32",
		"compat_sunos",
		"compat_sunos32",
		"compat_svr4",
		"compat_svr4_32",
		"compat_ultrix",
		NULL
	};
	char const * const *list;
	int i;

	mutex_enter(&module_lock);
	list = (nexecs == 0 ? native : compat);
	for (i = 0; list[i] != NULL; i++) {
		if (module_autoload(list[i], MODULE_CLASS_MISC) != 0) {
		    	continue;
		}
		mutex_exit(&module_lock);
	   	yield();
		mutex_enter(&module_lock);
	}
	mutex_exit(&module_lock);
#endif
}

int
execve1(struct lwp *l, const char *path, char * const *args,
    char * const *envs, execve_fetch_element_t fetch_element)
{
	int			error;
	struct exec_package	pack;
	struct nameidata	nid;
	struct vattr		attr;
	struct proc		*p;
	char			*argp;
	char			*dp, *sp;
	long			argc, envc;
	size_t			i, len;
	char			*stack;
	struct ps_strings	arginfo;
	struct ps_strings	*aip = &arginfo;
	struct vmspace		*vm;
	struct exec_fakearg	*tmpfap;
	int			szsigcode;
	struct exec_vmcmd	*base_vcp;
	int			oldlwpflags;
	ksiginfo_t		ksi;
	ksiginfoq_t		kq;
	char			*pathbuf;
	size_t			pathbuflen;
	u_int			modgen;

	p = l->l_proc;
 	modgen = 0;

	/*
	 * Check if we have exceeded our number of processes limit.
	 * This is so that we handle the case where a root daemon
	 * forked, ran setuid to become the desired user and is trying
	 * to exec. The obvious place to do the reference counting check
	 * is setuid(), but we don't do the reference counting check there
	 * like other OS's do because then all the programs that use setuid()
	 * must be modified to check the return code of setuid() and exit().
	 * It is dangerous to make setuid() fail, because it fails open and
	 * the program will continue to run as root. If we make it succeed
	 * and return an error code, again we are not enforcing the limit.
	 * The best place to enforce the limit is here, when the process tries
	 * to execute a new image, because eventually the process will need
	 * to call exec in order to do something useful.
	 */
 retry:
	if ((p->p_flag & PK_SUGID) && kauth_authorize_generic(l->l_cred,
	    KAUTH_GENERIC_ISSUSER, NULL) != 0 && chgproccnt(kauth_cred_getuid(
	    l->l_cred), 0) > p->p_rlimit[RLIMIT_NPROC].rlim_cur)
		return EAGAIN;

	oldlwpflags = l->l_flag & (LW_SA | LW_SA_UPCALL);
	if (l->l_flag & LW_SA) {
		lwp_lock(l);
		l->l_flag &= ~(LW_SA | LW_SA_UPCALL);
		lwp_unlock(l);
	}

	/*
	 * Drain existing references and forbid new ones.  The process
	 * should be left alone until we're done here.  This is necessary
	 * to avoid race conditions - e.g. in ptrace() - that might allow
	 * a local user to illicitly obtain elevated privileges.
	 */
	rw_enter(&p->p_reflock, RW_WRITER);

	base_vcp = NULL;
	/*
	 * Init the namei data to point the file user's program name.
	 * This is done here rather than in check_exec(), so that it's
	 * possible to override this settings if any of makecmd/probe
	 * functions call check_exec() recursively - for example,
	 * see exec_script_makecmds().
	 */
	pathbuf = PNBUF_GET();
	error = copyinstr(path, pathbuf, MAXPATHLEN, &pathbuflen);
	if (error) {
		DPRINTF(("execve: copyinstr path %d", error));
		goto clrflg;
	}

	NDINIT(&nid, LOOKUP, NOFOLLOW | TRYEMULROOT, UIO_SYSSPACE, pathbuf);

	/*
	 * initialize the fields of the exec package.
	 */
	pack.ep_name = path;
	pack.ep_hdr = kmem_alloc(exec_maxhdrsz, KM_SLEEP);
	pack.ep_hdrlen = exec_maxhdrsz;
	pack.ep_hdrvalid = 0;
	pack.ep_ndp = &nid;
	pack.ep_emul_arg = NULL;
	pack.ep_vmcmds.evs_cnt = 0;
	pack.ep_vmcmds.evs_used = 0;
	pack.ep_vap = &attr;
	pack.ep_flags = 0;
	pack.ep_emul_root = NULL;
	pack.ep_interp = NULL;
	pack.ep_esch = NULL;
	pack.ep_pax_flags = 0;

	rw_enter(&exec_lock, RW_READER);

	/* see if we can run it. */
	if ((error = check_exec(l, &pack)) != 0) {
		if (error != ENOENT) {
			DPRINTF(("execve: check exec failed %d\n", error));
		}
		goto freehdr;
	}

	/* XXX -- THE FOLLOWING SECTION NEEDS MAJOR CLEANUP */

	/* allocate an argument buffer */
	argp = pool_get(&exec_pool, PR_WAITOK);
	KASSERT(argp != NULL);
	dp = argp;
	argc = 0;

	/* copy the fake args list, if there's one, freeing it as we go */
	if (pack.ep_flags & EXEC_HASARGL) {
		tmpfap = pack.ep_fa;
		while (tmpfap->fa_arg != NULL) {
			const char *cp;

			cp = tmpfap->fa_arg;
			while (*cp)
				*dp++ = *cp++;
			*dp++ = '\0';

			kmem_free(tmpfap->fa_arg, tmpfap->fa_len);
			tmpfap++; argc++;
		}
		kmem_free(pack.ep_fa, pack.ep_fa_len);
		pack.ep_flags &= ~EXEC_HASARGL;
	}

	/* Now get argv & environment */
	if (args == NULL) {
		DPRINTF(("execve: null args\n"));
		error = EINVAL;
		goto bad;
	}
	/* 'i' will index the argp/envp element to be retrieved */
	i = 0;
	if (pack.ep_flags & EXEC_SKIPARG)
		i++;

	while (1) {
		len = argp + ARG_MAX - dp;
		if ((error = (*fetch_element)(args, i, &sp)) != 0) {
			DPRINTF(("execve: fetch_element args %d\n", error));
			goto bad;
		}
		if (!sp)
			break;
		if ((error = copyinstr(sp, dp, len, &len)) != 0) {
			DPRINTF(("execve: copyinstr args %d\n", error));
			if (error == ENAMETOOLONG)
				error = E2BIG;
			goto bad;
		}
		ktrexecarg(dp, len - 1);
		dp += len;
		i++;
		argc++;
	}

	envc = 0;
	/* environment need not be there */
	if (envs != NULL) {
		i = 0;
		while (1) {
			len = argp + ARG_MAX - dp;
			if ((error = (*fetch_element)(envs, i, &sp)) != 0) {
				DPRINTF(("execve: fetch_element env %d\n", error));
				goto bad;
			}
			if (!sp)
				break;
			if ((error = copyinstr(sp, dp, len, &len)) != 0) {
				DPRINTF(("execve: copyinstr env %d\n", error));
				if (error == ENAMETOOLONG)
					error = E2BIG;
				goto bad;
			}
			ktrexecenv(dp, len - 1);
			dp += len;
			i++;
			envc++;
		}
	}

	dp = (char *) ALIGN(dp);

	szsigcode = pack.ep_esch->es_emul->e_esigcode -
	    pack.ep_esch->es_emul->e_sigcode;

#ifdef __MACHINE_STACK_GROWS_UP
/* See big comment lower down */
#define	RTLD_GAP	32
#else
#define	RTLD_GAP	0
#endif

	/* Now check if args & environ fit into new stack */
	if (pack.ep_flags & EXEC_32)
		len = ((argc + envc + 2 + pack.ep_esch->es_arglen) *
		    sizeof(int) + sizeof(int) + dp + RTLD_GAP +
		    szsigcode + sizeof(struct ps_strings) + STACK_PTHREADSPACE)
		    - argp;
	else
		len = ((argc + envc + 2 + pack.ep_esch->es_arglen) *
		    sizeof(char *) + sizeof(int) + dp + RTLD_GAP +
		    szsigcode + sizeof(struct ps_strings) + STACK_PTHREADSPACE)
		    - argp;

#ifdef PAX_ASLR
	if (pax_aslr_active(l))
		len += (arc4random() % PAGE_SIZE);
#endif /* PAX_ASLR */

#ifdef STACKLALIGN	/* arm, etc. */
	len = STACKALIGN(len);	/* make the stack "safely" aligned */
#else
	len = ALIGN(len);	/* make the stack "safely" aligned */
#endif

	if (len > pack.ep_ssize) { /* in effect, compare to initial limit */
		DPRINTF(("execve: stack limit exceeded %zu\n", len));
		error = ENOMEM;
		goto bad;
	}

	/* Get rid of other LWPs. */
	if (p->p_sa || p->p_nlwps > 1) {
		mutex_enter(p->p_lock);
		exit_lwps(l);
		mutex_exit(p->p_lock);
	}
	KDASSERT(p->p_nlwps == 1);

	/* Destroy any lwpctl info. */
	if (p->p_lwpctl != NULL)
		lwp_ctl_exit();

	/* This is now LWP 1 */
	l->l_lid = 1;
	p->p_nlwpid = 1;

#ifdef KERN_SA
	/* Release any SA state. */
	if (p->p_sa)
		sa_release(p);
#endif /* KERN_SA */

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
	vm->vm_taddr = (void *)pack.ep_taddr;
	vm->vm_tsize = btoc(pack.ep_tsize);
	vm->vm_daddr = (void*)pack.ep_daddr;
	vm->vm_dsize = btoc(pack.ep_dsize);
	vm->vm_ssize = btoc(pack.ep_ssize);
	vm->vm_issize = 0;
	vm->vm_maxsaddr = (void *)pack.ep_maxsaddr;
	vm->vm_minsaddr = (void *)pack.ep_minsaddr;

#ifdef PAX_ASLR
	pax_aslr_init(l, vm);
#endif /* PAX_ASLR */

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
		error = (*vcp->ev_proc)(l, vcp);
#ifdef DEBUG_EXEC
		if (error) {
			size_t j;
			struct exec_vmcmd *vp = &pack.ep_vmcmds.evs_cmds[0];
			for (j = 0; j <= i; j++)
				uprintf(
			"vmcmd[%zu] = %#lx/%#lx fd@%#lx prot=0%o flags=%d\n",
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
	VOP_CLOSE(pack.ep_vp, FREAD, l->l_cred);
	vput(pack.ep_vp);

	/* if an error happened, deallocate and punt */
	if (error) {
		DPRINTF(("execve: vmcmd %zu failed: %d\n", i - 1, error));
		goto exec_abort;
	}

	/* remember information about the process */
	arginfo.ps_nargvstr = argc;
	arginfo.ps_nenvstr = envc;

	/* set command name & other accounting info */
	i = min(nid.ni_cnd.cn_namelen, MAXCOMLEN);
	(void)memcpy(p->p_comm, nid.ni_cnd.cn_nameptr, i);
	p->p_comm[i] = '\0';

	dp = PNBUF_GET();
	/*
	 * If the path starts with /, we don't need to do any work.
	 * This handles the majority of the cases.
	 * In the future perhaps we could canonicalize it?
	 */
	if (pathbuf[0] == '/')
		(void)strlcpy(pack.ep_path = dp, pathbuf, MAXPATHLEN);
#ifdef notyet
	/*
	 * Although this works most of the time [since the entry was just
	 * entered in the cache] we don't use it because it theoretically
	 * can fail and it is not the cleanest interface, because there
	 * could be races. When the namei cache is re-written, this can
	 * be changed to use the appropriate function.
	 */
	else if (!(error = vnode_to_path(dp, MAXPATHLEN, p->p_textvp, l, p)))
		pack.ep_path = dp;
#endif
	else {
#ifdef notyet
		printf("Cannot get path for pid %d [%s] (error %d)",
		    (int)p->p_pid, p->p_comm, error);
#endif
		pack.ep_path = NULL;
		PNBUF_PUT(dp);
	}

	stack = (char *)STACK_ALLOC(STACK_GROW(vm->vm_minsaddr,
		STACK_PTHREADSPACE + sizeof(struct ps_strings) + szsigcode),
		len - (sizeof(struct ps_strings) + szsigcode));

#ifdef __MACHINE_STACK_GROWS_UP
	/*
	 * The copyargs call always copies into lower addresses
	 * first, moving towards higher addresses, starting with
	 * the stack pointer that we give.  When the stack grows
	 * down, this puts argc/argv/envp very shallow on the
	 * stack, right at the first user stack pointer.
	 * When the stack grows up, the situation is reversed.
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
	 * out and steal 32 bytes before we call copyargs.
	 * This extra space was allowed for when 'len' was calculated.
	 */
	stack += RTLD_GAP;
#endif /* __MACHINE_STACK_GROWS_UP */

	/* Now copy argc, args & environ to new stack */
	error = (*pack.ep_esch->es_copyargs)(l, &pack, &arginfo, &stack, argp);
	if (pack.ep_path) {
		PNBUF_PUT(pack.ep_path);
		pack.ep_path = NULL;
	}
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
	if ((error = copyout(aip, (char *)p->p_psstr,
	    sizeof(arginfo))) != 0) {
		DPRINTF(("execve: ps_strings copyout %p->%p size %ld failed\n",
		       aip, (char *)p->p_psstr, (long)sizeof(arginfo)));
		goto exec_abort;
	}

	fd_closeexec();		/* handle close on exec */
	execsigs(p);		/* reset catched signals */

	l->l_ctxlink = NULL;	/* reset ucontext link */


	p->p_acflag &= ~AFORK;
	mutex_enter(p->p_lock);
	p->p_flag |= PK_EXEC;
	mutex_exit(p->p_lock);

	/*
	 * Stop profiling.
	 */
	if ((p->p_stflag & PST_PROFIL) != 0) {
		mutex_spin_enter(&p->p_stmutex);
		stopprofclock(p);
		mutex_spin_exit(&p->p_stmutex);
	}

	/*
	 * It's OK to test PL_PPWAIT unlocked here, as other LWPs have
	 * exited and exec()/exit() are the only places it will be cleared.
	 */
	if ((p->p_lflag & PL_PPWAIT) != 0) {
		mutex_enter(proc_lock);
		p->p_lflag &= ~PL_PPWAIT;
		cv_broadcast(&p->p_pptr->p_waitcv);
		mutex_exit(proc_lock);
	}

	/*
	 * Deal with set[ug]id.  MNT_NOSUID has already been used to disable
	 * s[ug]id.  It's OK to check for PSL_TRACED here as we have blocked
	 * out additional references on the process for the moment.
	 */
	if ((p->p_slflag & PSL_TRACED) == 0 &&

	    (((attr.va_mode & S_ISUID) != 0 &&
	      kauth_cred_geteuid(l->l_cred) != attr.va_uid) ||

	     ((attr.va_mode & S_ISGID) != 0 &&
	      kauth_cred_getegid(l->l_cred) != attr.va_gid))) {
		/*
		 * Mark the process as SUGID before we do
		 * anything that might block.
		 */
		proc_crmod_enter();
		proc_crmod_leave(NULL, NULL, true);

		/* Make sure file descriptors 0..2 are in use. */
		if ((error = fd_checkstd()) != 0) {
			DPRINTF(("execve: fdcheckstd failed %d\n", error));
			goto exec_abort;
		}

		/*
		 * Copy the credential so other references don't see our
		 * changes.
		 */
		l->l_cred = kauth_cred_copy(l->l_cred);
#ifdef KTRACE
		/*
		 * If the persistent trace flag isn't set, turn off.
		 */
		if (p->p_tracep) {
			mutex_enter(&ktrace_lock);
			if (!(p->p_traceflag & KTRFAC_PERSISTENT))
				ktrderef(p);
			mutex_exit(&ktrace_lock);
		}
#endif
		if (attr.va_mode & S_ISUID)
			kauth_cred_seteuid(l->l_cred, attr.va_uid);
		if (attr.va_mode & S_ISGID)
			kauth_cred_setegid(l->l_cred, attr.va_gid);
	} else {
		if (kauth_cred_geteuid(l->l_cred) ==
		    kauth_cred_getuid(l->l_cred) &&
		    kauth_cred_getegid(l->l_cred) ==
		    kauth_cred_getgid(l->l_cred))
			p->p_flag &= ~PK_SUGID;
	}

	/*
	 * Copy the credential so other references don't see our changes.
	 * Test to see if this is necessary first, since in the common case
	 * we won't need a private reference.
	 */
	if (kauth_cred_geteuid(l->l_cred) != kauth_cred_getsvuid(l->l_cred) ||
	    kauth_cred_getegid(l->l_cred) != kauth_cred_getsvgid(l->l_cred)) {
		l->l_cred = kauth_cred_copy(l->l_cred);
		kauth_cred_setsvuid(l->l_cred, kauth_cred_geteuid(l->l_cred));
		kauth_cred_setsvgid(l->l_cred, kauth_cred_getegid(l->l_cred));
	}

	/* Update the master credentials. */
	if (l->l_cred != p->p_cred) {
		kauth_cred_t ocred;

		kauth_cred_hold(l->l_cred);
		mutex_enter(p->p_lock);
		ocred = p->p_cred;
		p->p_cred = l->l_cred;
		mutex_exit(p->p_lock);
		kauth_cred_free(ocred);
	}

#if defined(__HAVE_RAS)
	/*
	 * Remove all RASs from the address space.
	 */
	ras_purgeall();
#endif

	doexechooks(p);

	/* setup new registers and do misc. setup. */
	(*pack.ep_esch->es_emul->e_setregs)(l, &pack, (u_long) stack);
	if (pack.ep_esch->es_setregs)
		(*pack.ep_esch->es_setregs)(l, &pack, (u_long) stack);

	/* map the process's signal trampoline code */
	if (exec_sigcode_map(p, pack.ep_esch->es_emul)) {
		DPRINTF(("execve: map sigcode failed %d\n", error));
		goto exec_abort;
	}

	pool_put(&exec_pool, argp);

	PNBUF_PUT(nid.ni_cnd.cn_pnbuf);

	/* notify others that we exec'd */
	KNOTE(&p->p_klist, NOTE_EXEC);

	kmem_free(pack.ep_hdr, pack.ep_hdrlen);

	/* The emulation root will usually have been found when we looked
	 * for the elf interpreter (or similar), if not look now. */
	if (pack.ep_esch->es_emul->e_path != NULL && pack.ep_emul_root == NULL)
		emul_find_root(l, &pack);

	/* Any old emulation root got removed by fdcloseexec */
	rw_enter(&p->p_cwdi->cwdi_lock, RW_WRITER);
	p->p_cwdi->cwdi_edir = pack.ep_emul_root;
	rw_exit(&p->p_cwdi->cwdi_lock);
	pack.ep_emul_root = NULL;
	if (pack.ep_interp != NULL)
		vrele(pack.ep_interp);

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
	    && p->p_emul != pack.ep_esch->es_emul)
		(*p->p_emul->e_proc_exit)(p);

	/*
	 * Call exec hook. Emulation code may NOT store reference to anything
	 * from &pack.
	 */
        if (pack.ep_esch->es_emul->e_proc_exec)
                (*pack.ep_esch->es_emul->e_proc_exec)(p, &pack);

	/* update p_emul, the old value is no longer needed */
	p->p_emul = pack.ep_esch->es_emul;

	/* ...and the same for p_execsw */
	p->p_execsw = pack.ep_esch;

#ifdef __HAVE_SYSCALL_INTERN
	(*p->p_emul->e_syscall_intern)(p);
#endif
	ktremul();

	/* Allow new references from the debugger/procfs. */
	rw_exit(&p->p_reflock);
	rw_exit(&exec_lock);

	mutex_enter(proc_lock);

	if ((p->p_slflag & (PSL_TRACED|PSL_SYSCALL)) == PSL_TRACED) {
		KSI_INIT_EMPTY(&ksi);
		ksi.ksi_signo = SIGTRAP;
		ksi.ksi_lid = l->l_lid;
		kpsignal(p, &ksi, NULL);
	}

	if (p->p_sflag & PS_STOPEXEC) {
		KERNEL_UNLOCK_ALL(l, &l->l_biglocks);
		p->p_pptr->p_nstopchild++;
		p->p_pptr->p_waited = 0;
		mutex_enter(p->p_lock);
		ksiginfo_queue_init(&kq);
		sigclearall(p, &contsigmask, &kq);
		lwp_lock(l);
		l->l_stat = LSSTOP;
		p->p_stat = SSTOP;
		p->p_nrlwps--;
		mutex_exit(p->p_lock);
		mutex_exit(proc_lock);
		mi_switch(l);
		ksiginfo_queue_drain(&kq);
		KERNEL_LOCK(l->l_biglocks, l);
	} else {
		mutex_exit(proc_lock);
	}

	PNBUF_PUT(pathbuf);
	return (EJUSTRETURN);

 bad:
	/* free the vmspace-creation commands, and release their references */
	kill_vmcmds(&pack.ep_vmcmds);
	/* kill any opened file descriptor, if necessary */
	if (pack.ep_flags & EXEC_HASFD) {
		pack.ep_flags &= ~EXEC_HASFD;
		fd_close(pack.ep_fd);
	}
	/* close and put the exec'd file */
	vn_lock(pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(pack.ep_vp, FREAD, l->l_cred);
	vput(pack.ep_vp);
	PNBUF_PUT(nid.ni_cnd.cn_pnbuf);
	pool_put(&exec_pool, argp);

 freehdr:
	kmem_free(pack.ep_hdr, pack.ep_hdrlen);
	if (pack.ep_emul_root != NULL)
		vrele(pack.ep_emul_root);
	if (pack.ep_interp != NULL)
		vrele(pack.ep_interp);

	rw_exit(&exec_lock);

 clrflg:
	lwp_lock(l);
	l->l_flag |= oldlwpflags;
	lwp_unlock(l);
	PNBUF_PUT(pathbuf);
	rw_exit(&p->p_reflock);

	if (modgen != module_gen && error == ENOEXEC) {
		modgen = module_gen;
		exec_autoload();
		goto retry;
	}

	return error;

 exec_abort:
	PNBUF_PUT(pathbuf);
	rw_exit(&p->p_reflock);
	rw_exit(&exec_lock);

	/*
	 * the old process doesn't exist anymore.  exit gracefully.
	 * get rid of the (new) address space we have created, if any, get rid
	 * of our namei data and vnode, and exit noting failure
	 */
	uvm_deallocate(&vm->vm_map, VM_MIN_ADDRESS,
		VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS);
	if (pack.ep_emul_arg)
		free(pack.ep_emul_arg, M_TEMP);
	PNBUF_PUT(nid.ni_cnd.cn_pnbuf);
	pool_put(&exec_pool, argp);
	kmem_free(pack.ep_hdr, pack.ep_hdrlen);
	if (pack.ep_emul_root != NULL)
		vrele(pack.ep_emul_root);
	if (pack.ep_interp != NULL)
		vrele(pack.ep_interp);

	/* Acquire the sched-state mutex (exit1() will release it). */
	mutex_enter(p->p_lock);
	exit1(l, W_EXITCODE(error, SIGABRT));

	/* NOTREACHED */
	return 0;
}


int
copyargs(struct lwp *l, struct exec_package *pack, struct ps_strings *arginfo,
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

	dp = (char *) (cpp + argc + envc + 2 + pack->ep_esch->es_arglen);
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


/*
 * Add execsw[] entries.
 */
int
exec_add(struct execsw *esp, int count)
{
	struct exec_entry	*it;
	int			i;

	if (count == 0) {
		return 0;
	}

	/* Check for duplicates. */
	rw_enter(&exec_lock, RW_WRITER);
	for (i = 0; i < count; i++) {
		LIST_FOREACH(it, &ex_head, ex_list) {
			/* assume unique (makecmds, probe_func, emulation) */
			if (it->ex_sw->es_makecmds == esp[i].es_makecmds &&
			    it->ex_sw->u.elf_probe_func ==
			    esp[i].u.elf_probe_func &&
			    it->ex_sw->es_emul == esp[i].es_emul) {
				rw_exit(&exec_lock);
				return EEXIST;
			}
		}
	}

	/* Allocate new entries. */
	for (i = 0; i < count; i++) {
		it = kmem_alloc(sizeof(*it), KM_SLEEP);
		it->ex_sw = &esp[i];
		LIST_INSERT_HEAD(&ex_head, it, ex_list);
	}

	/* update execsw[] */
	exec_init(0);
	rw_exit(&exec_lock);
	return 0;
}

/*
 * Remove execsw[] entry.
 */
int
exec_remove(struct execsw *esp, int count)
{
	struct exec_entry	*it, *next;
	int			i;
	const struct proclist_desc *pd;
	proc_t			*p;

	if (count == 0) {
		return 0;
	}

	/* Abort if any are busy. */
	rw_enter(&exec_lock, RW_WRITER);
	for (i = 0; i < count; i++) {
		mutex_enter(proc_lock);
		for (pd = proclists; pd->pd_list != NULL; pd++) {
			PROCLIST_FOREACH(p, pd->pd_list) {
				if (p->p_execsw == &esp[i]) {
					mutex_exit(proc_lock);
					rw_exit(&exec_lock);
					return EBUSY;
				}
			}
		}
		mutex_exit(proc_lock);
	}

	/* None are busy, so remove them all. */
	for (i = 0; i < count; i++) {
		for (it = LIST_FIRST(&ex_head); it != NULL; it = next) {
			next = LIST_NEXT(it, ex_list);
			if (it->ex_sw == &esp[i]) {
				LIST_REMOVE(it, ex_list);
				kmem_free(it, sizeof(*it));
				break;
			}
		}
	}

	/* update execsw[] */
	exec_init(0);
	rw_exit(&exec_lock);
	return 0;
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
	const struct execsw 	**sw;
	struct exec_entry	*ex;
	SLIST_HEAD(,exec_entry)	first;
	SLIST_HEAD(,exec_entry)	any;
	SLIST_HEAD(,exec_entry)	last;
	int			i, sz;

	if (init_boot) {
		/* do one-time initializations */
		rw_init(&exec_lock);
		mutex_init(&sigobject_lock, MUTEX_DEFAULT, IPL_NONE);
		pool_init(&exec_pool, NCARGS, 0, 0, PR_NOALIGN|PR_NOTOUCH,
		    "execargs", &exec_palloc, IPL_NONE);
		pool_sethardlimit(&exec_pool, maxexec, "should not happen", 0);
	} else {
		KASSERT(rw_write_held(&exec_lock));
	}

	/* Sort each entry onto the appropriate queue. */
	SLIST_INIT(&first);
	SLIST_INIT(&any);
	SLIST_INIT(&last);
	sz = 0;
	LIST_FOREACH(ex, &ex_head, ex_list) {
		switch(ex->ex_sw->es_prio) {
		case EXECSW_PRIO_FIRST:
			SLIST_INSERT_HEAD(&first, ex, ex_slist);
			break;
		case EXECSW_PRIO_ANY:
			SLIST_INSERT_HEAD(&any, ex, ex_slist);
			break;
		case EXECSW_PRIO_LAST:
			SLIST_INSERT_HEAD(&last, ex, ex_slist);
			break;
		default:
			panic("exec_init");
			break;
		}
		sz++;
	}

	/*
	 * Create new execsw[].  Ensure we do not try a zero-sized
	 * allocation.
	 */
	sw = kmem_alloc(sz * sizeof(struct execsw *) + 1, KM_SLEEP);
	i = 0;
	SLIST_FOREACH(ex, &first, ex_slist) {
		sw[i++] = ex->ex_sw;
	}
	SLIST_FOREACH(ex, &any, ex_slist) {
		sw[i++] = ex->ex_sw;
	}
	SLIST_FOREACH(ex, &last, ex_slist) {
		sw[i++] = ex->ex_sw;
	}

	/* Replace old execsw[] and free used memory. */
	if (execsw != NULL) {
		kmem_free(__UNCONST(execsw),
		    nexecs * sizeof(struct execsw *) + 1);
	}
	execsw = sw;
	nexecs = sz;

	/* Figure out the maximum size of an exec header. */
	exec_maxhdrsz = sizeof(int);
	for (i = 0; i < nexecs; i++) {
		if (execsw[i]->es_hdrsz > exec_maxhdrsz)
			exec_maxhdrsz = execsw[i]->es_hdrsz;
	}

	return 0;
}

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
		mutex_enter(&sigobject_lock);
		if ((uobj = *e->e_sigobject) == NULL) {
			uobj = uao_create(sz, 0);
			(*uobj->pgops->pgo_reference)(uobj);
			va = vm_map_min(kernel_map);
			if ((error = uvm_map(kernel_map, &va, round_page(sz),
			    uobj, 0, 0,
			    UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
			    UVM_INH_SHARE, UVM_ADV_RANDOM, 0)))) {
				printf("kernel mapping failed %d\n", error);
				(*uobj->pgops->pgo_detach)(uobj);
				mutex_exit(&sigobject_lock);
				return (error);
			}
			memcpy((void *)va, e->e_sigcode, sz);
#ifdef PMAP_NEED_PROCWR
			pmap_procwr(&proc0, va, sz);
#endif
			uvm_unmap(kernel_map, va, va + round_page(sz));
			*e->e_sigobject = uobj;
		}
		mutex_exit(&sigobject_lock);
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
