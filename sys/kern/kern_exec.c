/*	$NetBSD: kern_exec.c,v 1.133 2000/12/11 05:29:02 mycroft Exp $	*/

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

#include "opt_ktrace.h"
#include "opt_syscall_debug.h"

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
#include <sys/signalvar.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>

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
extern const struct execsw execsw_builtin[];
extern int nexecs_builtin;
static const struct execsw **execsw = NULL;
static int nexecs;
int exec_maxhdrsz;		/* must not be static - netbsd32 needs it */

#ifdef LKM
/* list of supported emulations */
static
LIST_HEAD(emlist_head, emul_entry) el_head = LIST_HEAD_INITIALIZER(el_head);
struct emul_entry {
	LIST_ENTRY(emul_entry) el_list;
	const struct emul *el_emul;
	int ro_entry;
};

/* list of dynamically loaded execsw entries */
static
LIST_HEAD(execlist_head, exec_entry) ex_head = LIST_HEAD_INITIALIZER(ex_head);
struct exec_entry {
	LIST_ENTRY(exec_entry) ex_list;
	const struct execsw *es;
};

/* structure used for building execw[] */
struct execsw_entry {
	struct execsw_entry *next;
	const struct execsw *es;
};
#endif /* LKM */

/* NetBSD emul struct */
extern char sigcode[], esigcode[];
#ifdef SYSCALL_DEBUG
extern const char * const syscallnames[];
#endif
#ifdef __HAVE_SYSCALL_INTERN
void syscall_intern __P((struct proc *));
#else
void syscall __P((void));
#endif

const struct emul emul_netbsd = {
	"netbsd",
	NULL,		/* emulation path */
#ifndef __HAVE_MINIMAL_EMUL
	EMUL_HAS_SYS___syscall,
	NULL,
	SYS_syscall,
	SYS_MAXSYSCALL,
#endif
	sysent,
#ifdef SYSCALL_DEBUG
	syscallnames,
#else
	NULL,
#endif
	sendsig,
	sigcode,
	esigcode,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	syscall_intern,
#else
	syscall,
#endif
};

/*
 * Exec lock. Used to control access to execsw[] structures.
 * This must not be static so that netbsd32 can access it, too.
 */
struct lock exec_lock;
 
#ifdef LKM
static const struct emul *	emul_search __P((const char *));
static void link_es __P((struct execsw_entry **, const struct execsw *));
#endif /* LKM */

/*
 * check exec:
 * given an "executable" described in the exec package's namei info,
 * see what we can do with it.
 *
 * ON ENTRY:
 *	exec package with appropriate namei info
 *	proc pointer of exec'ing proc
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
check_exec(struct proc *p, struct exec_package *epp)
{
	int error, i;
	struct vnode *vp;
	struct nameidata *ndp;
	size_t resid;

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
	if ((vp->v_mount->mnt_flag & MNT_NOSUID) || (p->p_flag & P_TRACED))
		epp->ep_vap->va_mode &= ~(S_ISUID | S_ISGID);

	/* try to open it */
	if ((error = VOP_OPEN(vp, FREAD, p->p_ucred, p)) != 0)
		goto bad1;

	/* unlock vp, since we need it unlocked from here on out. */
	VOP_UNLOCK(vp, 0);

	/* now we have the file, get the exec header */
	uvn_attach(vp, VM_PROT_READ);
	error = vn_rdwr(UIO_READ, vp, epp->ep_hdr, epp->ep_hdrlen, 0,
			UIO_SYSSPACE, 0, p->p_ucred, &resid, p);
	if (error)
		goto bad2;
	epp->ep_hdrvalid = epp->ep_hdrlen - resid;

	/*
	 * set up the vmcmds for creation of the process
	 * address space
	 */
	error = ENOEXEC;
	for (i = 0; i < nexecs && error != 0; i++) {
		int newerror;

		epp->ep_esch = execsw[i];
		newerror = (*execsw[i]->es_check)(p, epp);
		/* make sure the first "interesting" error code is saved. */
		if (!newerror || error == ENOEXEC)
			error = newerror;

		/* if es_check call was successful, update epp->ep_es */
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
		    (epp->ep_dsize > p->p_rlimit[RLIMIT_DATA].rlim_cur))
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

/*
 * exec system call
 */
