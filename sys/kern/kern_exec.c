/*	$NetBSD: kern_exec.c,v 1.339.2.11 2018/05/22 14:38:20 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kern_exec.c,v 1.339.2.11 2018/05/22 14:38:20 martin Exp $");

#include "opt_exec.h"
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
#include <sys/atomic.h>
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
#include <sys/sdt.h>
#include <sys/spawn.h>
#include <sys/prot.h>
#include <sys/cprng.h>

#include <uvm/uvm_extern.h>

#include <machine/reg.h>

#include <compat/common/compat_util.h>

static int exec_sigcode_map(struct proc *, const struct emul *);

#ifdef DEBUG_EXEC
#define DPRINTF(a) printf a
#define COPYPRINTF(s, a, b) printf("%s, %d: copyout%s @%p %zu\n", __func__, \
    __LINE__, (s), (a), (b))
#else
#define DPRINTF(a)
#define COPYPRINTF(s, a, b)
#endif /* DEBUG_EXEC */

/*
 * DTrace SDT provider definitions
 */
SDT_PROBE_DEFINE(proc,,,exec, 
	    "char *", NULL,
	    NULL, NULL, NULL, NULL,
	    NULL, NULL, NULL, NULL);
SDT_PROBE_DEFINE(proc,,,exec_success, 
	    "char *", NULL,
	    NULL, NULL, NULL, NULL,
	    NULL, NULL, NULL, NULL);
SDT_PROBE_DEFINE(proc,,,exec_failure, 
	    "int", NULL,
	    NULL, NULL, NULL, NULL,
	    NULL, NULL, NULL, NULL);

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
	.e_name =		"netbsd",
#ifdef EMUL_NATIVEROOT
	.e_path =		EMUL_NATIVEROOT,
#else
	.e_path =		NULL,
#endif
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		EMUL_HAS_SYS___syscall,
	.e_errno =		NULL,
	.e_nosys =		SYS_syscall,
	.e_nsysent =		SYS_NSYSENT,
#endif
	.e_sysent =		sysent,
#ifdef SYSCALL_DEBUG
	.e_syscallnames =	syscallnames,
#else
	.e_syscallnames =	NULL,
#endif
	.e_sendsig =		sendsig,
	.e_trapsignal =		trapsignal,
	.e_tracesig =		NULL,
	.e_sigcode =		NULL,
	.e_esigcode =		NULL,
	.e_sigobject =		NULL,
	.e_setregs =		setregs,
	.e_proc_exec =		NULL,
	.e_proc_fork =		NULL,
	.e_proc_exit =		NULL,
	.e_lwp_fork =		NULL,
	.e_lwp_exit =		NULL,
#ifdef __HAVE_SYSCALL_INTERN
	.e_syscall_intern =	syscall_intern,
#else
	.e_syscall =		syscall,
#endif
	.e_sysctlovly =		NULL,
	.e_fault =		NULL,
	.e_vm_default_addr =	uvm_default_mapaddr,
	.e_usertrap =		NULL,
#ifdef KERN_SA
	.e_sa =			&saemul_netbsd,
#else
	.e_sa =			NULL,
#endif
	.e_ucsize =		sizeof(ucontext_t),
	.e_startlwp =		startlwp
};

/*
 * Exec lock. Used to control access to execsw[] structures.
 * This must not be static so that netbsd32 can access it, too.
 */
krwlock_t exec_lock;

static kmutex_t sigobject_lock;

/*
 * Data used between a loadvm and execve part of an "exec" operation
 */
struct execve_data {
	struct exec_package	ed_pack;
	struct pathbuf		*ed_pathbuf;
	struct vattr		ed_attr;
	struct ps_strings	ed_arginfo;
	char			*ed_argp;
	const char		*ed_pathstring;
	char			*ed_resolvedpathbuf;
	size_t			ed_ps_strings_sz;
	int			ed_szsigcode;
	long			ed_argc;
	long			ed_envc;
};

/*
 * data passed from parent lwp to child during a posix_spawn()
 */
struct spawn_exec_data {
	struct execve_data	sed_exec;
	struct posix_spawn_file_actions
				*sed_actions;
	struct posix_spawnattr	*sed_attrs;
	struct proc		*sed_parent;
	kcondvar_t		sed_cv_child_ready;
	kmutex_t		sed_mtx_child;
	int			sed_error;
	volatile uint32_t	sed_refcnt;
};

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
check_exec(struct lwp *l, struct exec_package *epp, struct pathbuf *pb)
{
	int		error, i;
	struct vnode	*vp;
	struct nameidata nd;
	size_t		resid;

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, pb);

	/* first get the vnode */
	if ((error = namei(&nd)) != 0)
		return error;
	epp->ep_vp = vp = nd.ni_vp;
	/* this cannot overflow as both are size PATH_MAX */
	strcpy(epp->ep_resolvedname, nd.ni_pnbuf);

#ifdef DIAGNOSTIC
	/* paranoia (take this out once namei stuff stabilizes) */
	memset(nd.ni_pnbuf, '~', PATH_MAX);
#endif

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
	VOP_UNLOCK(vp);

#if NVERIEXEC > 0
	error = veriexec_verify(l, vp, epp->ep_resolvedname,
	    epp->ep_flags & EXEC_INDIR ? VERIEXEC_INDIRECT : VERIEXEC_DIRECT,
	    NULL);
	if (error)
		goto bad2;
#endif /* NVERIEXEC > 0 */

#ifdef PAX_SEGVGUARD
	error = pax_segvguard(l, vp, epp->ep_resolvedname, false);
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
			/* Seems ok: check that entry point is not too high */
			if (epp->ep_entry > epp->ep_vm_maxaddr) {
#ifdef DIAGNOSTIC
				printf("%s: rejecting %p due to "
				    "too high entry address (> %p)\n",
					 __func__, (void *)epp->ep_entry,
					 (void *)epp->ep_vm_maxaddr);
#endif
				error = ENOEXEC;
				break;
			}
			/* Seems ok: check that entry point is not too low */
			if (epp->ep_entry < epp->ep_vm_minaddr) {
#ifdef DIAGNOSTIC
				printf("%s: rejecting %p due to "
				    "too low entry address (< %p)\n",
				     __func__, (void *)epp->ep_entry,
				     (void *)epp->ep_vm_minaddr);
#endif
				error = ENOEXEC;
				break;
			}

			/* check limits */
			if ((epp->ep_tsize > MAXTSIZ) ||
			    (epp->ep_dsize > (u_quad_t)l->l_proc->p_rlimit
						    [RLIMIT_DATA].rlim_cur)) {
#ifdef DIAGNOSTIC
				printf("%s: rejecting due to "
				    "limits (t=%llu > %llu || d=%llu > %llu)\n",
				    __func__,
				    (unsigned long long)epp->ep_tsize,
				    (unsigned long long)MAXTSIZ,
				    (unsigned long long)epp->ep_dsize,
				    (unsigned long long)
				    l->l_proc->p_rlimit[RLIMIT_DATA].rlim_cur);
#endif
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
	return error;

bad1:
	/*
	 * free the namei pathname buffer, and put the vnode
	 * (which we don't yet have open).
	 */
	vput(vp);				/* was still locked */
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

int   
sys_fexecve(struct lwp *l, const struct sys_fexecve_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int)			fd;
		syscallarg(char * const *)	argp;
		syscallarg(char * const *)	envp;
	} */

	return ENOSYS;
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
		"compat_linux",
		"compat_linux32",
		"compat_netbsd32",
		"compat_sunos",
		"compat_sunos32",
		"compat_ultrix",
		NULL
	};
	char const * const *list;
	int i;

	list = (nexecs == 0 ? native : compat);
	for (i = 0; list[i] != NULL; i++) {
		if (module_autoload(list[i], MODULE_CLASS_MISC) != 0) {
		    	continue;
		}
	   	yield();
	}
