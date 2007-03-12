/*	$NetBSD: irix_prctl.c,v 1.35.2.1 2007/03/12 05:52:12 rmind Exp $ */

/*-
 * Copyright (c) 2001-2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: irix_prctl.c,v 1.35.2.1 2007/03/12 05:52:12 rmind Exp $");

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/lock.h>
#include <sys/filedesc.h>
#include <sys/vnode.h>
#include <sys/resourcevar.h>
#include <sys/kauth.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>

#include <machine/regnum.h>
#include <machine/vmparam.h>

#include <compat/svr4/svr4_types.h>

#include <compat/irix/irix_types.h>
#include <compat/irix/irix_exec.h>
#include <compat/irix/irix_prctl.h>
#include <compat/irix/irix_signal.h>
#include <compat/irix/irix_syscallargs.h>

struct irix_sproc_child_args {
	struct proc **isc_proc;
	void *isc_entry;
	void *isc_arg;
	size_t isc_len;
	int isc_inh;
	struct lwp *isc_parent_lwp;
	struct irix_share_group *isc_share_group;
	int isc_child_done;
};
static void irix_sproc_child __P((struct irix_sproc_child_args *));
static int irix_sproc __P((void *, unsigned int, void *, void *, size_t,
    pid_t, struct lwp *, register_t *));
static struct irix_shared_regions_rec *irix_isrr_create __P((vaddr_t,
    vsize_t, int));
#ifdef DEBUG_IRIX
static void irix_isrr_debug __P((struct proc *));
#endif
static void irix_isrr_cleanup __P((struct proc *));

int
irix_sys_prctl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct irix_sys_prctl_args /* {
		syscallarg(unsigned) option;
		syscallarg(void *) arg1;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	unsigned int option = SCARG(uap, option);

#ifdef DEBUG_IRIX
	printf("irix_sys_prctl(): option = %d\n", option);
#endif

	switch(option) {
	case IRIX_PR_GETSHMASK:	{	/* Get shared resources */
		struct proc *p2;
		int shmask = 0;
		struct irix_emuldata *ied;

		p2 = pfind((pid_t)SCARG(uap, arg1));

		if (p2 == p || SCARG(uap, arg1) == 0) {
			/* XXX return our own shmask */
			return 0;
		}

		if (p2 == NULL)
			return EINVAL;

		ied = (struct irix_emuldata *)p->p_emuldata;
		if (ied->ied_shareaddr)
			shmask |= IRIX_PR_SADDR;
		if (p->p_fd == p2->p_fd)
			shmask |= IRIX_PR_SFDS;
		if (p->p_cwdi == p2->p_cwdi)
			shmask |= (IRIX_PR_SDIR|IRIX_PR_SUMASK);

		*retval = (register_t)shmask;
		return 0;
		break;
	}

	case IRIX_PR_LASTSHEXIT:  	/* "Last sproc exit" */
		/* We no nothing */
		break;

	case IRIX_PR_GETNSHARE: {	/* Number of sproc share group memb.*/
		struct irix_emuldata *ied;
		struct irix_emuldata *iedp;
		struct irix_share_group *isg;
		int count;

		ied = (struct irix_emuldata *)p->p_emuldata;
		if ((isg = ied->ied_share_group) == NULL) {
			*retval = 0;
			return 0;
		}

		count = 0;
		(void)lockmgr(&isg->isg_lock, LK_SHARED, NULL);
		LIST_FOREACH(iedp, &isg->isg_head, ied_sglist)
			count++;
		(void)lockmgr(&isg->isg_lock, LK_RELEASE, NULL);

		*retval = count;
		return 0;
		break;
	}

	case IRIX_PR_TERMCHILD: {	/* Get SIGHUP when parent's exit */
		struct irix_emuldata *ied;

		ied = (struct irix_emuldata *)(p->p_emuldata);
		ied->ied_termchild = 1;
		break;
	}

	case IRIX_PR_ISBLOCKED: {	/* Is process blocked? */
		pid_t pid = (pid_t)SCARG(uap, arg1);
		struct irix_emuldata *ied;
		struct proc *target;
		kauth_cred_t pc;

		if (pid == 0)
			pid = p->p_pid;

		if ((target = pfind(pid)) == NULL)
			return ESRCH;

		if (irix_check_exec(target) == 0)
			return 0;

		pc = l->l_cred;
		if (!(kauth_authorize_generic(pc, KAUTH_GENERIC_ISSUSER, NULL) == 0 || \
		    kauth_cred_getuid(pc) == kauth_cred_getuid(target->p_cred) || \
		    kauth_cred_geteuid(pc) == kauth_cred_getuid(target->p_cred) || \
		    kauth_cred_getuid(pc) == kauth_cred_geteuid(target->p_cred) || \
		    kauth_cred_geteuid(pc) == kauth_cred_geteuid(target->p_cred)))
			return EPERM;

		ied = (struct irix_emuldata *)(target->p_emuldata);
		*retval = (ied->ied_procblk_count < 0);
		return 0;
		break;
	}

	default:
		printf("Warning: call to unimplemented prctl() command %d\n",
		    option);
		return EINVAL;
		break;
	}
	return 0;
}


