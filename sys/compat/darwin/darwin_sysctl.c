/*	$NetBSD: darwin_sysctl.c,v 1.14 2003/09/06 23:54:47 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_sysctl.c,v 1.14 2003/09/06 23:54:47 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/mount.h> 
#include <sys/exec.h> 
#include <sys/proc.h> 
#include <sys/malloc.h> 
#include <sys/sysctl.h> 
#include <sys/sa.h> 
#include <sys/tty.h> 

#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <miscfs/specfs/specdev.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_types.h>
#include <compat/darwin/darwin_exec.h>
#include <compat/darwin/darwin_sysctl.h>
#include <compat/darwin/darwin_syscallargs.h>

pid_t darwin_init_pid = 0;
static char *darwin_sysctl_hw_machine = "Power Macintosh";

static int darwin_kern_sysctl
    (int *, u_int, void *, size_t *, void *, size_t, struct proc *);
static int darwin_vm_sysctl
    (int *, u_int, void *, size_t *, void *, size_t, struct proc *);
static int darwin_vfs_sysctl
    (int *, u_int, void *, size_t *, void *, size_t, struct proc *);
static int darwin_net_sysctl
    (int *, u_int, void *, size_t *, void *, size_t, struct proc *);
static int darwin_debug_sysctl
    (int *, u_int, void *, size_t *, void *, size_t, struct proc *);
static int darwin_hw_sysctl
    (int *, u_int, void *, size_t *, void *, size_t, struct proc *);
static int darwin_machdep_sysctl
    (int *, u_int, void *, size_t *, void *, size_t, struct proc *);
static int darwin_user_sysctl
    (int *, u_int, void *, size_t *, void *, size_t, struct proc *);

static int darwin_sysctl_dokproc(int *, u_int, void *, size_t *);
static void darwin_fill_kproc(struct proc *, struct darwin_kinfo_proc *);
static void native_to_darwin_pflag(int *, int);
static int darwin_procargs(int *, u_int, void *, size_t *, struct proc *);

int
darwin_sys___sysctl(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys___sysctl_args /* {
		syscallarg(int *) name;
		syscallarg(u_int) namelen;
		syscallarg(void *) oldp;
		syscallarg(size_t *) oldlenp;
		syscallarg(void *) newp;
		syscallarg(size_t) newlen;
	} */ *uap = v;
	int error;
	sysctlfn *fn;
	int name[CTL_MAXNAME];
	size_t oldlen;
	size_t savelen;
	int *namep = SCARG(uap, name);
	int namelen = SCARG(uap, namelen);
	void *oldp = SCARG(uap, oldp);
	size_t *oldlenp = SCARG(uap, oldlenp);
	void *newp = SCARG(uap, newp);
	size_t newlen = SCARG(uap, newlen);
	struct proc *p = l->l_proc;

	/*
	 * all top-level sysctl names are non-terminal
	 */
	if (namelen > CTL_MAXNAME || namelen < 2)
		return EINVAL;
	if ((error = copyin(namep, &name, namelen * sizeof(int))) != 0)
		return error;

#ifdef DEBUG_DARWIN
	{
		int i;

		printf("darwin_sys___sysctl: name = [ ");
		for (i = 0; i < namelen; i++)
			printf("%d, ", name[i]);
		printf("]\n");
	}