#endif
}

static int
execve_loadvm(struct lwp *l, const char *path, char * const *args,
	char * const *envs, execve_fetch_element_t fetch_element,
	struct execve_data * restrict data)
{
	int			error;
	struct proc		*p;
	char			*dp, *sp;
	size_t			i, len;
	struct exec_fakearg	*tmpfap;
	int			oldlwpflags;
	u_int			modgen;

	KASSERT(data != NULL);

	p = l->l_proc;
 	modgen = 0;

	SDT_PROBE(proc,,,exec, path, 0, 0, 0, 0);

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

	/*
	 * Init the namei data to point the file user's program name.
	 * This is done here rather than in check_exec(), so that it's
	 * possible to override this settings if any of makecmd/probe
	 * functions call check_exec() recursively - for example,
	 * see exec_script_makecmds().
	 */
	error = pathbuf_copyin(path, &data->ed_pathbuf);
	if (error) {
		DPRINTF(("%s: pathbuf_copyin path @%p %d\n", __func__,
		    path, error));
		goto clrflg;
	}
	data->ed_pathstring = pathbuf_stringcopy_get(data->ed_pathbuf);

	data->ed_resolvedpathbuf = PNBUF_GET();
#ifdef DIAGNOSTIC
	strcpy(data->ed_resolvedpathbuf, "/wrong");
#endif

	/*
	 * initialize the fields of the exec package.
	 */
	data->ed_pack.ep_name = path;
	data->ed_pack.ep_kname = data->ed_pathstring;
	data->ed_pack.ep_resolvedname = data->ed_resolvedpathbuf;
	data->ed_pack.ep_hdr = kmem_alloc(exec_maxhdrsz, KM_SLEEP);
	data->ed_pack.ep_hdrlen = exec_maxhdrsz;
	data->ed_pack.ep_hdrvalid = 0;
	data->ed_pack.ep_emul_arg = NULL;
	data->ed_pack.ep_emul_arg_free = NULL;
	data->ed_pack.ep_vmcmds.evs_cnt = 0;
	data->ed_pack.ep_vmcmds.evs_used = 0;
	data->ed_pack.ep_vap = &data->ed_attr;
	data->ed_pack.ep_flags = 0;
	data->ed_pack.ep_emul_root = NULL;
	data->ed_pack.ep_interp = NULL;
	data->ed_pack.ep_esch = NULL;
	data->ed_pack.ep_pax_flags = 0;

	rw_enter(&exec_lock, RW_READER);

	/* see if we can run it. */
	if ((error = check_exec(l, &data->ed_pack, data->ed_pathbuf)) != 0) {
		if (error != ENOENT) {
			DPRINTF(("%s: check exec failed %d\n",
			    __func__, error));
		}
		goto freehdr;
	}

	/* XXX -- THE FOLLOWING SECTION NEEDS MAJOR CLEANUP */

	/* allocate an argument buffer */
	data->ed_argp = pool_get(&exec_pool, PR_WAITOK);
	KASSERT(data->ed_argp != NULL);
	dp = data->ed_argp;
	data->ed_argc = 0;

	/* copy the fake args list, if there's one, freeing it as we go */
	if (data->ed_pack.ep_flags & EXEC_HASARGL) {
		tmpfap = data->ed_pack.ep_fa;
		while (tmpfap->fa_arg != NULL) {
			const char *cp;

			cp = tmpfap->fa_arg;
			while (*cp)
				*dp++ = *cp++;
			*dp++ = '\0';
			ktrexecarg(tmpfap->fa_arg, cp - tmpfap->fa_arg);

			kmem_free(tmpfap->fa_arg, tmpfap->fa_len);
			tmpfap++; data->ed_argc++;
		}
		kmem_free(data->ed_pack.ep_fa, data->ed_pack.ep_fa_len);
		data->ed_pack.ep_flags &= ~EXEC_HASARGL;
	}

	/* Now get argv & environment */
	if (args == NULL) {
		DPRINTF(("%s: null args\n", __func__));
		error = EINVAL;
		goto bad;
	}
	/* 'i' will index the argp/envp element to be retrieved */
	i = 0;
	if (data->ed_pack.ep_flags & EXEC_SKIPARG)
		i++;

	while (1) {
		len = data->ed_argp + ARG_MAX - dp;
		if ((error = (*fetch_element)(args, i, &sp)) != 0) {
			DPRINTF(("%s: fetch_element args %d\n",
			    __func__, error));
			goto bad;
		}
		if (!sp)
			break;
		if ((error = copyinstr(sp, dp, len, &len)) != 0) {
			DPRINTF(("%s: copyinstr args %d\n", __func__, error));
			if (error == ENAMETOOLONG)
				error = E2BIG;
			goto bad;
		}
		ktrexecarg(dp, len - 1);
		dp += len;
		i++;
		data->ed_argc++;
	}

	data->ed_envc = 0;
	/* environment need not be there */
	if (envs != NULL) {
		i = 0;
		while (1) {
			len = data->ed_argp + ARG_MAX - dp;
			if ((error = (*fetch_element)(envs, i, &sp)) != 0) {
				DPRINTF(("%s: fetch_element env %d\n",
				    __func__, error));
				goto bad;
			}
			if (!sp)
				break;
			if ((error = copyinstr(sp, dp, len, &len)) != 0) {
				DPRINTF(("%s: copyinstr env %d\n",
				    __func__, error));
				if (error == ENAMETOOLONG)
					error = E2BIG;
				goto bad;
			}

			ktrexecenv(dp, len - 1);
			dp += len;
			i++;
			data->ed_envc++;
		}
	}

	dp = (char *) ALIGN(dp);

	data->ed_szsigcode = data->ed_pack.ep_esch->es_emul->e_esigcode -
	    data->ed_pack.ep_esch->es_emul->e_sigcode;

#ifdef __MACHINE_STACK_GROWS_UP
/* See big comment lower down */
#define	RTLD_GAP	32
#else
#define	RTLD_GAP	0
#endif

	/* Now check if args & environ fit into new stack */
	if (data->ed_pack.ep_flags & EXEC_32) {
		data->ed_ps_strings_sz = sizeof(struct ps_strings32);
		len = ((data->ed_argc + data->ed_envc + 2 +
		    data->ed_pack.ep_esch->es_arglen) *
		    sizeof(int) + sizeof(int) + dp + RTLD_GAP +
		    data->ed_szsigcode + data->ed_ps_strings_sz + STACK_PTHREADSPACE)
		    - data->ed_argp;
	} else {
		data->ed_ps_strings_sz = sizeof(struct ps_strings);
		len = ((data->ed_argc + data->ed_envc + 2 +
		    data->ed_pack.ep_esch->es_arglen) *
		    sizeof(char *) + sizeof(int) + dp + RTLD_GAP +
		    data->ed_szsigcode + data->ed_ps_strings_sz + STACK_PTHREADSPACE)
		    - data->ed_argp;
	}

#ifdef PAX_ASLR
	if (pax_aslr_active(l))
		len += (cprng_fast32() % PAGE_SIZE);
#endif /* PAX_ASLR */

	/* make the stack "safely" aligned */
	len = STACK_LEN_ALIGN(len, STACK_ALIGNBYTES);

	if (len > data->ed_pack.ep_ssize) {
		/* in effect, compare to initial limit */
		DPRINTF(("%s: stack limit exceeded %zu\n", __func__, len));
		error = ENOMEM;
		goto bad;
	}
	/* adjust "active stack depth" for process VSZ */
	data->ed_pack.ep_ssize = len;

	return 0;