int
irix_sys_pidsprocsp(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct irix_sys_pidsprocsp_args /* {
		syscallarg(void *) entry;
		syscallarg(unsigned) inh;
		syscallarg(void *) arg;
		syscallarg(void *) sp;
		syscallarg(irix_size_t) len;
		syscallarg(irix_pid_t) pid;
	} */ *uap = v;

	/* pid is ignored for now */
	printf("Warning: unsupported pid argument to IRIX sproc\n");

	return irix_sproc(SCARG(uap, entry), SCARG(uap, inh), SCARG(uap, arg),
	    SCARG(uap, sp), SCARG(uap, len), SCARG(uap, pid), l, retval);
}

int
irix_sys_sprocsp(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct irix_sys_sprocsp_args /* {
		syscallarg(void *) entry;
		syscallarg(unsigned) inh;
		syscallarg(void *) arg;
		syscallarg(void *) sp;
		syscallarg(irix_size_t) len;
	} */ *uap = v;

	return irix_sproc(SCARG(uap, entry), SCARG(uap, inh), SCARG(uap, arg),
	    SCARG(uap, sp), SCARG(uap, len), 0, l, retval);
}

int
irix_sys_sproc(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct irix_sys_sproc_args /* {
		syscallarg(void *) entry;
		syscallarg(unsigned) inh;
		syscallarg(void *) arg;
	} */ *uap = v;
	struct proc *p = l->l_proc;

	return irix_sproc(SCARG(uap, entry), SCARG(uap, inh), SCARG(uap, arg),
	    NULL, p->p_rlimit[RLIMIT_STACK].rlim_cur, 0, l, retval);
}


static int
irix_sproc(entry, inh, arg, sp, len, pid, l, retval)
	void *entry;
	unsigned int inh;
	void *arg;
	void *sp;
	size_t len;
	pid_t pid;
	struct lwp *l;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	int bsd_flags = 0;
	struct exec_vmcmd vmc;
	int error;
	struct proc *p2;
	struct irix_sproc_child_args *isc;
	struct irix_emuldata *ied;
	struct irix_emuldata *iedp;
	struct irix_share_group *isg = NULL;
	segsz_t stacksize;

#ifdef DEBUG_IRIX
	printf("irix_sproc(): entry = %p, inh = %x, arg = %p, sp = 0x%08lx, len = 0x%08lx, pid = %d\n", entry, inh, arg, (u_long)sp, (u_long)len, pid);