#endif /* DEBUG_DARWIN */
	/* 
	 * Need to be root to change a value
	 */
	if ((newp != NULL) && (error = suser(p->p_ucred, &p->p_acflag)))
		return error;

	switch (name[0]) {
	case DARWIN_CTL_KERN:
		fn = darwin_kern_sysctl;
		break;
	case DARWIN_CTL_VM:
		fn = darwin_vm_sysctl;
		break;
	case DARWIN_CTL_VFS:
		fn = darwin_vfs_sysctl;
		break;
	case DARWIN_CTL_NET:
		fn = darwin_net_sysctl;
		break;
	case DARWIN_CTL_DEBUG:
		fn = darwin_debug_sysctl;
		break;
	case DARWIN_CTL_HW:
		fn = darwin_hw_sysctl;
		break;
	case DARWIN_CTL_MACHDEP:
		fn = darwin_machdep_sysctl;
		break;
	case DARWIN_CTL_USER:
		fn = darwin_user_sysctl;
		break;
	default:
		return EOPNOTSUPP;
	}

	if (oldlenp) {
		if ((error = copyin(oldlenp, &oldlen, sizeof(oldlen))) != 0)
			return error;
		oldlenp = &oldlen;
	}

	if (oldp) {
		error = uvm_vslock(p, oldp, oldlen, 
		    VM_PROT_READ|VM_PROT_WRITE);
		savelen = oldlen;
	}

	error = (*fn)(name + 1, namelen - 1, oldp, oldlenp, newp, newlen, p);

	if (oldp)
		uvm_vsunlock(p, oldp, savelen);

	if (error)
		return error;

	if (oldlenp)
		error = copyout(&oldlen, SCARG(uap, oldlenp), sizeof(oldlen));

	return error;
}


static int
darwin_kern_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	/* sysctl with the same definition */
	case DARWIN_KERN_OSTYPE:
	case DARWIN_KERN_OSRELEASE:
	case DARWIN_KERN_OSREV:
	case DARWIN_KERN_VERSION:
	case DARWIN_KERN_MAXVNODES:
	case DARWIN_KERN_MAXPROC:
	case DARWIN_KERN_MAXFILES:
	case DARWIN_KERN_ARGMAX:
	case DARWIN_KERN_SECURELVL:
	case DARWIN_KERN_HOSTNAME:
	case DARWIN_KERN_HOSTID:
	case DARWIN_KERN_CLOCKRATE:
	case DARWIN_KERN_VNODE:
	case DARWIN_KERN_FILE:
	case DARWIN_KERN_PROF:
	case DARWIN_KERN_POSIX1:
	case DARWIN_KERN_NGROUPS:
	case DARWIN_KERN_JOB_CONTROL:
	case DARWIN_KERN_SAVED_IDS:
	case DARWIN_KERN_BOOTTIME:
	case DARWIN_KERN_NISDOMAINNAME:
	case DARWIN_KERN_MAXPARTITIONS:
		return kern_sysctl(name, 1, oldp, oldlenp, newp, newlen, p);
		break;

	case DARWIN_KERN_PROCARGS: 
		return darwin_procargs(name + 1, nlen - 1, oldp, oldlenp, p);
		break;

	case DARWIN_KERN_PROC:
		return darwin_sysctl_dokproc(name, nlen, oldp, oldlenp);
		break;
		
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

static int
darwin_vm_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

static int
darwin_vfs_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

static int
darwin_net_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

static int
darwin_debug_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

static int
darwin_hw_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	case DARWIN_HW_NCPU:
		name[0] = HW_NCPU;
		return hw_sysctl(name, 1, oldp, oldlenp, newp, newlen, p);
		break;
	case DARWIN_HW_VECTORUNIT:
		return sysctl_rdint(oldp, oldlenp, newp, 0);
		break;
	case DARWIN_HW_MACHINE:
		return sysctl_rdstring(oldp, oldlenp, newp,
		    darwin_sysctl_hw_machine);
		break; 
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

static int
darwin_machdep_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

static int
darwin_user_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	switch (name[0]) {
	default:
		return EOPNOTSUPP;
	}

	/* NOTREACHED */
	return 0;
}

int
darwin_sysctl(name, nlen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int nlen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	if (nlen != 1)
		return EOPNOTSUPP;

	switch (name[0]) {
	case EMUL_DARWIN_INIT_PID:
		return sysctl_int(oldp, oldlenp, 
		    newp, newlen, &darwin_init_pid);

	default:
		return EOPNOTSUPP;
	}

	return 0;
}