 bad:
	/* free the vmspace-creation commands, and release their references */
	kill_vmcmds(&data->ed_pack.ep_vmcmds);
	/* kill any opened file descriptor, if necessary */
	if (data->ed_pack.ep_flags & EXEC_HASFD) {
		data->ed_pack.ep_flags &= ~EXEC_HASFD;
		fd_close(data->ed_pack.ep_fd);
	}
	/* close and put the exec'd file */
	vn_lock(data->ed_pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(data->ed_pack.ep_vp, FREAD, l->l_cred);
	vput(data->ed_pack.ep_vp);
	pool_put(&exec_pool, data->ed_argp);

 freehdr:
	kmem_free(data->ed_pack.ep_hdr, data->ed_pack.ep_hdrlen);
	if (data->ed_pack.ep_emul_root != NULL)
		vrele(data->ed_pack.ep_emul_root);
	if (data->ed_pack.ep_interp != NULL)
		vrele(data->ed_pack.ep_interp);

	rw_exit(&exec_lock);

	pathbuf_stringcopy_put(data->ed_pathbuf, data->ed_pathstring);
	pathbuf_destroy(data->ed_pathbuf);
	PNBUF_PUT(data->ed_resolvedpathbuf);

 clrflg:
	lwp_lock(l);
	l->l_flag |= oldlwpflags;
	lwp_unlock(l);
	rw_exit(&p->p_reflock);

	if (modgen != module_gen && error == ENOEXEC) {
		modgen = module_gen;
		exec_autoload();
		goto retry;
	}

	SDT_PROBE(proc,,,exec_failure, error, 0, 0, 0, 0);
	return error;
}

static void
execve_free_data(struct execve_data *data)
{

	/* free the vmspace-creation commands, and release their references */
	kill_vmcmds(&data->ed_pack.ep_vmcmds);
	/* kill any opened file descriptor, if necessary */
	if (data->ed_pack.ep_flags & EXEC_HASFD) {
		data->ed_pack.ep_flags &= ~EXEC_HASFD;
		fd_close(data->ed_pack.ep_fd);
	}

	/* close and put the exec'd file */
	vn_lock(data->ed_pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(data->ed_pack.ep_vp, FREAD, curlwp->l_cred);
	vput(data->ed_pack.ep_vp);
	pool_put(&exec_pool, data->ed_argp);

	kmem_free(data->ed_pack.ep_hdr, data->ed_pack.ep_hdrlen);
	if (data->ed_pack.ep_emul_root != NULL)
		vrele(data->ed_pack.ep_emul_root);
	if (data->ed_pack.ep_interp != NULL)
		vrele(data->ed_pack.ep_interp);

	pathbuf_stringcopy_put(data->ed_pathbuf, data->ed_pathstring);
	pathbuf_destroy(data->ed_pathbuf);
	PNBUF_PUT(data->ed_resolvedpathbuf);
}

static int
execve_runproc(struct lwp *l, struct execve_data * restrict data,
	bool no_local_exec_lock, bool is_spawn)
{
	int error = 0;
	struct proc		*p;
	size_t			i;
	char			*stack, *dp;
	const char		*commandname;
	struct ps_strings32	arginfo32;
	struct exec_vmcmd	*base_vcp;
	void			*aip;
	struct vmspace		*vm;
	ksiginfo_t		ksi;
	ksiginfoq_t		kq;

	/*
	 * In case of a posix_spawn operation, the child doing the exec
	 * might not hold the reader lock on exec_lock, but the parent
	 * will do this instead.
	 */
	KASSERT(no_local_exec_lock || rw_lock_held(&exec_lock));
	KASSERT(data != NULL);
	if (data == NULL)
		return (EINVAL);

	p = l->l_proc;
	if (no_local_exec_lock)
		KASSERT(is_spawn);

	base_vcp = NULL;

	if (data->ed_pack.ep_flags & EXEC_32) 
		aip = &arginfo32;
	else
		aip = &data->ed_arginfo;

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

#ifdef KERN_SA
	/* Release any SA state. */
	if (p->p_sa)
		sa_release(p);
#endif /* KERN_SA */

	/* Remove POSIX timers */
	timers_free(p, TIMERS_POSIX);

	/*
	 * Do whatever is necessary to prepare the address space
	 * for remapping.  Note that this might replace the current
	 * vmspace with another!
	 */
	if (is_spawn)
		uvmspace_spawn(l, data->ed_pack.ep_vm_minaddr,
		    data->ed_pack.ep_vm_maxaddr);
	else
		uvmspace_exec(l, data->ed_pack.ep_vm_minaddr,
		    data->ed_pack.ep_vm_maxaddr);

	/* record proc's vnode, for use by procfs and others */
        if (p->p_textvp)
                vrele(p->p_textvp);
	vref(data->ed_pack.ep_vp);
	p->p_textvp = data->ed_pack.ep_vp;

	/* Now map address space */
	vm = p->p_vmspace;
	vm->vm_taddr = (void *)data->ed_pack.ep_taddr;
	vm->vm_tsize = btoc(data->ed_pack.ep_tsize);
	vm->vm_daddr = (void*)data->ed_pack.ep_daddr;
	vm->vm_dsize = btoc(data->ed_pack.ep_dsize);
	vm->vm_ssize = btoc(data->ed_pack.ep_ssize);
	vm->vm_issize = 0;
	vm->vm_maxsaddr = (void *)data->ed_pack.ep_maxsaddr;
	vm->vm_minsaddr = (void *)data->ed_pack.ep_minsaddr;

#ifdef PAX_ASLR
	pax_aslr_init(l, vm);
#endif /* PAX_ASLR */

	/* create the new process's VM space by running the vmcmds */
#ifdef DIAGNOSTIC
	if (data->ed_pack.ep_vmcmds.evs_used == 0)
		panic("%s: no vmcmds", __func__);
#endif

#ifdef DEBUG_EXEC
	{
		size_t j;
		struct exec_vmcmd *vp = &data->ed_pack.ep_vmcmds.evs_cmds[0];
		DPRINTF(("vmcmds %u\n", data->ed_pack.ep_vmcmds.evs_used));
		for (j = 0; j < data->ed_pack.ep_vmcmds.evs_used; j++) {
			DPRINTF(("vmcmd[%zu] = vmcmd_map_%s %#"
			    PRIxVADDR"/%#"PRIxVSIZE" fd@%#"
			    PRIxVSIZE" prot=0%o flags=%d\n", j,
			    vp[j].ev_proc == vmcmd_map_pagedvn ?
			    "pagedvn" :
			    vp[j].ev_proc == vmcmd_map_readvn ?
			    "readvn" :
			    vp[j].ev_proc == vmcmd_map_zero ?
			    "zero" : "*unknown*",
			    vp[j].ev_addr, vp[j].ev_len,
			    vp[j].ev_offset, vp[j].ev_prot,
			    vp[j].ev_flags));
		}
	}
#endif	/* DEBUG_EXEC */

	for (i = 0; i < data->ed_pack.ep_vmcmds.evs_used && !error; i++) {
		struct exec_vmcmd *vcp;

		vcp = &data->ed_pack.ep_vmcmds.evs_cmds[i];
		if (vcp->ev_flags & VMCMD_RELATIVE) {
#ifdef DIAGNOSTIC
			if (base_vcp == NULL)
				panic("%s: relative vmcmd with no base",
				    __func__);
			if (vcp->ev_flags & VMCMD_BASE)
				panic("%s: illegal base & relative vmcmd",
				    __func__);
#endif
			vcp->ev_addr += base_vcp->ev_addr;
		}
		error = (*vcp->ev_proc)(l, vcp);
#ifdef DEBUG_EXEC
		if (error) {
			size_t j;
			struct exec_vmcmd *vp =
			    &data->ed_pack.ep_vmcmds.evs_cmds[0];
			DPRINTF(("vmcmds %zu/%u, error %d\n", i, 
			    data->ed_pack.ep_vmcmds.evs_used, error));
			for (j = 0; j < data->ed_pack.ep_vmcmds.evs_used; j++) {
				DPRINTF(("vmcmd[%zu] = vmcmd_map_%s %#"
				    PRIxVADDR"/%#"PRIxVSIZE" fd@%#"
				    PRIxVSIZE" prot=0%o flags=%d\n", j,
				    vp[j].ev_proc == vmcmd_map_pagedvn ?
				    "pagedvn" :
				    vp[j].ev_proc == vmcmd_map_readvn ?
				    "readvn" :
				    vp[j].ev_proc == vmcmd_map_zero ?
				    "zero" : "*unknown*",
				    vp[j].ev_addr, vp[j].ev_len,
				    vp[j].ev_offset, vp[j].ev_prot,
				    vp[j].ev_flags));
				if (j == i)
					DPRINTF(("     ^--- failed\n"));
			}
		}
#endif /* DEBUG_EXEC */
		if (vcp->ev_flags & VMCMD_BASE)
			base_vcp = vcp;
	}

	/* free the vmspace-creation commands, and release their references */
	kill_vmcmds(&data->ed_pack.ep_vmcmds);

	vn_lock(data->ed_pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(data->ed_pack.ep_vp, FREAD, l->l_cred);
	vput(data->ed_pack.ep_vp);

	/* if an error happened, deallocate and punt */
	if (error) {
		DPRINTF(("%s: vmcmd %zu failed: %d\n", __func__, i - 1, error));
		goto exec_abort;
	}

	/* remember information about the process */
	data->ed_arginfo.ps_nargvstr = data->ed_argc;
	data->ed_arginfo.ps_nenvstr = data->ed_envc;

	/* set command name & other accounting info */
	commandname = strrchr(data->ed_pack.ep_resolvedname, '/');
	if (commandname != NULL) {
		commandname++;
	} else {
		commandname = data->ed_pack.ep_resolvedname;
	}
	i = min(strlen(commandname), MAXCOMLEN);
	(void)memcpy(p->p_comm, commandname, i);
	p->p_comm[i] = '\0';

	dp = PNBUF_GET();
	/*
	 * If the path starts with /, we don't need to do any work.
	 * This handles the majority of the cases.
	 * In the future perhaps we could canonicalize it?
	 */
	if (data->ed_pathstring[0] == '/')
		(void)strlcpy(data->ed_pack.ep_path = dp, data->ed_pathstring,
		    MAXPATHLEN);
#ifdef notyet
	/*
	 * Although this works most of the time [since the entry was just
	 * entered in the cache] we don't use it because it theoretically
	 * can fail and it is not the cleanest interface, because there
	 * could be races. When the namei cache is re-written, this can
	 * be changed to use the appropriate function.
	 */
	else if (!(error = vnode_to_path(dp, MAXPATHLEN, p->p_textvp, l, p)))
		data->ed_pack.ep_path = dp;
#endif
	else {
#ifdef notyet
		printf("Cannot get path for pid %d [%s] (error %d)",
		    (int)p->p_pid, p->p_comm, error);
#endif
		data->ed_pack.ep_path = NULL;
		PNBUF_PUT(dp);
	}

	stack = (char *)STACK_ALLOC(STACK_GROW(vm->vm_minsaddr,
		STACK_PTHREADSPACE + data->ed_ps_strings_sz + data->ed_szsigcode),
		data->ed_pack.ep_ssize - (data->ed_ps_strings_sz + data->ed_szsigcode));

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
	 * This extra space was allowed for when 'pack.ep_ssize' was calculated.
	 */
	stack += RTLD_GAP;
#endif /* __MACHINE_STACK_GROWS_UP */
	
	/* Now copy argc, args & environ to new stack */
	error = (*data->ed_pack.ep_esch->es_copyargs)(l, &data->ed_pack,
	    &data->ed_arginfo, &stack, data->ed_argp);

	if (data->ed_pack.ep_path) {
		PNBUF_PUT(data->ed_pack.ep_path);
		data->ed_pack.ep_path = NULL;
	}
	if (error) {
		DPRINTF(("%s: copyargs failed %d\n", __func__, error));
		goto exec_abort;
	}
	/* Move the stack back to original point */
	stack = (char *)STACK_GROW(vm->vm_minsaddr, data->ed_pack.ep_ssize);

	/* fill process ps_strings info */
	p->p_psstrp = (vaddr_t)STACK_ALLOC(STACK_GROW(vm->vm_minsaddr,
	    STACK_PTHREADSPACE), data->ed_ps_strings_sz);

	if (data->ed_pack.ep_flags & EXEC_32) {
		arginfo32.ps_argvstr = (vaddr_t)data->ed_arginfo.ps_argvstr;
		arginfo32.ps_nargvstr = data->ed_arginfo.ps_nargvstr;
		arginfo32.ps_envstr = (vaddr_t)data->ed_arginfo.ps_envstr;
		arginfo32.ps_nenvstr = data->ed_arginfo.ps_nenvstr;
	}

	/* copy out the process's ps_strings structure */
	if ((error = copyout(aip, (void *)p->p_psstrp, data->ed_ps_strings_sz))
	    != 0) {
		DPRINTF(("%s: ps_strings copyout %p->%p size %zu failed\n",
		    __func__, aip, (void *)p->p_psstrp, data->ed_ps_strings_sz));
		goto exec_abort;
	}

	cwdexec(p);
	fd_closeexec();		/* handle close on exec */

	if (__predict_false(ktrace_on))
		fd_ktrexecfd();

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
		l->l_lwpctl = NULL; /* was on loan from blocked parent */
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

	    (((data->ed_attr.va_mode & S_ISUID) != 0 &&
	      kauth_cred_geteuid(l->l_cred) != data->ed_attr.va_uid) ||

	     ((data->ed_attr.va_mode & S_ISGID) != 0 &&
	      kauth_cred_getegid(l->l_cred) != data->ed_attr.va_gid))) {
		/*
		 * Mark the process as SUGID before we do
		 * anything that might block.
		 */
		proc_crmod_enter();
		proc_crmod_leave(NULL, NULL, true);

		/* Make sure file descriptors 0..2 are in use. */
		if ((error = fd_checkstd()) != 0) {
			DPRINTF(("%s: fdcheckstd failed %d\n",
			    __func__, error));
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
		if (data->ed_attr.va_mode & S_ISUID)
			kauth_cred_seteuid(l->l_cred, data->ed_attr.va_uid);
		if (data->ed_attr.va_mode & S_ISGID)
			kauth_cred_setegid(l->l_cred, data->ed_attr.va_gid);
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
	(*data->ed_pack.ep_esch->es_emul->e_setregs)(l, &data->ed_pack,
	     (vaddr_t)stack);
	if (data->ed_pack.ep_esch->es_setregs)
		(*data->ed_pack.ep_esch->es_setregs)(l, &data->ed_pack,
		    (vaddr_t)stack);

	/* Provide a consistent LWP private setting */
	(void)lwp_setprivate(l, NULL);

	/* Discard all PCU state; need to start fresh */
	pcu_discard_all(l);

	/* map the process's signal trampoline code */
	if ((error = exec_sigcode_map(p, data->ed_pack.ep_esch->es_emul)) != 0) {
		DPRINTF(("%s: map sigcode failed %d\n", __func__, error));
		goto exec_abort;
	}

	pool_put(&exec_pool, data->ed_argp);

	/* notify others that we exec'd */
	KNOTE(&p->p_klist, NOTE_EXEC);

	kmem_free(data->ed_pack.ep_hdr, data->ed_pack.ep_hdrlen);

	SDT_PROBE(proc,,,exec_success, data->ed_pack.ep_name, 0, 0, 0, 0);

	/* The emulation root will usually have been found when we looked
	 * for the elf interpreter (or similar), if not look now. */
	if (data->ed_pack.ep_esch->es_emul->e_path != NULL &&
	    data->ed_pack.ep_emul_root == NULL)
		emul_find_root(l, &data->ed_pack);

	/* Any old emulation root got removed by fdcloseexec */
	rw_enter(&p->p_cwdi->cwdi_lock, RW_WRITER);
	p->p_cwdi->cwdi_edir = data->ed_pack.ep_emul_root;
	rw_exit(&p->p_cwdi->cwdi_lock);
	data->ed_pack.ep_emul_root = NULL;
	if (data->ed_pack.ep_interp != NULL)
		vrele(data->ed_pack.ep_interp);

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
	    && p->p_emul != data->ed_pack.ep_esch->es_emul)
		(*p->p_emul->e_proc_exit)(p);

	/*
	 * This is now LWP 1.
	 */
	mutex_enter(p->p_lock);
	p->p_nlwpid = 1;
	l->l_lid = 1;
	mutex_exit(p->p_lock);

	/*
	 * Call exec hook. Emulation code may NOT store reference to anything
	 * from &pack.
	 */
	if (data->ed_pack.ep_esch->es_emul->e_proc_exec)
		(*data->ed_pack.ep_esch->es_emul->e_proc_exec)(p, &data->ed_pack);

	/* update p_emul, the old value is no longer needed */
	p->p_emul = data->ed_pack.ep_esch->es_emul;

	/* ...and the same for p_execsw */
	p->p_execsw = data->ed_pack.ep_esch;

#ifdef __HAVE_SYSCALL_INTERN
	(*p->p_emul->e_syscall_intern)(p);
#endif
	ktremul();

	/* Allow new references from the debugger/procfs. */
	rw_exit(&p->p_reflock);
	if (!no_local_exec_lock)
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
		p->p_waited = 0;
		mutex_enter(p->p_lock);
		ksiginfo_queue_init(&kq);
		sigclearall(p, &contsigmask, &kq);
		lwp_lock(l);
		l->l_stat = LSSTOP;
		p->p_stat = SSTOP;
		p->p_nrlwps--;
		lwp_unlock(l);
		mutex_exit(p->p_lock);
		mutex_exit(proc_lock);
		lwp_lock(l);
		mi_switch(l);
		ksiginfo_queue_drain(&kq);
		KERNEL_LOCK(l->l_biglocks, l);
	} else {
		mutex_exit(proc_lock);
	}

	pathbuf_stringcopy_put(data->ed_pathbuf, data->ed_pathstring);
	pathbuf_destroy(data->ed_pathbuf);
	PNBUF_PUT(data->ed_resolvedpathbuf);
	DPRINTF(("%s finished\n", __func__));
	return (EJUSTRETURN);

 exec_abort:
	SDT_PROBE(proc,,,exec_failure, error, 0, 0, 0, 0);
	rw_exit(&p->p_reflock);
	if (!no_local_exec_lock)
		rw_exit(&exec_lock);

	pathbuf_stringcopy_put(data->ed_pathbuf, data->ed_pathstring);
	pathbuf_destroy(data->ed_pathbuf);
	PNBUF_PUT(data->ed_resolvedpathbuf);

	/*
	 * the old process doesn't exist anymore.  exit gracefully.
	 * get rid of the (new) address space we have created, if any, get rid
	 * of our namei data and vnode, and exit noting failure
	 */
	uvm_deallocate(&vm->vm_map, VM_MIN_ADDRESS,
		VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS);

	exec_free_emul_arg(&data->ed_pack);
	pool_put(&exec_pool, data->ed_argp);
	kmem_free(data->ed_pack.ep_hdr, data->ed_pack.ep_hdrlen);
	if (data->ed_pack.ep_emul_root != NULL)
		vrele(data->ed_pack.ep_emul_root);
	if (data->ed_pack.ep_interp != NULL)
		vrele(data->ed_pack.ep_interp);

	/* Acquire the sched-state mutex (exit1() will release it). */
	if (!is_spawn) {
		mutex_enter(p->p_lock);
		exit1(l, W_EXITCODE(error, SIGABRT));
	}

	return error;
}

int
execve1(struct lwp *l, const char *path, char * const *args,
    char * const *envs, execve_fetch_element_t fetch_element)
{
	struct execve_data data;
	int error;

	error = execve_loadvm(l, path, args, envs, fetch_element, &data);
	if (error)
		return error;
	error = execve_runproc(l, &data, false, false);
	return error;
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
	if ((error = copyout(&argc, cpp++, sizeof(argc))) != 0) {
		COPYPRINTF("", cpp - 1, sizeof(argc));
		return error;
	}

	dp = (char *) (cpp + argc + envc + 2 + pack->ep_esch->es_arglen);
	sp = argp;

	/* XXX don't copy them out, remap them! */
	arginfo->ps_argvstr = cpp; /* remember location of argv for later */

	for (; --argc >= 0; sp += len, dp += len) {
		if ((error = copyout(&dp, cpp++, sizeof(dp))) != 0) {
			COPYPRINTF("", cpp - 1, sizeof(dp));
			return error;
		}
		if ((error = copyoutstr(sp, dp, ARG_MAX, &len)) != 0) {
			COPYPRINTF("str", dp, (size_t)ARG_MAX);
			return error;
		}
	}

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0) {
		COPYPRINTF("", cpp - 1, sizeof(nullp));
		return error;
	}

	arginfo->ps_envstr = cpp; /* remember location of envp for later */

	for (; --envc >= 0; sp += len, dp += len) {
		if ((error = copyout(&dp, cpp++, sizeof(dp))) != 0) {
			COPYPRINTF("", cpp - 1, sizeof(dp));
			return error;
		}
		if ((error = copyoutstr(sp, dp, ARG_MAX, &len)) != 0) {
			COPYPRINTF("str", dp, (size_t)ARG_MAX);
			return error;
		}

	}

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0) {
		COPYPRINTF("", cpp - 1, sizeof(nullp));
		return error;
	}

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
			panic("%s", __func__);
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
		DPRINTF(("%s, %d: map %p "
		    "uvm_map %#"PRIxVSIZE"@%#"PRIxVADDR" failed %d\n",
		    __func__, __LINE__, &p->p_vmspace->vm_map, round_page(sz),
		    va, error));
		(*uobj->pgops->pgo_detach)(uobj);
		return (error);
	}
	p->p_sigctx.ps_sigcode = (void *)va;
	return (0);
}