/* ARGSUSED */
int
sys_execve(struct proc *p, void *v, register_t *retval)
{
	struct sys_execve_args /* {
		syscallarg(const char *) path;
		syscallarg(char * const *) argp;
		syscallarg(char * const *) envp;
	} */ *uap = v;
	int error, i;
	struct exec_package pack;
	struct nameidata nid;
	struct vattr attr;
	struct ucred *cred = p->p_ucred;
	char *argp;
	char * const *cpp;
	char *dp, *sp;
	long argc, envc;
	size_t len;
	char *stack;
	struct ps_strings arginfo;
	struct vmspace *vm;
	char **tmpfap;
	int szsigcode;
	struct exec_vmcmd *base_vcp = NULL;

	/*
	 * Init the namei data to point the file user's program name.
	 * This is done here rather than in check_exec(), so that it's
	 * possible to override this settings if any of makecmd/probe
	 * functions call check_exec() recursively - for example,
	 * see exec_script_makecmds().
	 */
	NDINIT(&nid, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, path), p);

	/*
	 * initialize the fields of the exec package.
	 */
	pack.ep_name = SCARG(uap, path);
	pack.ep_hdr = malloc(exec_maxhdrsz, M_EXEC, M_WAITOK);
	pack.ep_hdrlen = exec_maxhdrsz;
	pack.ep_hdrvalid = 0;
	pack.ep_ndp = &nid;
	pack.ep_emul_arg = NULL;
	pack.ep_vmcmds.evs_cnt = 0;
	pack.ep_vmcmds.evs_used = 0;
	pack.ep_vap = &attr;
	pack.ep_flags = 0;

	lockmgr(&exec_lock, LK_SHARED, NULL);

	/* see if we can run it. */
	if ((error = check_exec(p, &pack)) != 0)
		goto freehdr;

	/* XXX -- THE FOLLOWING SECTION NEEDS MAJOR CLEANUP */

	/* allocate an argument buffer */
	argp = (char *) uvm_km_valloc_wait(exec_map, NCARGS);
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
		    szsigcode + sizeof(struct ps_strings)) - argp;
	else
		len = ((argc + envc + 2 + pack.ep_es->es_arglen) *
		    sizeof(char *) + sizeof(int) + dp + STACKGAPLEN +
		    szsigcode + sizeof(struct ps_strings)) - argp;

	len = ALIGN(len);	/* make the stack "safely" aligned */

	if (len > pack.ep_ssize) { /* in effect, compare to initial limit */
		error = ENOMEM;
		goto bad;
	}

	/* adjust "active stack depth" for process VSZ */
	pack.ep_ssize = len;	/* maybe should go elsewhere, but... */

	/*
	 * Do whatever is necessary to prepare the address space
	 * for remapping.  Note that this might replace the current
	 * vmspace with another!
	 */
	uvmspace_exec(p);

	/* Now map address space */
	vm = p->p_vmspace;
	vm->vm_taddr = (char *) pack.ep_taddr;
	vm->vm_tsize = btoc(pack.ep_tsize);
	vm->vm_daddr = (char *) pack.ep_daddr;
	vm->vm_dsize = btoc(pack.ep_dsize);
	vm->vm_ssize = btoc(pack.ep_ssize);
	vm->vm_maxsaddr = (char *) pack.ep_maxsaddr;
	vm->vm_minsaddr = (char *) pack.ep_minsaddr;

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
#ifdef DEBUG
		if (error) {
			if (i > 0)
				printf("vmcmd[%d] = %#lx/%#lx @ %#lx\n", i-1,
				       vcp[-1].ev_addr, vcp[-1].ev_len,
				       vcp[-1].ev_offset);
			printf("vmcmd[%d] = %#lx/%#lx @ %#lx\n", i,
			       vcp->ev_addr, vcp->ev_len, vcp->ev_offset);
		}