#endif

	if (len == 0)
		return EINVAL;

	if (inh & IRIX_PR_SFDS)
		bsd_flags |= FORK_SHAREFILES;
	if (inh & (IRIX_PR_SUMASK|IRIX_PR_SDIR)) {
		bsd_flags |= FORK_SHARECWD;
		/* Forget them so that we don't get warning below */
		inh &= ~(IRIX_PR_SUMASK|IRIX_PR_SDIR);
	}
	/* We know how to do PR_SUMASK and PR_SDIR together only */
	if (inh & IRIX_PR_SUMASK)
		printf("Warning: unimplemented IRIX sproc flag PR_SUMASK\n");
	if (inh & IRIX_PR_SDIR)
		printf("Warning: unimplemented IRIX sproc flag PR_SDIR\n");

	/*
	 * If relevant, initialize the share group structure
	 */
	ied = (struct irix_emuldata *)(p->p_emuldata);
	if (ied->ied_share_group == NULL) {
		isg = malloc(sizeof(struct irix_share_group),
		    M_EMULDATA, M_WAITOK);
		lockinit(&isg->isg_lock, PZERO|PCATCH, "sharegroup", 0, 0);
		isg->isg_refcount = 0;

		(void)lockmgr(&isg->isg_lock, LK_EXCLUSIVE, NULL);
		LIST_INIT(&isg->isg_head);
		LIST_INSERT_HEAD(&isg->isg_head, ied, ied_sglist);
		isg->isg_refcount++;
		(void)lockmgr(&isg->isg_lock, LK_RELEASE, NULL);

		ied->ied_share_group = isg;
	}

	/*
	 * Setting up child stack
	 */
	if (inh & IRIX_PR_SADDR) {
		if (sp == NULL) {
			/*
			 * All share group  members have vm_maxsaddr set
			 * to the bottom of the lowest stack in address space,
			 * therefore we map the new stack there.
			 */
			sp = p->p_vmspace->vm_maxsaddr;

			/* Compute new stacks's bottom address */
			sp = (void *)trunc_page((u_long)sp - len);
		}

		/* Now map the new stack */
		bzero(&vmc, sizeof(vmc));
		vmc.ev_addr = trunc_page((u_long)sp);
		vmc.ev_len = round_page(len);
		vmc.ev_prot = UVM_PROT_RWX;
		vmc.ev_flags = UVM_FLAG_COPYONW|UVM_FLAG_FIXED|UVM_FLAG_OVERLAY;
		vmc.ev_proc = vmcmd_map_zero;
#ifdef DEBUG_IRIX
		printf("irix_sproc(): new stack addr=0x%08lx, len=0x%08lx\n",
		    (u_long)sp, (u_long)len);
#endif
		/* Normally it cannot be NULL since we just initialized it */
		if ((isg = ied->ied_share_group) == NULL)
			panic("irix_sproc: NULL ied->ied_share_group");

		IRIX_VM_SYNC(p, error = (*vmc.ev_proc)(l, &vmc));
		if (error)
			return error;

		/* Update stack parameters for the share group members */
		ied = (struct irix_emuldata *)p->p_emuldata;
		stacksize = ((char *)p->p_vmspace->vm_minsaddr - (char *)sp)
		    / PAGE_SIZE;


		(void)lockmgr(&isg->isg_lock, LK_EXCLUSIVE, NULL);
		LIST_FOREACH(iedp, &isg->isg_head, ied_sglist) {
			iedp->ied_p->p_vmspace->vm_maxsaddr = (void *)sp;
			iedp->ied_p->p_vmspace->vm_ssize = stacksize;
		}
		(void)lockmgr(&isg->isg_lock, LK_RELEASE, NULL);
	}

	/*
	 * Arguments for irix_sproc_child()
	 * This will be freed by the child.
	 */
	isc = malloc(sizeof(*isc), M_TEMP, M_WAITOK);
	isc->isc_proc = &p2;
	isc->isc_entry = entry;
	isc->isc_arg = arg;
	isc->isc_len = len;
	isc->isc_inh = inh;
	isc->isc_parent_lwp = l;
	isc->isc_share_group = isg;
	isc->isc_child_done = 0;

	if (inh & IRIX_PR_SADDR) {
		ied->ied_shareaddr = 1;
	}

	if ((error = fork1(l, bsd_flags, SIGCHLD, (void *)sp, len,
	    (void *)irix_sproc_child, (void *)isc, retval, &p2)) != 0)
		return error;

	/*
	 * The child needs the parent to stay alive until it has
	 * copied a few things from it. We sleep whatever happen
	 * until the child is done.
	 */
	while (!isc->isc_child_done)
		(void)tsleep(&isc->isc_child_done, PZERO, "sproc", 0);
	free(isc, M_TEMP);

	retval[0] = (register_t)p2->p_pid;
	retval[1] = 0;

	return 0;
}