/*
 * Release a refcount on spawn_exec_data and destroy memory, if this
 * was the last one.
 */
static void
spawn_exec_data_release(struct spawn_exec_data *data)
{
	if (atomic_dec_32_nv(&data->sed_refcnt) != 0)
		return;

	cv_destroy(&data->sed_cv_child_ready);
	mutex_destroy(&data->sed_mtx_child);

	if (data->sed_actions)
		posix_spawn_fa_free(data->sed_actions,
		    data->sed_actions->len);
	if (data->sed_attrs)
		kmem_free(data->sed_attrs,
		    sizeof(*data->sed_attrs));
	kmem_free(data, sizeof(*data));
}

/*
 * A child lwp of a posix_spawn operation starts here and ends up in
 * cpu_spawn_return, dealing with all filedescriptor and scheduler
 * manipulations in between.
 * The parent waits for the child, as it is not clear wether the child
 * will be able to aquire its own exec_lock. If it can, the parent can
 * be released early and continue running in parallel. If not (or if the
 * magic debug flag is passed in the scheduler attribute struct), the
 * child rides on the parent's exec lock untill it is ready to return to
 * to userland - and only then releases the parent. This method loses
 * concurrency, but improves error reporting.
 */
static void
spawn_return(void *arg)
{
	struct spawn_exec_data *spawn_data = arg;
	struct lwp *l = curlwp;
	int error, newfd;
	int ostat;
	size_t i;
	const struct posix_spawn_file_actions_entry *fae;
	pid_t ppid;
	register_t retval;
	bool have_reflock;
	bool parent_is_waiting = true;

	/*
	 * Check if we can release parent early.
	 * We either need to have no sed_attrs, or sed_attrs does not
	 * have POSIX_SPAWN_RETURNERROR or one of the flags, that require
	 * safe access to the parent proc (passed in sed_parent).
	 * We then try to get the exec_lock, and only if that works, we can
	 * release the parent here already.
	 */
	ppid = spawn_data->sed_parent->p_pid;
	if ((!spawn_data->sed_attrs
	    || (spawn_data->sed_attrs->sa_flags
	        & (POSIX_SPAWN_RETURNERROR|POSIX_SPAWN_SETPGROUP)) == 0)
	    && rw_tryenter(&exec_lock, RW_READER)) {
		parent_is_waiting = false;
		mutex_enter(&spawn_data->sed_mtx_child);
		cv_signal(&spawn_data->sed_cv_child_ready);
		mutex_exit(&spawn_data->sed_mtx_child);
	}

	/* don't allow debugger access yet */
	rw_enter(&l->l_proc->p_reflock, RW_WRITER);
	have_reflock = true;

	error = 0;
	/* handle posix_spawn_file_actions */
	if (spawn_data->sed_actions != NULL) {
		for (i = 0; i < spawn_data->sed_actions->len; i++) {
			fae = &spawn_data->sed_actions->fae[i];
			switch (fae->fae_action) {
			case FAE_OPEN:
				if (fd_getfile(fae->fae_fildes) != NULL) {
					error = fd_close(fae->fae_fildes);
					if (error)
						break;
				}
				error = fd_open(fae->fae_path, fae->fae_oflag,
				    fae->fae_mode, &newfd);
 				if (error)
 					break;
				if (newfd != fae->fae_fildes) {
					error = dodup(l, newfd,
					    fae->fae_fildes, 0, &retval);
					if (fd_getfile(newfd) != NULL)
						fd_close(newfd);
				}
				break;
			case FAE_DUP2:
				error = dodup(l, fae->fae_fildes,
				    fae->fae_newfildes, 0, &retval);
				break;
			case FAE_CLOSE:
				if (fd_getfile(fae->fae_fildes) == NULL) {
					error = EBADF;
					break;
				}
				error = fd_close(fae->fae_fildes);
				break;
			}
			if (error)
				goto report_error;
		}
	}

	/* handle posix_spawnattr */
	if (spawn_data->sed_attrs != NULL) {
		struct sigaction sigact;
		sigact._sa_u._sa_handler = SIG_DFL;
		sigact.sa_flags = 0;

		/* 
		 * set state to SSTOP so that this proc can be found by pid.
		 * see proc_enterprp, do_sched_setparam below
		 */
		mutex_enter(proc_lock);
		/*
		 * p_stat should be SACTIVE, so we need to adjust the
		 * parent's p_nstopchild here.  For safety, just make
		 * we're on the good side of SDEAD before we adjust.
		 */
		ostat = l->l_proc->p_stat;
		KASSERT(ostat < SSTOP);
		l->l_proc->p_stat = SSTOP;
		l->l_proc->p_waited = 0;
		l->l_proc->p_pptr->p_nstopchild++;
		mutex_exit(proc_lock);

		/* Set process group */
		if (spawn_data->sed_attrs->sa_flags & POSIX_SPAWN_SETPGROUP) {
			pid_t mypid = l->l_proc->p_pid,
			     pgrp = spawn_data->sed_attrs->sa_pgroup;

			if (pgrp == 0)
				pgrp = mypid;

			error = proc_enterpgrp(spawn_data->sed_parent,
			    mypid, pgrp, false);
			if (error)
				goto report_error_stopped;
		}

		/* Set scheduler policy */
		if (spawn_data->sed_attrs->sa_flags & POSIX_SPAWN_SETSCHEDULER)
			error = do_sched_setparam(l->l_proc->p_pid, 0,
			    spawn_data->sed_attrs->sa_schedpolicy,
			    &spawn_data->sed_attrs->sa_schedparam);
		else if (spawn_data->sed_attrs->sa_flags
		    & POSIX_SPAWN_SETSCHEDPARAM) {
			error = do_sched_setparam(ppid, 0,
			    SCHED_NONE, &spawn_data->sed_attrs->sa_schedparam);
		}
		if (error)
			goto report_error_stopped;

		/* Reset user ID's */
		if (spawn_data->sed_attrs->sa_flags & POSIX_SPAWN_RESETIDS) {
			error = do_setresuid(l, -1,
			     kauth_cred_getgid(l->l_cred), -1,
			     ID_E_EQ_R | ID_E_EQ_S);
			if (error)
				goto report_error_stopped;
			error = do_setresuid(l, -1,
			    kauth_cred_getuid(l->l_cred), -1,
			    ID_E_EQ_R | ID_E_EQ_S);
			if (error)
				goto report_error_stopped;
		}

		/* Set signal masks/defaults */
		if (spawn_data->sed_attrs->sa_flags & POSIX_SPAWN_SETSIGMASK) {
			mutex_enter(l->l_proc->p_lock);
			error = sigprocmask1(l, SIG_SETMASK,
			    &spawn_data->sed_attrs->sa_sigmask, NULL);
			mutex_exit(l->l_proc->p_lock);
			if (error)
				goto report_error_stopped;
		}

		if (spawn_data->sed_attrs->sa_flags & POSIX_SPAWN_SETSIGDEF) {
			for (i = 1; i <= NSIG; i++) {
				if (sigismember(
				    &spawn_data->sed_attrs->sa_sigdefault, i))
					sigaction1(l, i, &sigact, NULL, NULL,
					    0);
			}
		}
		mutex_enter(proc_lock);
		l->l_proc->p_stat = ostat;
		l->l_proc->p_pptr->p_nstopchild--;
		mutex_exit(proc_lock);
	}

	/* now do the real exec */
	error = execve_runproc(l, &spawn_data->sed_exec, parent_is_waiting,
	    true);
	have_reflock = false;
	if (error == EJUSTRETURN)
		error = 0;
	else if (error)
		goto report_error;

	if (parent_is_waiting) {
		mutex_enter(&spawn_data->sed_mtx_child);
		cv_signal(&spawn_data->sed_cv_child_ready);
		mutex_exit(&spawn_data->sed_mtx_child);
	}

	/* release our refcount on the data */
	spawn_exec_data_release(spawn_data);

	/* and finaly: leave to userland for the first time */
	cpu_spawn_return(l);

	/* NOTREACHED */
	return;

 report_error_stopped:
	mutex_enter(proc_lock);
	l->l_proc->p_stat = ostat;
	l->l_proc->p_pptr->p_nstopchild--;
	mutex_exit(proc_lock);
 report_error:
 	if (have_reflock) {
 		/*
		 * We have not passed through execve_runproc(),
		 * which would have released the p_reflock and also
		 * taken ownership of the sed_exec part of spawn_data,
		 * so release/free both here.
		 */
		rw_exit(&l->l_proc->p_reflock);
		execve_free_data(&spawn_data->sed_exec);
	}

	if (parent_is_waiting) {
		/* pass error to parent */
		mutex_enter(&spawn_data->sed_mtx_child);
		spawn_data->sed_error = error;
		cv_signal(&spawn_data->sed_cv_child_ready);
		mutex_exit(&spawn_data->sed_mtx_child);
	} else {
		rw_exit(&exec_lock);
	}

	/* release our refcount on the data */
	spawn_exec_data_release(spawn_data);

	/* done, exit */
	mutex_enter(l->l_proc->p_lock);
	/*
	 * Posix explicitly asks for an exit code of 127 if we report
	 * errors from the child process - so, unfortunately, there
	 * is no way to report a more exact error code.
	 * A NetBSD specific workaround is POSIX_SPAWN_RETURNERROR as
	 * flag bit in the attrp argument to posix_spawn(2), see above.
	 */
	exit1(l, W_EXITCODE(127, 0));
}