/*
 * On Darwin, mach_init is the system bootstrap process. It is responsible
 * for forking the traditional UNIX init(8) and it does the Mach port naming
 * service. We need mach_init for the naming service, but unfortunately, it
 * will only act as such if its PID is 1. We use a sysctl 
 * (emul.darwin.init_pid) to fool a given process into thinking its PID is 1.
 * That way we can run mach_init when we want to. 
 * Typical use:
 * sysctl -w emul.darwin.init_pid=$$; exec /emul/darwin/sbin/mach_init
 * 
 * The same problem exists after mach_init has forked init: the fork libc stub
 * really insist on the child to have PID 2 (if PID is not 2, then the stub
 * will issue bootstrap calls to an already running mach_init, which fails,
 * of course). 
 */
int 
darwin_sys_getpid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_emuldata *ded;
	struct proc *p = l->l_proc;

	ded = (struct darwin_emuldata *)p->p_emuldata;

	printf("pid %d: fakepid = %d, mach_init_pid = %d\n", 
	    p->p_pid, ded->ded_fakepid, darwin_init_pid);
	if (ded->ded_fakepid != 0)
		*retval = ded->ded_fakepid;
	else
		*retval = p->p_pid;

	return 0;
}

/*
 * This is stolen from sys/kern/kern_sysctl.c:sysctl_doeproc() 
 */
#define DARWIN_KERN_PROCSLOP	(5 * sizeof(struct darwin_kinfo_proc))

static int
darwin_sysctl_dokproc(int *name, u_int namelen, void *vwhere, size_t *sizep)
{
	struct darwin_kinfo_proc kproc;
	struct darwin_kinfo_proc *dp;
	struct proc *p;
	const struct proclist_desc *pd;
	char *where, *dp2;
	int type, op, arg;
	u_int elem_size, elem_count;
	size_t buflen, needed;
	int error;

	dp = vwhere;
	dp2 = where = vwhere;
	buflen = where != NULL ? *sizep : 0;
	error = 0;
	needed = 0;
	type = name[0];

	if (type == DARWIN_KERN_PROC) {
		if (namelen != 3 && 
		    !(namelen == 2 && name[1] == DARWIN_KERN_PROC_ALL))
			return (EINVAL);
		op = name[1];
		if (op != DARWIN_KERN_PROC_ALL)
			arg = name[2];
		else
			arg = 0;		/* Quell compiler warning */
		elem_size = elem_count = 0;	/* Ditto */
	} else {
		if (namelen != 5)
			return (EINVAL);
		op = name[1];
		arg = name[2];
		elem_size = name[3];
		elem_count = name[4];
	}

	proclist_lock_read();

	pd = proclists;
again:
	for (p = LIST_FIRST(pd->pd_list); p != NULL; p = LIST_NEXT(p, p_list)) {
		/*
		 * Skip embryonic processes.
		 */
		if (p->p_stat == SIDL)
			continue;
		/*
		 * TODO - make more efficient (see notes below).
		 * do by session.
		 */
		switch (op) {

		case DARWIN_KERN_PROC_PID:
			/* could do this with just a lookup */
			if (p->p_pid != (pid_t)arg)
				continue;
			break;

		case DARWIN_KERN_PROC_PGRP:
			/* could do this by traversing pgrp */
			if (p->p_pgrp->pg_id != (pid_t)arg)
				continue;
			break;

		case DARWIN_KERN_PROC_SESSION:
			if (p->p_session->s_sid != (pid_t)arg)
				continue;
			break;

		case DARWIN_KERN_PROC_TTY:
			if (arg == (int) KERN_PROC_TTY_REVOKE) {
				if ((p->p_flag & P_CONTROLT) == 0 ||
				    p->p_session->s_ttyp == NULL ||
				    p->p_session->s_ttyvp != NULL)
					continue;
			} else if ((p->p_flag & P_CONTROLT) == 0 ||
			    p->p_session->s_ttyp == NULL) {
					continue;
			} else if (p->p_session->s_ttyp->t_dev != 
			    darwin_to_native_dev((dev_t)arg))
				continue;
			break;

		case DARWIN_KERN_PROC_UID:
			if (p->p_ucred->cr_uid != (uid_t)arg)
				continue;
			break;

		case DARWIN_KERN_PROC_RUID:
			if (p->p_cred->p_ruid != (uid_t)arg)
				continue;
			break;

		case DARWIN_KERN_PROC_ALL:
			/* allow everything */
			break;

		default:
			error = EINVAL;
			goto cleanup;
		}
		if (buflen >= sizeof(struct darwin_kinfo_proc)) {
			darwin_fill_kproc(p, &kproc);
			error = copyout((caddr_t)&kproc, dp, sizeof(kproc));
			if (error)
				goto cleanup;
			dp++;
			buflen -= sizeof(struct darwin_kinfo_proc);
		}
			needed += sizeof(struct darwin_kinfo_proc);
	}
	pd++;
	if (pd->pd_list != NULL)
		goto again;
	proclist_unlock_read();

	if (where != NULL) {
		*sizep = (caddr_t)dp - where;
		if (needed > *sizep)
			return (ENOMEM);
	} else {
		needed += DARWIN_KERN_PROCSLOP;
		*sizep = needed;
	}
	return (0);
 cleanup:
	proclist_unlock_read();
	return (error);
}