static void
irix_sproc_child(isc)
	struct irix_sproc_child_args *isc;
{
	struct proc *p2 = *isc->isc_proc;
	struct lwp *l2 = curlwp;
	int inh = isc->isc_inh;
	struct lwp *lparent = isc->isc_parent_lwp;
	struct proc *parent = lparent->l_proc;
	struct frame *tf = (struct frame *)l2->l_md.md_regs;
	struct frame *ptf = (struct frame *)lparent->l_md.md_regs;
	kauth_cred_t pc;
	struct plimit *pl;
	struct irix_emuldata *ied;
	struct irix_emuldata *parent_ied;

#ifdef DEBUG_IRIX
	printf("irix_sproc_child()\n");
#endif
	/*
	 * Handle shared VM space. The process private arena is not shared
	 */
	if (inh & IRIX_PR_SADDR) {
		int error;
		vaddr_t minp, maxp;
		vsize_t len;
		struct irix_shared_regions_rec *isrr;

		/*
		 * First, unmap the whole address space
		 */
		minp = vm_map_min(&p2->p_vmspace->vm_map);
		maxp = vm_map_max(&p2->p_vmspace->vm_map);
		uvm_unmap(&p2->p_vmspace->vm_map, minp, maxp);

		/*
		 * Now, copy the mapping from the parent for shared regions
		 */
		parent_ied = (struct irix_emuldata *)parent->p_emuldata;
		LIST_FOREACH(isrr, &parent_ied->ied_shared_regions, isrr_list) {
			minp = isrr->isrr_start;
			len = isrr->isrr_len;
			maxp = minp + len;
			/* If this is a private region, skip */
			if (isrr->isrr_shared == IRIX_ISRR_PRIVATE)
				continue;

			/* Copy the new mapping from the parent */
			error = uvm_map_extract(&parent->p_vmspace->vm_map,
			    minp, len, &p2->p_vmspace->vm_map, &minp, 0);
			if (error != 0) {
#ifdef DEBUG_IRIX
				printf("irix_sproc_child(): error %d\n", error);
#endif
				isc->isc_child_done = 1;
				wakeup(&isc->isc_child_done);
				killproc(p2,
				    "failed to initialize share group VM");
			}
		}

		/* Map and initialize the process private arena (unshared) */
		error = irix_prda_init(p2);
		if (error != 0) {
			isc->isc_child_done = 1;
			wakeup(&isc->isc_child_done);
			killproc(p2, "failed to initialize the PRDA");
		}
	}

	/*
	 * Handle shared process UID/GID
	 */
	if (inh & IRIX_PR_SID) {
		pc = p2->p_cred;
		kauth_cred_hold(parent->p_cred);
		p2->p_cred = parent->p_cred;
		kauth_cred_free(pc);
	}

	/*
	 * Handle shared process limits
	 */
	if (inh & IRIX_PR_SULIMIT) {
		pl = p2->p_limit;
		parent->p_limit->p_refcnt++;
		p2->p_limit = parent->p_limit;
		if(--pl->p_refcnt == 0)
			limfree(pl);
	}

	/*
	 * Setup PC to return to the child entry point
	 */
	tf->f_regs[_R_PC] = (unsigned long)isc->isc_entry;
	tf->f_regs[_R_RA] = 0;

	/*
	 * Setup child arguments
	 */
	tf->f_regs[_R_A0] = (unsigned long)isc->isc_arg;
	tf->f_regs[_R_A1] = 0;
	tf->f_regs[_R_A2] = 0;
	tf->f_regs[_R_A3] = 0;
	if (ptf->f_regs[_R_S3] == (unsigned long)isc->isc_len) {
		tf->f_regs[_R_S0] = ptf->f_regs[_R_S0];
		tf->f_regs[_R_S1] = ptf->f_regs[_R_S1];
		tf->f_regs[_R_S2] = ptf->f_regs[_R_S2];
		tf->f_regs[_R_S3] = ptf->f_regs[_R_S3];
	}

	/*
	 * Join the share group
	 */
	ied = (struct irix_emuldata *)(p2->p_emuldata);
	parent_ied = (struct irix_emuldata *)(parent->p_emuldata);
	ied->ied_share_group = parent_ied->ied_share_group;

	(void)lockmgr(&ied->ied_share_group->isg_lock, LK_EXCLUSIVE, NULL);
	LIST_INSERT_HEAD(&ied->ied_share_group->isg_head, ied, ied_sglist);
	ied->ied_share_group->isg_refcount++;
	(void)lockmgr(&ied->ied_share_group->isg_lock, LK_RELEASE, NULL);

	if (inh & IRIX_PR_SADDR)
		ied->ied_shareaddr = 1;

	/*
	 * wakeup the parent as it can now die without
	 * causing a panic in the child.
	 */
	isc->isc_child_done = 1;
	wakeup(&isc->isc_child_done);

	/*
	 * Return to userland for a newly created process
	 */
	child_return((void *)l2);
	return;
}