void
posix_spawn_fa_free(struct posix_spawn_file_actions *fa, size_t len)
{

	for (size_t i = 0; i < len; i++) {
		struct posix_spawn_file_actions_entry *fae = &fa->fae[i];
		if (fae->fae_action != FAE_OPEN)
			continue;
		kmem_free(fae->fae_path, strlen(fae->fae_path) + 1);
	}
	if (fa->len > 0)
		kmem_free(fa->fae, sizeof(*fa->fae) * fa->len);
	kmem_free(fa, sizeof(*fa));
}

static int
posix_spawn_fa_alloc(struct posix_spawn_file_actions **fap,
    const struct posix_spawn_file_actions *ufa, rlim_t lim)
{
	struct posix_spawn_file_actions *fa;
	struct posix_spawn_file_actions_entry *fae;
	char *pbuf = NULL;
	int error;
	size_t i = 0;

	fa = kmem_alloc(sizeof(*fa), KM_SLEEP);
	error = copyin(ufa, fa, sizeof(*fa));
	if (error) {
		fa->fae = NULL;
		fa->len = 0;
		goto out;
	}

	if (fa->len == 0) {
		kmem_free(fa, sizeof(*fa));
		return 0;
	}

	if (fa->len > lim) {
		kmem_free(fa, sizeof(*fa));
		return EINVAL;
	}

	fa->size = fa->len;
	size_t fal = fa->len * sizeof(*fae);
	fae = fa->fae;
	fa->fae = kmem_alloc(fal, KM_SLEEP);
	error = copyin(fae, fa->fae, fal);
	if (error)
		goto out;

	pbuf = PNBUF_GET();
	for (; i < fa->len; i++) {
		fae = &fa->fae[i];
		if (fae->fae_action != FAE_OPEN)
			continue;
		error = copyinstr(fae->fae_path, pbuf, MAXPATHLEN, &fal);
		if (error)
			goto out;
		fae->fae_path = kmem_alloc(fal, KM_SLEEP);
		memcpy(fae->fae_path, pbuf, fal);
	}
	PNBUF_PUT(pbuf);

	*fap = fa;
	return 0;
out:
	if (pbuf)
		PNBUF_PUT(pbuf);
	posix_spawn_fa_free(fa, i);
	return error;
}