/* 
 * Native struct proc to Darin's struct kinfo_proc 
 */
static void
darwin_fill_kproc(p, dkp)
	struct proc *p;
	struct darwin_kinfo_proc *dkp;
{
	struct lwp *l;
	struct darwin_extern_proc *dep;
	struct darwin_eproc *de;

	printf("fillkproc: pid %d\n", p->p_pid);
	l = proc_representative_lwp(p);
	(void)memset(dkp, 0, sizeof(*dkp));

	dep = (struct darwin_extern_proc *)&dkp->kp_proc;
	de = (struct darwin_eproc *)&dkp->kp_eproc;

	/* (ptr) dep->p_un */
	/* (ptr) dep->p_vmspace */
	/* (ptr) dep->p_sigacts */
	native_to_darwin_pflag(&dep->p_flag, p->p_flag);
	dep->p_stat = p->p_stat; /* XXX Neary the same */
	dep->p_pid = p->p_pid;
	dep->p_oppid = p->p_opptr->p_pid;
	dep->p_dupfd = p->p_dupfd;
	/* dep->userstack */
	/* dep->exit_thread */
	/* dep->p_debugger */
	/* dep->p_sigwait */
	dep->p_estcpu = p->p_estcpu;
	dep->p_cpticks = p->p_cpticks;
	dep->p_pctcpu = p->p_pctcpu;
	/* (ptr) dep->p_wchan */
	/* (ptr) dep->p_wmesg */
	/* dep->p_swtime */
	/* dep->p_slptime */
	/* dep->p_realtimer */
	memcpy(&dep->p_rtime, &p->p_rtime, sizeof(p->p_rtime));
	dep->p_uticks = p->p_uticks;
	dep->p_sticks = p->p_sticks;
	dep->p_iticks = p->p_iticks;
	dep->p_traceflag = p->p_traceflag; /* XXX */
	/* (ptr) dep->p_tracep */
	native_sigset13_to_sigset(&dep->p_siglist, 
	    &p->p_sigctx.ps_siglist);
	/* (ptr) dep->p_textvp */
	/* dep->p_holdcnt */
	native_sigset13_to_sigset(&dep->p_sigmask, 
	    &p->p_sigctx.ps_sigmask);
	native_sigset13_to_sigset(&dep->p_sigignore, 
	    &p->p_sigctx.ps_sigignore);
	native_sigset13_to_sigset(&dep->p_sigcatch, 
	    &p->p_sigctx.ps_sigcatch);
	dep->p_priority = l->l_priority;
	dep->p_nice = p->p_nice;
	(void)strlcpy(dep->p_comm, p->p_comm, DARWIN_MAXCOMLEN);
	/* (ptr) dep->p_pgrp */
	/* (ptr) dep->p_addr */
	dep->p_xstat = p->p_xstat;
	dep->p_acflag = p->p_acflag; /* XXX accounting flags */
	/* (ptr) dep->p_ru */