int
irix_sys_procblk(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct irix_sys_procblk_args /* {
		syscallarg(int) cmd;
		syscallarg(pid_t) pid;
		syscallarg(int) count;
	} */ *uap = v;
	int cmd = SCARG(uap, cmd);
	struct irix_emuldata *ied;
	struct irix_emuldata *iedp;
	struct irix_share_group *isg;
	struct proc *target;
	kauth_cred_t pc;
	int oldcount;
	struct lwp *ied_lwp;
	int error, last_error;
	struct irix_sys_procblk_args cup;

	/* Find the process */
	if ((target = pfind(SCARG(uap, pid))) == NULL)
		return ESRCH;

	/* May we stop it? */
	pc = l->l_cred;
	if (!(kauth_authorize_generic(pc, KAUTH_GENERIC_ISSUSER, NULL) == 0 || \
	    kauth_cred_getuid(pc) == kauth_cred_getuid(target->p_cred) || \
	    kauth_cred_geteuid(pc) == kauth_cred_getuid(target->p_cred) || \
	    kauth_cred_getuid(pc) == kauth_cred_geteuid(target->p_cred) || \
	    kauth_cred_geteuid(pc) == kauth_cred_geteuid(target->p_cred)))
		return EPERM;

	/* Is it an IRIX process? */
	if (irix_check_exec(target) == 0)
		return EPERM;

	ied = (struct irix_emuldata *)(target->p_emuldata);
	oldcount = ied->ied_procblk_count;

	switch (cmd) {
	case IRIX_PROCBLK_BLOCK:
		ied->ied_procblk_count--;
		break;

	case IRIX_PROCBLK_UNBLOCK:
		ied->ied_procblk_count++;
		break;

	case IRIX_PROCBLK_COUNT:
		if (SCARG(uap, count) > IRIX_PR_MAXBLOCKCNT ||
		    SCARG(uap, count) < IRIX_PR_MINBLOCKCNT)
			return EINVAL;
		ied->ied_procblk_count = SCARG(uap, count);
		break;

	case IRIX_PROCBLK_BLOCKALL:
	case IRIX_PROCBLK_UNBLOCKALL:
	case IRIX_PROCBLK_COUNTALL:
		SCARG(&cup, cmd) = cmd -IRIX_PROCBLK_ONLYONE;
		SCARG(&cup, count) = SCARG(uap, count);
		last_error = 0;

		/*
		 * If the process does not belong to a
		 * share group, do it just for the process
		 */
		if ((isg = ied->ied_share_group) == NULL) {
			SCARG(&cup, pid) = SCARG(uap, pid);
			return irix_sys_procblk(l, &cup, retval);
		}

		(void)lockmgr(&isg->isg_lock, LK_SHARED, NULL);
		LIST_FOREACH(iedp, &isg->isg_head, ied_sglist) {
			/* Recall procblk for this process */
			SCARG(&cup, pid) = iedp->ied_p->p_pid;
			ied_lwp = proc_representative_lwp(iedp->ied_p, NULL, 0);
			error = irix_sys_procblk(ied_lwp, &cup, retval);
			if (error != 0)
				last_error = error;
		}
		(void)lockmgr(&isg->isg_lock, LK_RELEASE, NULL);

		return last_error;
		break;
	default:
		printf("Warning: unimplemented IRIX procblk command %d\n", cmd);
		return EINVAL;
		break;
	}

	/*
	 * We emulate the process block/unblock using SIGSTOP and SIGCONT
	 * signals. This is not very accurate, since on IRIX theses way
	 * of blocking a process are completely separated.
	 */
	if (oldcount >= 0 && ied->ied_procblk_count < 0) /* blocked */
		psignal(target, SIGSTOP);

	if (oldcount < 0 && ied->ied_procblk_count >= 0) /* unblocked */
		psignal(target, SIGCONT);

	return 0;
}

int
irix_prda_init(p)
	struct proc *p;
{
	int error;
	struct exec_vmcmd evc;
	struct irix_prda *ip;
	struct irix_prda_sys ips;
	struct lwp *l;

	bzero(&evc, sizeof(evc));
	evc.ev_addr = (u_long)IRIX_PRDA;
	evc.ev_len = sizeof(struct irix_prda);
	evc.ev_prot = UVM_PROT_RW;
	evc.ev_proc = *vmcmd_map_zero;
	l = proc_representative_lwp(p, NULL, 0);

	if ((error = (*evc.ev_proc)(l, &evc)) != 0)
		return error;

	ip = (struct irix_prda *)IRIX_PRDA;
	bzero(&ips, sizeof(ips));

