/*	$NetBSD: irix_prctl.c,v 1.17 2002/09/21 21:14:57 manu Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: irix_prctl.c,v 1.17 2002/09/21 21:14:57 manu Exp $");

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
	struct proc *isc_parent;
	struct irix_share_group *isc_share_group;
	int isc_child_done;
}; 
static void irix_sproc_child __P((struct irix_sproc_child_args *));
static int irix_sproc __P((void *, unsigned int, void *, caddr_t, size_t, 
    pid_t, struct proc *, register_t *));

int
irix_sys_prctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_prctl_args /* {
		syscallarg(int) option;
		syscallarg(void *) arg1;
	} */ *uap = v;
	int option = SCARG(uap, option);

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
		if (p->p_cwdi == p2->p_cwdi);
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
		struct pcred *pc;

		if (pid == 0)
			pid = p->p_pid;

		if ((target = pfind(pid)) == NULL)
			return ESRCH;

		if (irix_check_exec(target) == 0)
			return 0;

		pc = p->p_cred;
		if (!(pc->pc_ucred->cr_uid == 0 || \
		    pc->p_ruid == target->p_cred->p_ruid || \
		    pc->pc_ucred->cr_uid == target->p_cred->p_ruid || \
		    pc->p_ruid == target->p_ucred->cr_uid || \
		    pc->pc_ucred->cr_uid == target->p_ucred->cr_uid))
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
irix_sys_pidsprocsp(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_pidsprocsp_args /* {
		syscallarg(void *) entry;
		syscallarg(unsigned) inh;
		syscallarg(void *) arg;
		syscallarg(caddr_t) sp;
		syscallarg(irix_size_t) len;
		syscallarg(irix_pid_t) pid;
	} */ *uap = v;
	
	/* pid is ignored for now */
	printf("Warning: unsupported pid argument to IRIX sproc\n");

	return irix_sproc(SCARG(uap, entry), SCARG(uap, inh), SCARG(uap, arg),
	    SCARG(uap, sp), SCARG(uap, len), SCARG(uap, pid), p, retval);
}

int
irix_sys_sprocsp(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_sprocsp_args /* {
		syscallarg(void *) entry;
		syscallarg(unsigned) inh;
		syscallarg(void *) arg;
		syscallarg(caddr_t) sp;
		syscallarg(irix_size_t) len;
	} */ *uap = v;

	return irix_sproc(SCARG(uap, entry), SCARG(uap, inh), SCARG(uap, arg),
	    SCARG(uap, sp), SCARG(uap, len), 0, p, retval);
}

int
irix_sys_sproc(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct irix_sys_sproc_args /* {
		syscallarg(void *) entry;
		syscallarg(unsigned) inh;
		syscallarg(void *) arg;
	} */ *uap = v;

	return irix_sproc(SCARG(uap, entry), SCARG(uap, inh), SCARG(uap, arg),
	    NULL, p->p_rlimit[RLIMIT_STACK].rlim_cur, 0, p, retval);
}