	/* (ptr) */ de->e_paddr = (struct darwin_proc *)p;
	/* (ptr) */ de->e_sess = 
	    (struct darwin_session *)p->p_session;
	de->e_pcred.pc_ruid = p->p_cred->p_ruid;
	de->e_pcred.pc_svuid = p->p_cred->p_svuid;
	de->e_pcred.pc_rgid = p->p_cred->p_rgid;
	de->e_pcred.pc_svgid = p->p_cred->p_svgid;
	de->e_pcred.pc_refcnt = p->p_cred->p_refcnt;
	de->e_ucred.cr_ref = p->p_ucred->cr_ref;
	de->e_ucred.cr_uid = p->p_ucred->cr_uid;
	de->e_ucred.cr_ngroups = p->p_ucred->cr_ngroups;
	(void)memcpy(de->e_ucred.cr_groups, 
	    p->p_ucred->cr_groups, sizeof(gid_t) * DARWIN_NGROUPS);
	de->e_vm.vm_refcnt = p->p_vmspace->vm_refcnt;
	de->e_vm.vm_rssize = p->p_vmspace->vm_rssize;
	de->e_vm.vm_swrss = p->p_vmspace->vm_swrss;
	de->e_vm.vm_tsize = p->p_vmspace->vm_tsize;
	de->e_vm.vm_dsize = p->p_vmspace->vm_dsize;
	de->e_vm.vm_ssize = p->p_vmspace->vm_ssize;
	de->e_vm.vm_taddr = p->p_vmspace->vm_taddr;
	de->e_vm.vm_daddr = p->p_vmspace->vm_daddr;
	de->e_vm.vm_maxsaddr = p->p_vmspace->vm_maxsaddr;
	de->e_ppid = p->p_pptr->p_pid;
	de->e_pgid = p->p_pgid;
	de->e_jobc = p->p_pgrp->pg_jobc;
	if ((p->p_flag & P_CONTROLT) && (p->p_session->s_ttyp != NULL)) {
		de->e_tdev = 
		    native_to_darwin_dev(p->p_session->s_ttyp->t_dev);
		de->e_tpgid = p->p_session->s_ttyp->t_pgrp ? 
		    p->p_session->s_ttyp->t_pgrp->pg_id : NO_PGID;
		/* (ptr) */ de->e_tsess = (struct darwin_session *)
		    p->p_session->s_ttyp->t_session;
	} else {
		de->e_tdev = NODEV;
		/* de->e_tpgid */ 
		/* (ptr) de->e_tsess */ 
	}
	if (l->l_wmesg)
		strlcpy(de->e_wmesg, l->l_wmesg, DARWIN_WMESGLEN);
	de->e_xsize = 0;
	de->e_xrssize = 0;
	de->e_xccount = 0;
	de->e_xswrss = 0;
	de->e_flag = p->p_session->s_ttyvp ? DARWIN_EPROC_CTTY : 0;

	if (SESS_LEADER(p))
		de->e_flag |= DARWIN_EPROC_SLEADER;
	(void)strlcpy(de->e_login, 
	    p->p_session->s_login, DARWIN_COMAPT_MAXLOGNAME);
	/* de->e_spare */

	return;
}