	ips.t_pid = p->p_pid;
	/*
	 * The PRDA ID must be unique for a PRDA. IRIX uses a small
	 * integer, but we don't know how it is chosen. The PID
	 * should be unique enough to get the work done.
	 */
	ips.t_prid = p->p_pid;

	error = copyout(&ips, (void *)&ip->sys_prda.prda_sys, sizeof(ips));
	if (error)
		return error;

	/* Remeber the PRDA is private */
	irix_isrr_insert((vaddr_t)IRIX_PRDA, sizeof(ips), IRIX_ISRR_PRIVATE, p);

	return 0;
}

int
irix_vm_fault(p, vaddr, access_type)
	struct proc *p;
	vaddr_t vaddr;
	vm_prot_t access_type;
{
	int error;
	struct irix_emuldata *ied;
	struct vm_map *map;

	ied = (struct irix_emuldata *)p->p_emuldata;
	map = &p->p_vmspace->vm_map;

	if (ied->ied_share_group == NULL || ied->ied_shareaddr == 0)
		return uvm_fault(map, vaddr, access_type);

	/* share group version */
	(void)lockmgr(&ied->ied_share_group->isg_lock, LK_EXCLUSIVE, NULL);
	error = uvm_fault(map, vaddr, access_type);
	irix_vm_sync(p);
	(void)lockmgr(&ied->ied_share_group->isg_lock, LK_RELEASE, NULL);

	return error;
}

/*
 * Propagate changes to address space to other members of the share group
 */
void
irix_vm_sync(p)
	struct proc *p;
{
	struct proc *pp;
	struct irix_emuldata *iedp;
	struct irix_emuldata *ied = (struct irix_emuldata *)p->p_emuldata;
	struct irix_shared_regions_rec *isrr;
	vaddr_t minp;
	vaddr_t maxp;
	vsize_t len;
	int error;

	LIST_FOREACH(iedp, &ied->ied_share_group->isg_head, ied_sglist) {
		if (iedp->ied_shareaddr != 1 || iedp->ied_p == p)
			continue;

		pp = iedp->ied_p;

		error = 0;
		/* for each region in the target process ... */
		LIST_FOREACH(isrr, &iedp->ied_shared_regions, isrr_list) {
			/* skip regions private to the target process */
			if (isrr->isrr_shared == IRIX_ISRR_PRIVATE)
				continue;

			/*
			 * XXX We should also skip regions private to the
			 * original process...
			 */

			/* The region is shared */
			minp = isrr->isrr_start;
			len = isrr->isrr_len;
			maxp = minp + len;

			/* Drop the region */
			uvm_unmap(&pp->p_vmspace->vm_map, minp, maxp);

			/* Clone it from the parent */
			error = uvm_map_extract(&p->p_vmspace->vm_map,
			    minp, len, &pp->p_vmspace->vm_map, &minp, 0);

			if (error)
				break;
		}

		if (error)
			killproc(pp, "failed to keep share group VM in sync");
	}

	return;
}

static struct irix_shared_regions_rec *
irix_isrr_create(start, len, shared)
	vaddr_t start;
	vsize_t len;
	int shared;
{
	struct irix_shared_regions_rec *new_isrr;

	new_isrr = malloc(sizeof(struct irix_shared_regions_rec),
	    M_EMULDATA, M_WAITOK);
	new_isrr->isrr_start = start;
	new_isrr->isrr_len = len;
	new_isrr->isrr_shared = shared;

	return new_isrr;
}

/*
 * Insert a record for a new region in the list. The new region may be
 * overlaping or be included in an existing region.
 */
void
irix_isrr_insert(start, len, shared, p)
	vaddr_t start;
	vsize_t len;
	int shared;
	struct proc *p;
{
	struct irix_emuldata *ied = (struct irix_emuldata *)p->p_emuldata;
	struct irix_shared_regions_rec *isrr;
	struct irix_shared_regions_rec *new_isrr;
	vaddr_t end, cur_start, cur_end;
	int cur_shared;

	start = trunc_page(start);
	len = round_page(len);
	end = start + len;

	new_isrr = irix_isrr_create(start, len, shared);