int
check_posix_spawn(struct lwp *l1)
{
	int error, tnprocs, count;
	uid_t uid;
	struct proc *p1;

	p1 = l1->l_proc;
	uid = kauth_cred_getuid(l1->l_cred);
	tnprocs = atomic_inc_uint_nv(&nprocs);

	/*
	 * Although process entries are dynamically created, we still keep
	 * a global limit on the maximum number we will create.
	 */
	if (__predict_false(tnprocs >= maxproc))
		error = -1;
	else
		error = kauth_authorize_process(l1->l_cred,
		    KAUTH_PROCESS_FORK, p1, KAUTH_ARG(tnprocs), NULL, NULL);

	if (error) {
		atomic_dec_uint(&nprocs);
		return EAGAIN;
	}

	/*
	 * Enforce limits.
	 */
	count = chgproccnt(uid, 1);
	if (kauth_authorize_generic(l1->l_cred, KAUTH_GENERIC_ISSUSER, NULL) !=
	    0 && __predict_false(count > p1->p_rlimit[RLIMIT_NPROC].rlim_cur)) {
		(void)chgproccnt(uid, -1);
		atomic_dec_uint(&nprocs);
		return EAGAIN;
	}

	return 0;
}

int
do_posix_spawn(struct lwp *l1, pid_t *pid_res, bool *child_ok, const char *path,
	struct posix_spawn_file_actions *fa,
	struct posix_spawnattr *sa,
	char *const *argv, char *const *envp,
	execve_fetch_element_t fetch)
{

	struct proc *p1, *p2;
	struct lwp *l2;
	int error;
	struct spawn_exec_data *spawn_data;
	vaddr_t uaddr;
	pid_t pid;
	bool have_exec_lock = false;

	p1 = l1->l_proc;

	/* Allocate and init spawn_data */
	spawn_data = kmem_zalloc(sizeof(*spawn_data), KM_SLEEP);
	spawn_data->sed_refcnt = 1; /* only parent so far */
	cv_init(&spawn_data->sed_cv_child_ready, "pspawn");
	mutex_init(&spawn_data->sed_mtx_child, MUTEX_DEFAULT, IPL_NONE);
	mutex_enter(&spawn_data->sed_mtx_child);

	/*
	 * Do the first part of the exec now, collect state
	 * in spawn_data.
	 */
	error = execve_loadvm(l1, path, argv,
	    envp, fetch, &spawn_data->sed_exec);
	if (error == EJUSTRETURN)
		error = 0;
	else if (error)
		goto error_exit;

	have_exec_lock = true;

	/*
	 * Allocate virtual address space for the U-area now, while it
	 * is still easy to abort the fork operation if we're out of
	 * kernel virtual address space.
	 */
	uaddr = uvm_uarea_alloc();
	if (__predict_false(uaddr == 0)) {
		error = ENOMEM;
		goto error_exit;
	}
	
	/*
	 * Allocate new proc. Borrow proc0 vmspace for it, we will
	 * replace it with its own before returning to userland
	 * in the child.
	 * This is a point of no return, we will have to go through
	 * the child proc to properly clean it up past this point.
	 */
	p2 = proc_alloc();
	pid = p2->p_pid;

	/*
	 * Make a proc table entry for the new process.
	 * Start by zeroing the section of proc that is zero-initialized,
	 * then copy the section that is copied directly from the parent.
	 */
	memset(&p2->p_startzero, 0,
	    (unsigned) ((char *)&p2->p_endzero - (char *)&p2->p_startzero));
	memcpy(&p2->p_startcopy, &p1->p_startcopy,
	    (unsigned) ((char *)&p2->p_endcopy - (char *)&p2->p_startcopy));
	p2->p_vmspace = proc0.p_vmspace;

	CIRCLEQ_INIT(&p2->p_sigpend.sp_info);

	LIST_INIT(&p2->p_lwps);
	LIST_INIT(&p2->p_sigwaiters);

	/*
	 * Duplicate sub-structures as needed.
	 * Increase reference counts on shared objects.
	 * Inherit flags we want to keep.  The flags related to SIGCHLD
	 * handling are important in order to keep a consistent behaviour
	 * for the child after the fork.  If we are a 32-bit process, the
	 * child will be too.
	 */
	p2->p_flag =
	    p1->p_flag & (PK_SUGID | PK_NOCLDWAIT | PK_CLDSIGIGN | PK_32);
	p2->p_emul = p1->p_emul;
	p2->p_execsw = p1->p_execsw;

	mutex_init(&p2->p_stmutex, MUTEX_DEFAULT, IPL_HIGH);
	mutex_init(&p2->p_auxlock, MUTEX_DEFAULT, IPL_NONE);
	rw_init(&p2->p_reflock);
	cv_init(&p2->p_waitcv, "wait");
	cv_init(&p2->p_lwpcv, "lwpwait");

	p2->p_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);

	kauth_proc_fork(p1, p2);

	p2->p_raslist = NULL;
	p2->p_fd = fd_copy();

	/* XXX racy */
	p2->p_mqueue_cnt = p1->p_mqueue_cnt;

	p2->p_cwdi = cwdinit();

	/*
	 * Note: p_limit (rlimit stuff) is copy-on-write, so normally
	 * we just need increase pl_refcnt.
	 */
	if (!p1->p_limit->pl_writeable) {
		lim_addref(p1->p_limit);
		p2->p_limit = p1->p_limit;
	} else {
		p2->p_limit = lim_copy(p1->p_limit);
	}

	p2->p_lflag = 0;
	p2->p_sflag = 0;
	p2->p_slflag = 0;
	p2->p_pptr = p1;
	p2->p_ppid = p1->p_pid;
	LIST_INIT(&p2->p_children);

	p2->p_aio = NULL;