#endif
		if (vcp->ev_flags & VMCMD_BASE)
			base_vcp = vcp;
	}

	/* free the vmspace-creation commands, and release their references */
	kill_vmcmds(&pack.ep_vmcmds);

	/* if an error happened, deallocate and punt */
	if (error) {
#ifdef DEBUG
		printf("execve: vmcmd %i failed: %d\n", i-1, error);
#endif
		goto exec_abort;
	}

	/* remember information about the process */
	arginfo.ps_nargvstr = argc;
	arginfo.ps_nenvstr = envc;

	stack = (char *) (vm->vm_minsaddr - len);
	/* Now copy argc, args & environ to new stack */
	if (!(*pack.ep_es->es_copyargs)(&pack, &arginfo, stack, argp)) {
#ifdef DEBUG
		printf("execve: copyargs failed\n");
#endif
		goto exec_abort;
	}

	/* fill process ps_strings info */
	p->p_psstr = (struct ps_strings *)(vm->vm_minsaddr
		- sizeof(struct ps_strings));
	p->p_psargv = offsetof(struct ps_strings, ps_argvstr);
	p->p_psnargv = offsetof(struct ps_strings, ps_nargvstr);
	p->p_psenv = offsetof(struct ps_strings, ps_envstr);
	p->p_psnenv = offsetof(struct ps_strings, ps_nenvstr);

	/* copy out the process's ps_strings structure */
	if (copyout(&arginfo, (char *)p->p_psstr, sizeof(arginfo))) {
#ifdef DEBUG
		printf("execve: ps_strings copyout failed\n");
#endif
		goto exec_abort;
	}

	/* copy out the process's signal trapoline code */
	if (szsigcode) {
		if (copyout((char *)pack.ep_es->es_emul->e_sigcode,
		    p->p_sigacts->ps_sigcode = (char *)p->p_psstr - szsigcode,
		    szsigcode)) {
#ifdef DEBUG
			printf("execve: sig trampoline copyout failed\n");
#endif
			goto exec_abort;
		}
#ifdef PMAP_NEED_PROCWR
		/* This is code. Let the pmap do what is needed. */
		pmap_procwr(p, (vaddr_t)p->p_sigacts->ps_sigcode, szsigcode);
#endif
	}

	stopprofclock(p);	/* stop profiling */
	fdcloseexec(p);		/* handle close on exec */
	execsigs(p);		/* reset catched signals */
	p->p_ctxlink = NULL;	/* reset ucontext link */

	/* set command name & other accounting info */
	len = min(nid.ni_cnd.cn_namelen, MAXCOMLEN);
	memcpy(p->p_comm, nid.ni_cnd.cn_nameptr, len);
	p->p_comm[len] = 0;
	p->p_acflag &= ~AFORK;

	/* record proc's vnode, for use by procfs and others */
        if (p->p_textvp)
                vrele(p->p_textvp);
	VREF(pack.ep_vp);
	p->p_textvp = pack.ep_vp;

	p->p_flag |= P_EXEC;
	if (p->p_flag & P_PPWAIT) {
		p->p_flag &= ~P_PPWAIT;
		wakeup((caddr_t) p->p_pptr);
	}

	/*
	 * deal with set[ug]id.
	 * MNT_NOSUID and P_TRACED have already been used to disable s[ug]id.
	 */
	if (((attr.va_mode & S_ISUID) != 0 && p->p_ucred->cr_uid != attr.va_uid)
	 || ((attr.va_mode & S_ISGID) != 0 && p->p_ucred->cr_gid != attr.va_gid)){
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
		p_sugid(p);
	} else
		p->p_flag &= ~P_SUGID;
	p->p_cred->p_svuid = p->p_ucred->cr_uid;
	p->p_cred->p_svgid = p->p_ucred->cr_gid;

	doexechooks(p);

	uvm_km_free_wakeup(exec_map, (vaddr_t) argp, NCARGS);

	PNBUF_PUT(nid.ni_cnd.cn_pnbuf);
	vn_lock(pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(pack.ep_vp, FREAD, cred, p);
	vput(pack.ep_vp);

	/* setup new registers and do misc. setup. */
	(*pack.ep_es->es_setregs)(p, &pack, (u_long) stack);

	if (p->p_flag & P_TRACED)
		psignal(p, SIGTRAP);

	free(pack.ep_hdr, M_EXEC);

	/*
	 * Call emulation specific exec hook. This can setup setup per-process
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
#ifdef __HAVE_SYSCALL_INTERN
	(*p->p_emul->e_syscall_intern)(p);
#endif
#ifdef KTRACE
	if (KTRPOINT(p, KTR_EMUL))
		ktremul(p);
#endif

	lockmgr(&exec_lock, LK_RELEASE, NULL);

	return (EJUSTRETURN);

bad:
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
	uvm_km_free_wakeup(exec_map, (vaddr_t) argp, NCARGS);

freehdr:
	lockmgr(&exec_lock, LK_RELEASE, NULL);

	free(pack.ep_hdr, M_EXEC);
	return error;

exec_abort:
	lockmgr(&exec_lock, LK_RELEASE, NULL);

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
	vn_lock(pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(pack.ep_vp, FREAD, cred, p);
	vput(pack.ep_vp);
	uvm_km_free_wakeup(exec_map, (vaddr_t) argp, NCARGS);
	free(pack.ep_hdr, M_EXEC);
	exit1(p, W_EXITCODE(0, SIGABRT));
	exit1(p, -1);

	/* NOTREACHED */
	return 0;
}