static void 
native_to_darwin_pflag(dfp, bf)
	int *dfp;
	int bf;
{
	int df = 0;

	if (bf & P_ADVLOCK)
		df |= DARWIN_P_ADVLOCK;
	if (bf & P_CONTROLT)
		df |= DARWIN_P_CONTROLT;
	if (bf & P_NOCLDSTOP)
		df |= DARWIN_P_NOCLDSTOP;
	if (bf & P_PPWAIT)
		df |= DARWIN_P_PPWAIT;
	if (bf & P_PROFIL)
		df |= DARWIN_P_PROFIL;
	if (bf & P_SUGID)
		df |= DARWIN_P_SUGID;
	if (bf & P_SYSTEM)
		df |= DARWIN_P_SYSTEM;
	if (bf & P_TRACED)
		df |= DARWIN_P_TRACED;
	if (bf & P_WAITED)
		df |= DARWIN_P_WAITED;
	if (bf & P_WEXIT)
		df |= DARWIN_P_WEXIT;
	if (bf & P_EXEC)
		df |= DARWIN_P_EXEC;
	if (bf & P_OWEUPC)
		df |= DARWIN_P_OWEUPC;
	if (bf & P_FSTRACE)
		df |= DARWIN_P_FSTRACE;
	if (bf & P_NOCLDWAIT)
		df |= DARWIN_P_NOCLDWAIT;
	if (bf & P_NOCLDWAIT)
		df |= DARWIN_P_NOCLDWAIT;

	*dfp = df;
	return;
}

/* Derived from sys/kern/kern_sysctl.c:sysctl_procargs() */
static int
darwin_procargs(name, namelen, where, sizep, up)
	int *name;
	u_int namelen;
	void *where;
	size_t *sizep;
	struct proc *up;
{
	struct ps_strings pss;
	struct proc *p;
	size_t len, upper_bound, xlen, i;
	struct uio auio;
	struct iovec aiov;
	vaddr_t argv;
	pid_t pid;
	int nargv, nenv, nstr, error;
	char *arg;
	char *tmp;

	if (namelen != 1)
		return (EINVAL);
	pid = name[0];

	/* check pid */
	if ((p = pfind(pid)) == NULL)
		return (EINVAL);

	/* only root or same user change look at the environment */
	if (up->p_ucred->cr_uid != 0) {
		if (up->p_cred->p_ruid != p->p_cred->p_ruid ||
		    up->p_cred->p_ruid != p->p_cred->p_svuid)
			return (EPERM);
	}

	if (sizep != NULL && where == NULL) {
		*sizep = ARG_MAX;	/* XXX XXX XXX */
		return (0);
	}
	if (where == NULL || sizep == NULL)
		return (EINVAL);

	/*
	 * Zombies don't have a stack, so we can't read their psstrings.
	 * System processes also don't have a user stack.
	 */
	if (P_ZOMBIE(p) || (p->p_flag & P_SYSTEM) != 0)
		return (EINVAL);

	/*
	 * Lock the process down in memory.
	 */
	/* XXXCDC: how should locking work here? */
	if ((p->p_flag & P_WEXIT) || (p->p_vmspace->vm_refcnt < 1))
		return (EFAULT);

	p->p_vmspace->vm_refcnt++;	/* XXX */

	/*
	 * Allocate a temporary buffer to hold the arguments.
	 */
	arg = malloc(PAGE_SIZE, M_TEMP, M_WAITOK);

	/*
	 * Zero fill the destination buffer.
	 */
	(void)memset(arg, 0, PAGE_SIZE);
	upper_bound = *sizep;
	len = 0;
	while (len < upper_bound) {
		if (len + PAGE_SIZE > upper_bound)
			xlen = len - upper_bound;
		else
			xlen = PAGE_SIZE;

		if ((error = copyout(arg, (char *)where + len, xlen)) != 0)
			goto done;

		len += PAGE_SIZE;
	}

	/*
	 * Read in the ps_strings structure.
	 */
	aiov.iov_base = &pss;
	aiov.iov_len = sizeof(pss);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = (vaddr_t)p->p_psstr;
	auio.uio_resid = sizeof(pss);
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = UIO_READ;
	auio.uio_procp = NULL;
	if ((error = uvm_io(&p->p_vmspace->vm_map, &auio)) != 0)
		goto done;