#ifdef KTRACE
	/*
	 * Copy traceflag and tracefile if enabled.
	 * If not inherited, these were zeroed above.
	 */
	if (p1->p_traceflag & KTRFAC_INHERIT) {
		mutex_enter(&ktrace_lock);
		p2->p_traceflag = p1->p_traceflag;
		if ((p2->p_tracep = p1->p_tracep) != NULL)
			ktradref(p2);
		mutex_exit(&ktrace_lock);
	}
#endif

	/*
	 * Create signal actions for the child process.
	 */
	p2->p_sigacts = sigactsinit(p1, 0);
	mutex_enter(p1->p_lock);
	p2->p_sflag |=
	    (p1->p_sflag & (PS_STOPFORK | PS_STOPEXEC | PS_NOCLDSTOP));
	sched_proc_fork(p1, p2);
	mutex_exit(p1->p_lock);

	p2->p_stflag = p1->p_stflag;

	/*
	 * p_stats.
	 * Copy parts of p_stats, and zero out the rest.
	 */
	p2->p_stats = pstatscopy(p1->p_stats);

	/* copy over machdep flags to the new proc */
	cpu_proc_fork(p1, p2);

	/*
	 * Prepare remaining parts of spawn data
	 */
	spawn_data->sed_actions = fa;
	spawn_data->sed_attrs = sa;

	spawn_data->sed_parent = p1;

	/* create LWP */
	lwp_create(l1, p2, uaddr, 0, NULL, 0, spawn_return, spawn_data,
	    &l2, l1->l_class);
	l2->l_ctxlink = NULL;	/* reset ucontext link */

	/*
	 * Copy the credential so other references don't see our changes.
	 * Test to see if this is necessary first, since in the common case
	 * we won't need a private reference.
	 */
	if (kauth_cred_geteuid(l2->l_cred) != kauth_cred_getsvuid(l2->l_cred) ||
	    kauth_cred_getegid(l2->l_cred) != kauth_cred_getsvgid(l2->l_cred)) {
		l2->l_cred = kauth_cred_copy(l2->l_cred);
		kauth_cred_setsvuid(l2->l_cred, kauth_cred_geteuid(l2->l_cred));
		kauth_cred_setsvgid(l2->l_cred, kauth_cred_getegid(l2->l_cred));
	}

	/* Update the master credentials. */
	if (l2->l_cred != p2->p_cred) {
		kauth_cred_t ocred;

		kauth_cred_hold(l2->l_cred);
		mutex_enter(p2->p_lock);
		ocred = p2->p_cred;
		p2->p_cred = l2->l_cred;
		mutex_exit(p2->p_lock);
		kauth_cred_free(ocred);
	}

	*child_ok = true;
	spawn_data->sed_refcnt = 2;	/* child gets it as well */