void *
copyargs(struct exec_package *pack, struct ps_strings *arginfo,
    void *stack, void *argp)
{
	char **cpp = stack;
	char *dp, *sp;
	size_t len;
	void *nullp = NULL;
	long argc = arginfo->ps_nargvstr;
	long envc = arginfo->ps_nenvstr;

#ifdef __sparc_v9__
	/* XXX Temporary hack for argc format conversion. */
	argc <<= 32;
#endif
	if (copyout(&argc, cpp++, sizeof(argc)))
		return NULL;
#ifdef __sparc_v9__
	/* XXX Temporary hack for argc format conversion. */
	argc >>= 32;
#endif

	dp = (char *) (cpp + argc + envc + 2 + pack->ep_es->es_arglen);
	sp = argp;

	/* XXX don't copy them out, remap them! */
	arginfo->ps_argvstr = cpp; /* remember location of argv for later */

	for (; --argc >= 0; sp += len, dp += len)
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, dp, ARG_MAX, &len))
			return NULL;

	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	arginfo->ps_envstr = cpp; /* remember location of envp for later */

	for (; --envc >= 0; sp += len, dp += len)
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, dp, ARG_MAX, &len))
			return NULL;

	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	return cpp;
}

#ifdef LKM
/*
 * Find an emulation of given name in list of emulations.
 */
static const struct emul *
emul_search(name)
	const char *name;
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
emul_register(emul, ro_entry)
	const struct emul *emul;
	int ro_entry;
{
	struct emul_entry *ee;
	int error = 0;

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
emul_unregister(name)
	const char *name;
{
	struct emul_entry *it;
	int i, error = 0;
	const struct proclist_desc *pd;
	struct proc *ptmp;

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
		LIST_FOREACH(ptmp, pd->pd_list, p_list) {
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
exec_add(esp, e_name)
	struct execsw *esp;
	const char *e_name;
{
	struct exec_entry *it;
	int error = 0;

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
		if (it->es->es_check == esp->es_check
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
exec_remove(esp)
	const struct execsw *esp;
{
	struct exec_entry *it;
	int error = 0;

	lockmgr(&exec_lock, LK_EXCLUSIVE, NULL);

	LIST_FOREACH(it, &ex_head, ex_list) {
		/* assume tuple (makecmds, probe_func, emulation) is unique */
		if (it->es->es_check == esp->es_check
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
link_es(listp, esp)
	struct execsw_entry **listp;
	const struct execsw *esp;
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
		panic("execw[] entry with unknown priority %d found\n",
			et->es->es_prio);
#endif
		break;
	}
}

/*
 * Initialize exec structures. If init_boot is true, also does necessary
 * one-time initialization (it's called from main() that way).
 * Once system is multiuser, this should be called with exec_lock hold,
 * i.e. via exec_{add|remove}().
 */
int
exec_init(init_boot)
	int init_boot;
{
	const struct execsw **new_es, * const *old_es;
	struct execsw_entry *list, *e1;
	struct exec_entry *e2;
	int i, es_sz;

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
			panic("no emulations found in execsw_builtin[]\n");
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
		free((void *)old_es, M_EXEC);

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
exec_init(init_boot)
	int init_boot;
{
	int i;

#ifdef DIAGNOSTIC
	if (!init_boot)
		panic("exec_init(): called with init_boot == 0");
#endif

	/* do one-time initializations */
	lockinit(&exec_lock, PWAIT, "execlck", 0, 0);

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