	/* Do we need to insert the new region at the begining of the list? */
	if (LIST_EMPTY(&ied->ied_shared_regions) ||
	    LIST_FIRST(&ied->ied_shared_regions)->isrr_start > start) {
		LIST_INSERT_HEAD(&ied->ied_shared_regions, new_isrr, isrr_list);
	} else {
		/* Find the place where to insert it */
		LIST_FOREACH(isrr, &ied->ied_shared_regions, isrr_list) {
			cur_start = isrr->isrr_start;
			cur_end = isrr->isrr_start + isrr->isrr_len;
			cur_shared = isrr->isrr_shared;

			/*
			 * if there is no intersection between inserted
			 * and current region: skip to next region
			 */
			if (cur_end <= start)
				continue;

			/*
			 * if new region is included into the current
			 * region. Right-crop the current region,
			 * insert a new one, and insert a new region
			 * for the end of the split region
			 */
			if (cur_end > end && cur_start < start) {
				isrr->isrr_len = start - isrr->isrr_start;
				LIST_INSERT_AFTER(isrr, new_isrr, isrr_list);
				isrr = new_isrr;

				new_isrr = irix_isrr_create(end,
				    cur_end - end, cur_shared);
				LIST_INSERT_AFTER(isrr, new_isrr, isrr_list);

				/* Nothing more to do, exit now */
#ifdef DEBUG_IRIX
				irix_isrr_debug(p);
#endif
				irix_isrr_cleanup(p);
#ifdef DEBUG_IRIX
				irix_isrr_debug(p);
#endif
				return;
			}

			/*
			 * if inserted block overlap some part
			 * of current region: right-crop current region
			 * and insert the new region
			 */
			if (start < cur_end) {
				isrr->isrr_len = start - cur_start;
				LIST_INSERT_AFTER(isrr, new_isrr, isrr_list);

				/* exit the FOREACH loop */
				break;
			}
		}
	}

	/*
	 * At this point, we inserted the new region (new_isrr) but
	 * it may be overlaping with next regions, so we need to clean
	 * this up and remove or crop next regions
	 */
	LIST_FOREACH(isrr, &ied->ied_shared_regions, isrr_list) {
		cur_start = isrr->isrr_start;
		cur_end = isrr->isrr_start + isrr->isrr_len;

		/* skip until we get beyond new_isrr */
		if (cur_start <= start)
			continue;

		if (end >= cur_end) { /* overlap */
			LIST_REMOVE(isrr, isrr_list);
			free(isrr, M_EMULDATA);
			/* isrr is now invalid */
			isrr = new_isrr;
			continue;
		}

		/*
		 * Here end < cur_end, therefore we need to
		 * right-crop the current region
		 */
		 isrr->isrr_start = end;
		 isrr->isrr_len = cur_end - end;
		 break;
	}
#ifdef DEBUG_IRIX
	irix_isrr_debug(p);
#endif
	irix_isrr_cleanup(p);
#ifdef DEBUG_IRIX
	irix_isrr_debug(p);
#endif
	return;
}

/*
 * Cleanup the region list by
 * (1) removing regions with length 0, and
 * (2) merging contiguous regions with the same status
 */
static void
irix_isrr_cleanup(p)
	struct proc *p;
{
	struct irix_emuldata *ied = (struct irix_emuldata *)p->p_emuldata;
	struct irix_shared_regions_rec *isrr;
	struct irix_shared_regions_rec *new_isrr;

	isrr = LIST_FIRST(&ied->ied_shared_regions);
	do {
		new_isrr = LIST_NEXT(isrr, isrr_list);

		if (isrr->isrr_len == 0) {
			LIST_REMOVE(isrr, isrr_list);
			free(isrr, M_EMULDATA);
			isrr = new_isrr;
			if (isrr == NULL)
				break;
		}

		if (new_isrr == NULL)
			break;

		if (isrr->isrr_shared == new_isrr->isrr_shared) {
			isrr->isrr_len += new_isrr->isrr_len;
			new_isrr->isrr_len = 0;
		}

		isrr = new_isrr;
	} while (1);

	return;
}


#ifdef DEBUG_IRIX
static void
irix_isrr_debug(p)
	struct proc *p;
{
	struct irix_emuldata *ied = (struct irix_emuldata *)p->p_emuldata;
	struct irix_shared_regions_rec *isrr;

	printf("isrr for pid %d\n", p->p_pid);
	LIST_FOREACH(isrr, &ied->ied_shared_regions, isrr_list) {
		printf("  addr = %p, len = %p, shared = %d\n",
		    (void *)isrr->isrr_start,
		    (void *)isrr->isrr_len,
		    isrr->isrr_shared);
	}
}
#endif /* DEBUG_IRIX */
