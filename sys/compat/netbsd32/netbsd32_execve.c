/*	$NetBSD: netbsd32_execve.c,v 1.10 2002/06/06 10:12:42 fvdl Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_execve.c,v 1.10 2002/06/06 10:12:42 fvdl Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ktrace.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ktrace.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/namei.h>

#include <uvm/uvm_extern.h>

#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/acct.h>
#include <sys/exec.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

/* this is provided by kern/kern_exec.c */
extern int exec_maxhdrsz;
#if defined(LKM) || defined(_LKM)
extern struct lock exec_lock;
#endif

/* 
 * Need to completly reimplement this syscall due to argument copying.
 */
/* ARGSUSED */
int
netbsd32_execve(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_execve_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charpp) argp;
		syscallarg(netbsd32_charpp) envp;
	} */ *uap = v;
	struct sys_execve_args ua;
	caddr_t sg;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(argp, char *);
	NETBSD32TOP_UAP(envp, char *);
	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return netbsd32_execve2(p, &ua, retval);
}

int
netbsd32_execve2(p, uap, retval)
	struct proc *p;
	struct sys_execve_args *uap;
	register_t *retval;
{
	/* Function args */
	int error, i;
	struct exec_package pack;
	struct nameidata nid;
	struct vattr attr;
	struct ucred *cred = p->p_ucred;
	char *argp;
	netbsd32_charp const *cpp;
	char *dp;
	netbsd32_charp sp;
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

#if defined(LKM) || defined(_LKM)
	lockmgr(&exec_lock, LK_SHARED, NULL);
#endif

	/* see if we can run it. */
	if ((error = check_exec(p, &pack)) != 0)
		goto freehdr;

	/* XXX -- THE FOLLOWING SECTION NEEDS MAJOR CLEANUP */

	/* allocate an argument buffer */
	argp = (char *) uvm_km_valloc_wait(exec_map, NCARGS);
#ifdef DIAGNOSTIC
	if (argp == (vaddr_t) 0)
		panic("netbsd32_execve: argp == NULL");
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
	if (!(cpp = (netbsd32_charp *)SCARG(uap, argp))) {
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
		if ((error = copyinstr((char *)(u_long)sp, dp, 
				       len, &len)) != 0) {
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
	if ((cpp = (netbsd32_charp *)SCARG(uap, envp)) != NULL ) {
		while (1) {
			len = argp + ARG_MAX - dp;
			if ((error = copyin(cpp, &sp, sizeof(sp))) != 0)
				goto bad;
			if (!sp)
				break;
			if ((error = copyinstr((char *)(u_long)sp, 
					       dp, len, &len)) != 0) {
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
	uvmspace_exec(p, VM_MIN_ADDRESS, (vaddr_t)pack.ep_minsaddr);

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
		panic("netbsd32_execve: no vmcmds");
#endif
	for (i = 0; i < pack.ep_vmcmds.evs_used && !error; i++) {
		struct exec_vmcmd *vcp;

		vcp = &pack.ep_vmcmds.evs_cmds[i];
		if (vcp->ev_flags & VMCMD_RELATIVE) {
#ifdef DIAGNOSTIC
			if (base_vcp == NULL)
				panic("netbsd32_execve: relative vmcmd with no base");
			if (vcp->ev_flags & VMCMD_BASE)
				panic("netbsd32_execve: illegal base & relative vmcmd");
#endif
			vcp->ev_addr += base_vcp->ev_addr;
		}
		error = (*vcp->ev_proc)(p, vcp);
#ifdef DEBUG
		if (error) {
			int j;

			for (j = 0; j <= i; j++)
				printf("vmcmd[%d] = %#lx/%#lx @ %#lx\n", j,
				       vcp[j-i].ev_addr, vcp[j-i].ev_len,
				       vcp[j-i].ev_offset);
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
		printf("netbsd32_execve: vmcmd %i failed: %d\n", i-1, error);
#endif
		goto exec_abort;
	}

	/* remember information about the process */
	arginfo.ps_nargvstr = argc;
	arginfo.ps_nenvstr = envc;

	stack = (char *) (vm->vm_minsaddr - len);
	/* Now copy argc, args & environ to new stack */
	error = (*pack.ep_es->es_copyargs)(&pack, &arginfo,
	    &stack, argp);
	if (error) {
#ifdef DEBUG
		printf("netbsd32_execve: copyargs failed\n");
#endif
		goto exec_abort;
	}
	/* restore the stack back to its original point */
	stack = (char *) (vm->vm_minsaddr - len);

	/* fill process ps_strings info */
	p->p_psstr = (struct ps_strings *)(vm->vm_minsaddr -
	    sizeof(struct ps_strings));
	p->p_psargv = offsetof(struct ps_strings, ps_argvstr);
	p->p_psnargv = offsetof(struct ps_strings, ps_nargvstr);
	p->p_psenv = offsetof(struct ps_strings, ps_envstr);
	p->p_psnenv = offsetof(struct ps_strings, ps_nenvstr);

	/* copy out the process's ps_strings structure */
	if (copyout(&arginfo, (char *)p->p_psstr, sizeof(arginfo))) {
#ifdef DEBUG
		printf("netbsd32_execve: ps_strings copyout failed\n");
#endif
		goto exec_abort;
	}

	/* copy out the process's signal trapoline code */
	if (szsigcode) {
		if (copyout((char *)pack.ep_es->es_emul->e_sigcode,
		    p->p_sigctx.ps_sigcode = (char *)p->p_psstr - szsigcode,
		    szsigcode)) {
#ifdef DEBUG
			printf("netbsd32_execve: sig trampoline copyout failed\n");
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

	doexechooks(p);

	uvm_km_free_wakeup(exec_map, (vaddr_t) argp, NCARGS);

	PNBUF_PUT(nid.ni_cnd.cn_pnbuf);
	vn_lock(pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(pack.ep_vp, FREAD, cred, p);
	vput(pack.ep_vp);

	/* setup new registers and do misc. setup. */
	(*pack.ep_es->es_emul->e_setregs)(p, &pack, (u_long) stack);
	if (pack.ep_es->es_setregs)
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

#ifdef KTRACE
	if (KTRPOINT(p, KTR_EMUL))
		ktremul(p);
#endif

#if defined(LKM) || defined(_LKM)
	lockmgr(&exec_lock, LK_RELEASE, NULL);
#endif

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
#if defined(LKM) || defined(_LKM)
	lockmgr(&exec_lock, LK_RELEASE, NULL);
#endif

	free(pack.ep_hdr, M_EXEC);
	return error;

exec_abort:
#if defined(LKM) || defined(_LKM)
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
	vn_lock(pack.ep_vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(pack.ep_vp, FREAD, cred, p);
	vput(pack.ep_vp);
	uvm_km_free_wakeup(exec_map, (vaddr_t) argp, NCARGS);
	free(pack.ep_hdr, M_EXEC);
	exit1(p, W_EXITCODE(error, SIGABRT));

	/* NOTREACHED */
	return 0;
}