	/* 
	 * Get argument vector address and length. Since we want to
	 * copy argv and env at the same time, we add their lengths.
	 */
	memcpy(&nargv, (char *)&pss + p->p_psnargv, sizeof(nargv));
	memcpy(&nenv, (char *)&pss + p->p_psnenv, sizeof(nargv)); 
	nstr = nargv + nenv;
	memcpy(&tmp, (char *)&pss + p->p_psargv, sizeof(tmp));

	auio.uio_offset = (off_t)(long)tmp;
	aiov.iov_base = &argv;
	aiov.iov_len = sizeof(argv);
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_resid = sizeof(argv);
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = UIO_READ;
	auio.uio_procp = NULL;
	if ((error = uvm_io(&p->p_vmspace->vm_map, &auio)) != 0)
		goto done;


	/* 
	 * Check the whole argument vector to discover its size 
	 * We have to do this since Darwin's ps insist about having
	 * the data at the end of the buffer.
	 */ 
	len = 0;
	upper_bound = *sizep;
	for (; nstr != 0 && len < upper_bound; len += xlen) {
		aiov.iov_base = arg;
		aiov.iov_len = PAGE_SIZE;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = argv + len;
		xlen = PAGE_SIZE - ((argv + len) & PAGE_MASK);
		auio.uio_resid = xlen;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_rw = UIO_READ;
		auio.uio_procp = NULL;
		error = uvm_io(&p->p_vmspace->vm_map, &auio);
		if (error)
			goto done;

		for (i = 0;i < xlen && nstr != 0; i++) {
			if (arg[i] == '\0')
				nstr--;	/* one full string */
		}

		/*
		 * Make sure we don't reach the 
		 * end of the user's buffer.
		 */
		if (len + i > upper_bound)
			i = upper_bound - len;

		if (nstr == 0) {
			len += i;
			break;
		}
	}

	/* 
	 * Try to keep one null word at the
	 * end, and align on a word boundary 
	 */
	len = (((u_long)where + len - 5) & ~0x3UL) - (u_long)where;
	len = upper_bound - len; 

	/* 
	 * Align to a word boundary, and copy the program name 
	 */
	len = (((u_long)where + len - 1) & ~0x3UL) - (u_long)where;
	len = len - strlen(p->p_comm);
	if (len < 0)
		len = 0;

	error = copyout(p->p_comm, (char *)where + len, strlen(p->p_comm) + 1);
	if (error != 0)
		goto done;

	len = len + strlen(p->p_comm);
	len = (((u_long)where + len + 5) & ~0x3UL) - (u_long)where;

	/*
	 * Now copy in the actual argument vector, one page at a time,
	 * since we don't know how long the vector is (though, we do
	 * know how many NUL-terminated strings are in the vector).
	 */
	upper_bound = *sizep;
	nstr = nargv + nenv;
	for (; nstr != 0 && len < upper_bound; len += xlen) {
		aiov.iov_base = arg;
		aiov.iov_len = PAGE_SIZE;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = argv + len;
		xlen = PAGE_SIZE - ((argv + len) & PAGE_MASK);
		auio.uio_resid = xlen;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_rw = UIO_READ;
		auio.uio_procp = NULL;
		error = uvm_io(&p->p_vmspace->vm_map, &auio);
		if (error)
			goto done;

		for (i = 0;i < xlen && nstr != 0; i++) {
			if (arg[i] == '\0')
				nstr--;	/* one full string */
		}

		/*
		 * Make sure we don't copyout past the end of the user's
		 * buffer.
		 */
		if (len + i > upper_bound)
			i = upper_bound - len;

		if ((error = copyout(arg, (char *)where + len, i)) != 0)
			break;

		if (nstr == 0) {
			len += i;
			break;
		}
	}

done:
	uvmspace_free(p->p_vmspace);

	free(arg, M_TEMP);
	return (error);
}
