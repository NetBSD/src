/*	$NetBSD: darwin_sysctl.c,v 1.24 2004/03/29 21:43:28 christos Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: darwin_sysctl.c,v 1.24 2004/03/29 21:43:28 christos Exp $");

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
#include <compat/darwin/darwin_proc.h>
#include <compat/darwin/darwin_syscallargs.h>

pid_t darwin_init_pid = 0;
int darwin_ioframebuffer_unit = 0;
int darwin_ioframebuffer_screen = 0;
int darwin_iohidsystem_mux = 0;
static char *darwin_sysctl_hw_machine = "Power Macintosh";

static int darwin_sysctl_dokproc(SYSCTLFN_PROTO);
static void darwin_fill_kproc(struct proc *, struct darwin_kinfo_proc *);
static void native_to_darwin_pflag(int *, int);
static int darwin_sysctl_procargs(SYSCTLFN_PROTO);

static struct sysctlnode darwin_sysctl_root = {
	.sysctl_flags = SYSCTL_VERSION|CTLFLAG_ROOT|CTLTYPE_NODE,
	.sysctl_num = 0,
	.sysctl_size = sizeof(struct sysctlnode),
	.sysctl_name = "(darwin_root)",
};

static int
darwin_sysctl_redispatch(SYSCTLFN_ARGS)
{

	/*
	 * put namelen and name back at the top
	 */
	namelen += (name - oname);
	name = oname;

	/*
	 * call into (via NULL) main native tree
	 */
	return sysctl_dispatch(SYSCTLFN_CALL(NULL));
}

/*
 * this setup routine is a complement to darwin_sys___sysctl()
 */
SYSCTL_SETUP(sysctl_darwin_emul_setup, "darwin emulated sysctl tree setup")
{
	struct sysctlnode *_root = &darwin_sysctl_root;

	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       DARWIN_CTL_KERN, CTL_EOL);

	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "ostype", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_OSTYPE, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "osrelease", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_OSRELEASE, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "osrevision", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_OSREV, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "version", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_VERSION, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "maxvnodes", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_MAXVNODES, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "maxproc", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_MAXPROC, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "maxfiles", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_MAXFILES, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "argmax", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_ARGMAX, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "securelevel", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_SECURELVL, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "hostname", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_HOSTNAME, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "hostid", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_HOSTID, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "clockrate", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_CLOCKRATE, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "vnode", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_VNODE, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "file", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_FILE, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "profiling", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_PROF, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "posix1version", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_POSIX1, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "ngroups", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_NGROUPS, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "job_control", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_JOB_CONTROL, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "saved_ids", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_SAVED_IDS, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "boottime", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_BOOTTIME, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "nisdomainname", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_NISDOMAINNAME, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "maxpartitions", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_MAXPARTITIONS, CTL_EOL);

	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "procargs", NULL,
		       darwin_sysctl_procargs, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_PROCARGS, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "proc", NULL,
		       darwin_sysctl_dokproc, 0, NULL, 0,
		       DARWIN_CTL_KERN, DARWIN_KERN_PROC, CTL_EOL);

	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "hw", NULL,
		       NULL, 0, NULL, 0,
		       DARWIN_CTL_HW, CTL_EOL);

	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "machine", NULL,
		       NULL, 0, darwin_sysctl_hw_machine, 0,
		       DARWIN_CTL_HW, DARWIN_HW_MACHINE, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "ncpu", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_HW, DARWIN_HW_NCPU, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "vectorunit", NULL,
		       NULL, 0, NULL, 0,
		       DARWIN_CTL_HW, DARWIN_HW_VECTORUNIT, CTL_EOL);
	sysctl_createv(clog, 0, &_root, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "pagesize", NULL,
		       darwin_sysctl_redispatch, 0, NULL, 0,
		       DARWIN_CTL_HW, DARWIN_HW_PAGESIZE, CTL_EOL);
}

int
darwin_sys___sysctl(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys___sysctl_args *uap = v;
	int error, nerror, name[CTL_MAXNAME];
	size_t savelen = 0, oldlen = 0;

	/*
	 * get oldlen
	 */
	oldlen = 0;
	if (SCARG(uap, oldlenp) != NULL) {
		error = copyin(SCARG(uap, oldlenp), &oldlen, sizeof(oldlen));
		if (error)
			return (error);
	}
	savelen = oldlen;

	/*
	 * top-level sysctl names may or may not be non-terminal, but
	 * we don't care
	 */
	if (SCARG(uap, namelen) > CTL_MAXNAME || SCARG(uap, namelen) < 1)
		return (EINVAL);
	error = copyin(SCARG(uap, name), &name,
		       SCARG(uap, namelen) * sizeof(int));
	if (error)
		return (error);

	/*
	 * wire old so that copyout() is less likely to fail?
	 */
	error = sysctl_lock(l, SCARG(uap, oldp), savelen);
	if (error)
		return (error);

	/*
	 * dispatch request into darwin sysctl tree
	 */
	error = sysctl_dispatch(&name[0], SCARG(uap, namelen),
				SCARG(uap, oldp), &oldlen,
				SCARG(uap, newp), SCARG(uap, newlen),
				&name[0], l, &darwin_sysctl_root);

	/*
	 * release the sysctl lock
	 */
	sysctl_unlock(l);

	/*
	 * reset caller's oldlen, even if we got an error
	 */
	if (SCARG(uap, oldlenp) != NULL) {
		nerror = copyout(&oldlen, SCARG(uap, oldlenp), sizeof(oldlen));
		if (error == 0)
			error = nerror;
	}

	/*
	 * if the only problem is that we weren't given enough space,
	 * that's an ENOMEM error
	 */
	if (error == 0 && SCARG(uap, oldp) != NULL && savelen < oldlen)
		error = ENOMEM;

	return (error);
}