static int 
irix_sproc(entry, inh, arg, sp, len, pid, p, retval)
	void *entry;
	unsigned int inh;
	void *arg;
	caddr_t sp;
	size_t len;
	pid_t pid;
	struct proc *p;
	register_t *retval;
{
	int bsd_flags = 0;
	struct exec_vmcmd vmc;
	int error;
	struct proc *p2;
	struct irix_sproc_child_args *isc;	
	struct irix_emuldata *ied;
	struct irix_emuldata *iedp;
	struct irix_share_group *isg;
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
	 * If revelant, initialize the share group structure
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
			sp = (caddr_t)trunc_page((u_long)sp - len);
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

		IRIX_VM_SYNC(p, error = (*vmc.ev_proc)(p, &vmc));
		if (error)
			return error;

		/* Update stack parameters for the share group members */
		ied = (struct irix_emuldata *)p->p_emuldata;
		stacksize = (p->p_vmspace->vm_minsaddr - sp) / PAGE_SIZE;


		(void)lockmgr(&isg->isg_lock, LK_EXCLUSIVE, NULL);
		LIST_FOREACH(iedp, &isg->isg_head, ied_sglist) {
			iedp->ied_p->p_vmspace->vm_maxsaddr = (caddr_t)sp;
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
	isc->isc_parent = p;
	isc->isc_share_group = isg;
	isc->isc_child_done = 0;

	if (inh & IRIX_PR_SADDR)
		ied->ied_shareaddr = 1;

	if ((error = fork1(p, bsd_flags, SIGCHLD, (void *)sp, len, 
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
	int inh = isc->isc_inh;
	struct proc *parent = isc->isc_parent;
	struct frame *tf = (struct frame *)p2->p_md.md_regs;
	struct frame *ptf = (struct frame *)parent->p_md.md_regs;
	struct pcred *pc;
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
		vaddr_t vm_min;
		vsize_t vm_len;

		vm_min = vm_map_min(&parent->p_vmspace->vm_map);
		vm_len = vm_map_max(&parent->p_vmspace->vm_map) - vm_min;

		/* Drop the current VM space */
		uvm_unmap(&p2->p_vmspace->vm_map, vm_min, vm_min + vm_len);

		/* Clone the mapping from the parent */
		error = uvm_map_extract(&parent->p_vmspace->vm_map, 
		    vm_min, vm_len, &p2->p_vmspace->vm_map, &vm_min, 0);
		if (error != 0) {
#ifdef DEBUG_IRIX
			printf("irix_sproc_child(): error %d\n", error);
#endif
			isc->isc_child_done = 1;
			wakeup(&isc->isc_child_done);
			killproc(p2, "failed to initialize share group VM");
		}

		/* Unmap the process private arena (shared) */
		uvm_unmap(&p2->p_vmspace->vm_map, (vaddr_t)IRIX_PRDA,
		    (vaddr_t)((u_long)IRIX_PRDA + sizeof(struct irix_prda)));

		/* Remap the process private arena (unshared) */
		error = irix_prda_init(p2);
		if (error != 0) {
			isc->isc_child_done = 1;
			wakeup(&isc->isc_child_done);
			killproc(p2, "failed to initialize share group VM");
		}
	}

	/*
	 * Handle shared process UID/GID
	 */
	if (inh & IRIX_PR_SID) {
		pc = p2->p_cred;
		parent->p_cred->p_refcnt++;
		p2->p_cred = parent->p_cred;
		if (--pc->p_refcnt == 0) {
			crfree(pc->pc_ucred);	
			pool_put(&pcred_pool, pc);
		}
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
	tf->f_regs[PC] = (unsigned long)isc->isc_entry;
	tf->f_regs[RA] = 0;

	/* 
	 * Setup child arguments 
	 */
	tf->f_regs[A0] = (unsigned long)isc->isc_arg;
	tf->f_regs[A1] = 0;
	tf->f_regs[A2] = 0;
	tf->f_regs[A3] = 0;
	if (ptf->f_regs[S3] == (unsigned long)isc->isc_len) { 
		tf->f_regs[S0] = ptf->f_regs[S0];
		tf->f_regs[S1] = ptf->f_regs[S1];
		tf->f_regs[S2] = ptf->f_regs[S2];
		tf->f_regs[S3] = ptf->f_regs[S3];
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
	child_return((void *)p2);
	return;
}

int
irix_sys_procblk(p, v, retval)
	struct proc *p;
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
	struct pcred *pc;
	int oldcount;
	int error, last_error;
	struct irix_sys_procblk_args cup;

	/* Find the process */
	if ((target = pfind(SCARG(uap, pid))) == NULL)
		return ESRCH;

	/* May we stop it? */
	pc = p->p_cred;
	if (!(pc->pc_ucred->cr_uid == 0 || \
	    pc->p_ruid == target->p_cred->p_ruid || \
	    pc->pc_ucred->cr_uid == target->p_cred->p_ruid || \
	    pc->p_ruid == target->p_ucred->cr_uid || \
	    pc->pc_ucred->cr_uid == target->p_ucred->cr_uid))
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
			return irix_sys_procblk(p, &cup, retval);
		}

		(void)lockmgr(&isg->isg_lock, LK_SHARED, NULL);
		LIST_FOREACH(iedp, &isg->isg_head, ied_sglist) {
			/* Recall procblk for this process */
			SCARG(&cup, pid) = iedp->ied_p->p_pid;
			error = irix_sys_procblk(iedp->ied_p, &cup, retval);
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

	bzero(&evc, sizeof(evc));
	evc.ev_addr = (u_long)IRIX_PRDA;
	evc.ev_len = sizeof(struct irix_prda);
	evc.ev_prot = UVM_PROT_RW;
	evc.ev_proc = *vmcmd_map_zero;

	if ((error = (*evc.ev_proc)(p, &evc)) != 0)
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

	return error;
}

int
irix_vm_fault(p, vaddr, fault_type, access_type)
	struct proc *p;
	vaddr_t vaddr;
	vm_fault_t fault_type;
	vm_prot_t access_type;
{
	int error;
	struct irix_emuldata *ied;
	struct vm_map *map;

	ied = (struct irix_emuldata *)p->p_emuldata;
	map = &p->p_vmspace->vm_map;

	if (ied->ied_share_group == NULL || ied->ied_shareaddr == 0)
		return uvm_fault(map, vaddr, fault_type, access_type);

	/* share group version */
	(void)lockmgr(&ied->ied_share_group->isg_lock, LK_EXCLUSIVE, NULL);
	error = uvm_fault(map, vaddr, fault_type, access_type);
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
	vaddr_t low_min = vm_map_min(&p->p_vmspace->vm_map);
	vaddr_t low_max = (vaddr_t)IRIX_PRDA;
	vsize_t low_len = low_max - low_min;
	vaddr_t top_min = (vaddr_t)IRIX_PRDA + sizeof(struct irix_prda);
	vaddr_t top_max = vm_map_max(&p->p_vmspace->vm_map);
	vsize_t top_len = top_max - top_min;
	int error;

	LIST_FOREACH(iedp, &ied->ied_share_group->isg_head, ied_sglist) {
		if (iedp->ied_shareaddr != 1 || iedp->ied_p == p)
			continue;

		pp = iedp->ied_p;

		/* Drop the current VM space except the PRDA */
		uvm_unmap(&pp->p_vmspace->vm_map, low_min, low_max);
		uvm_unmap(&pp->p_vmspace->vm_map, top_min, top_max);

		/* Clone the mapping from the fault initiator, except PRDA */
		if ((error = uvm_map_extract(&p->p_vmspace->vm_map,
		    low_min, low_len, &pp->p_vmspace->vm_map, &low_min, 0)) ||
		    (error = uvm_map_extract(&p->p_vmspace->vm_map,
		    top_min, top_len, &pp->p_vmspace->vm_map, &top_min, 0)))
			killproc(pp, "failed to keep share group VM in sync");
	}

	return;
}