#if 0
	l2->l_nopreempt = 1; /* start it non-preemptable */
#endif

	/*
	 * It's now safe for the scheduler and other processes to see the
	 * child process.
	 */
	mutex_enter(proc_lock);

	if (p1->p_session->s_ttyvp != NULL && p1->p_lflag & PL_CONTROLT)
		p2->p_lflag |= PL_CONTROLT;

	LIST_INSERT_HEAD(&p1->p_children, p2, p_sibling);
	p2->p_exitsig = SIGCHLD;	/* signal for parent on exit */

	LIST_INSERT_AFTER(p1, p2, p_pglist);
	LIST_INSERT_HEAD(&allproc, p2, p_list);

	p2->p_trace_enabled = trace_is_enabled(p2);
#ifdef __HAVE_SYSCALL_INTERN
	(*p2->p_emul->e_syscall_intern)(p2);
#endif

	/*
	 * Make child runnable, set start time, and add to run queue except
	 * if the parent requested the child to start in SSTOP state.
	 */
	mutex_enter(p2->p_lock);

	getmicrotime(&p2->p_stats->p_start);

	lwp_lock(l2);
	KASSERT(p2->p_nrlwps == 1);
	p2->p_nrlwps = 1;
	p2->p_stat = SACTIVE;
	l2->l_stat = LSRUN;
	sched_enqueue(l2, false);
	lwp_unlock(l2);

	mutex_exit(p2->p_lock);
	mutex_exit(proc_lock);

	cv_wait(&spawn_data->sed_cv_child_ready, &spawn_data->sed_mtx_child);
	error = spawn_data->sed_error;
	mutex_exit(&spawn_data->sed_mtx_child);
	spawn_exec_data_release(spawn_data);

	rw_exit(&p1->p_reflock);
	rw_exit(&exec_lock);
	have_exec_lock = false;

	*pid_res = pid;
	return error;

 error_exit:
 	if (have_exec_lock) {
		execve_free_data(&spawn_data->sed_exec);
		rw_exit(&p1->p_reflock);
 		rw_exit(&exec_lock);
	}
	mutex_exit(&spawn_data->sed_mtx_child);
	spawn_exec_data_release(spawn_data);
 
	return error;
}

int
sys_posix_spawn(struct lwp *l1, const struct sys_posix_spawn_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(pid_t *) pid;
		syscallarg(const char *) path;
		syscallarg(const struct posix_spawn_file_actions *) file_actions;
		syscallarg(const struct posix_spawnattr *) attrp;
		syscallarg(char *const *) argv;
		syscallarg(char *const *) envp;
	} */	

	int error;
	struct posix_spawn_file_actions *fa = NULL;
	struct posix_spawnattr *sa = NULL;
	pid_t pid;
	bool child_ok = false;
	rlim_t max_fileactions;
	proc_t *p = l1->l_proc;

	error = check_posix_spawn(l1);
	if (error) {
		*retval = error;
		return 0;
	}

	/* copy in file_actions struct */
	if (SCARG(uap, file_actions) != NULL) {
		max_fileactions = 2 * min(p->p_rlimit[RLIMIT_NOFILE].rlim_cur,
		    maxfiles);
		error = posix_spawn_fa_alloc(&fa, SCARG(uap, file_actions),
		    max_fileactions);
		if (error)
			goto error_exit;
	}

	/* copyin posix_spawnattr struct */
	if (SCARG(uap, attrp) != NULL) {
		sa = kmem_alloc(sizeof(*sa), KM_SLEEP);
		error = copyin(SCARG(uap, attrp), sa, sizeof(*sa));
		if (error)
			goto error_exit;
	}

	/*
	 * Do the spawn
	 */
	error = do_posix_spawn(l1, &pid, &child_ok, SCARG(uap, path), fa, sa,
	    SCARG(uap, argv), SCARG(uap, envp), execve_fetch_element);
	if (error)
		goto error_exit;

	if (error == 0 && SCARG(uap, pid) != NULL)
		error = copyout(&pid, SCARG(uap, pid), sizeof(pid));

	*retval = error;
	return 0;

 error_exit:
	if (!child_ok) {
		(void)chgproccnt(kauth_cred_getuid(l1->l_cred), -1);
		atomic_dec_uint(&nprocs);

		if (sa)
			kmem_free(sa, sizeof(*sa));
		if (fa)
			posix_spawn_fa_free(fa, fa->len);
	}

	*retval = error;
	return 0;
}

void
exec_free_emul_arg(struct exec_package *epp)
{
	if (epp->ep_emul_arg_free != NULL) {
		KASSERT(epp->ep_emul_arg != NULL);
		(*epp->ep_emul_arg_free)(epp->ep_emul_arg);
		epp->ep_emul_arg_free = NULL;
		epp->ep_emul_arg = NULL;
	} else {
		KASSERT(epp->ep_emul_arg == NULL);
	}
}