SYSCTL_SETUP(sysctl_emul_darwin_setup, "sysctl emul.darwin subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "emul", NULL,
		       NULL, 0, NULL, 0,
		       CTL_EMUL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "darwin", NULL,
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_DARWIN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "init", NULL,
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_DARWIN, 
		       EMUL_DARWIN_INIT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ioframebuffer", NULL,
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_DARWIN, 
		       EMUL_DARWIN_IOFRAMEBUFFER, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "iohidsystem", NULL,
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_DARWIN, 
		       EMUL_DARWIN_IOHIDSYSTEM, CTL_EOL);
	/*
	 * XXX - darwin_init_pid is a pid_t, not an int
	 */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "pid", NULL,
		       NULL, 0, &darwin_init_pid, 0,
		       CTL_EMUL, EMUL_DARWIN, EMUL_DARWIN_INIT,
		       EMUL_DARWIN_INIT_PID, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "unit", NULL,
		       NULL, 0, &darwin_ioframebuffer_unit, 0,
		       CTL_EMUL, EMUL_DARWIN, EMUL_DARWIN_IOFRAMEBUFFER,
		       EMUL_DARWIN_IOFRAMEBUFFER_UNIT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "screen", NULL,
		       NULL, 0, &darwin_ioframebuffer_screen, 0,
		       CTL_EMUL, EMUL_DARWIN, EMUL_DARWIN_IOFRAMEBUFFER,
		       EMUL_DARWIN_IOFRAMEBUFFER_SCREEN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "mux", NULL,
		       NULL, 0, &darwin_iohidsystem_mux, 0,
		       CTL_EMUL, EMUL_DARWIN, EMUL_DARWIN_IOHIDSYSTEM,
		       EMUL_DARWIN_IOHIDSYSTEM_MUX, CTL_EOL);
}

/*
 * On Darwin, mach_init is the system bootstrap process. It is responsible
 * for forking the traditional UNIX init(8) and it does the Mach port naming
 * service. We need mach_init for the naming service, but unfortunately, it
 * will only act as such if its PID is 1. We use a sysctl 
 * (emul.darwin.init.pid) to fool a given process into thinking its PID is 1.
 * That way we can run mach_init when we want to. 
 * Typical use:
 * sysctl -w emul.darwin.init.pid=$$; exec /emul/darwin/sbin/mach_init
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
darwin_sysctl_dokproc(SYSCTLFN_ARGS)
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

	if (newp != NULL)
		return (EPERM);

	dp = oldp;
	dp2 = where = oldp;
	buflen = where != NULL ? *oldlenp : 0;
	error = 0;
	needed = 0;
	type = rnode->sysctl_num;

	if (type == DARWIN_KERN_PROC) {
		if (namelen != 2 && 
		    !(namelen == 1 && name[0] == DARWIN_KERN_PROC_ALL))
			return (EINVAL);
		op = name[0];
		if (op != DARWIN_KERN_PROC_ALL)
			arg = name[1];
		else
			arg = 0;		/* Quell compiler warning */
		elem_size = elem_count = 0;	/* Ditto */
	} else {
		if (namelen != 4)
			return (EINVAL);
		op = name[0];
		arg = name[1];
		elem_size = name[2];
		elem_count = name[3];
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
		*oldlenp = (caddr_t)dp - where;
		if (needed > *oldlenp)
			return (ENOMEM);
	} else {
		needed += DARWIN_KERN_PROCSLOP;
		*oldlenp = needed;
	}
	return (0);
 cleanup:
	proclist_unlock_read();
	return (error);
}

/* 
 * Native struct proc to Darwin's struct kinfo_proc 
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
	dep->p_dupfd = curlwp->l_dupfd;	/* XXX */
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
darwin_sysctl_procargs(SYSCTLFN_ARGS)
{
	struct ps_strings pss;
	struct proc *p, *up = l->l_proc;
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

	if (oldp == NULL) {
		*oldlenp = ARG_MAX;	/* XXX XXX XXX */
		return (0);
	}

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
	upper_bound = *oldlenp;
	len = 0;
	while (len < upper_bound) {
		if (len + PAGE_SIZE > upper_bound)
			xlen = len - upper_bound;
		else
			xlen = PAGE_SIZE;

		if ((error = copyout(arg, (char *)oldp + len, xlen)) != 0)
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
	upper_bound = *oldlenp;
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
	len = (((u_long)oldp + len - 5) & ~0x3UL) - (u_long)oldp;
	len = upper_bound - len; 

	/* 
	 * Align to a word boundary, and copy the program name 
	 */
	len = (((u_long)oldp + len - 1) & ~0x3UL) - (u_long)oldp;
	len = len - strlen(p->p_comm);
	if (len < 0)
		len = 0;

	error = copyout(p->p_comm, (char *)oldp + len, strlen(p->p_comm) + 1);
	if (error != 0)
		goto done;

	len = len + strlen(p->p_comm);
	len = (((u_long)oldp + len + 5) & ~0x3UL) - (u_long)oldp;

	/*
	 * Now copy in the actual argument vector, one page at a time,
	 * since we don't know how long the vector is (though, we do
	 * know how many NUL-terminated strings are in the vector).
	 */
	upper_bound = *oldlenp;
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

		if ((error = copyout(arg, (char *)oldp + len, i)) != 0)
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
