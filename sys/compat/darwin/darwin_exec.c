/*	$NetBSD: darwin_exec.c,v 1.59 2009/08/16 15:35:52 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#include "opt_compat_darwin.h" /* For COMPAT_DARWIN in mach_port.h */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_exec.c,v 1.59 2009/08/16 15:35:52 manu Exp $");

#include "opt_syscall_debug.h"
#include "wsdisplay.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/conf.h>
#include <sys/exec_macho.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_param.h>

#include <dev/wscons/wsconsio.h>

#include <compat/sys/signal.h>

#include <compat/common/compat_util.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_port.h>

#include <compat/darwin/darwin_exec.h>
#include <compat/darwin/darwin_commpage.h>
#include <compat/darwin/darwin_signal.h>
#include <compat/darwin/darwin_syscall.h>
#include <compat/darwin/darwin_sysctl.h>
#include <compat/darwin/darwin_iokit.h>
#include <compat/darwin/darwin_iohidsystem.h>

#include <machine/darwin_machdep.h>

#if defined(NWSDISPLAY) && NWSDISPLAY > 0
/* Redefined from sys/dev/wscons/wsdisplay.c */
extern const struct cdevsw wsdisplay_cdevsw;
#endif

static void darwin_e_proc_exec(struct proc *, struct exec_package *);
static void darwin_e_proc_fork(struct proc *, struct proc *, int);
static void darwin_e_proc_exit(struct proc *);
static void darwin_e_proc_init(struct proc *, struct vmspace *);

extern struct sysent darwin_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const darwin_syscallnames[];
#endif
#ifndef __HAVE_SYSCALL_INTERN
void syscall(void);
#else
void mach_syscall_intern(struct proc *);
#endif

struct emul emul_darwin = {
	"darwin",
	"/emul/darwin",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	0,
	DARWIN_SYS_syscall,
	DARWIN_SYS_NSYSENT,
#endif
	darwin_sysent,
#ifdef SYSCALL_DEBUG
	darwin_syscallnames,
#else
	NULL,
#endif
	darwin_sendsig,
	darwin_trapsignal,
	darwin_tracesig,
	NULL,
	NULL,
	NULL,
	setregs,
	darwin_e_proc_exec,
	darwin_e_proc_fork,
	darwin_e_proc_exit,
	mach_e_lwp_fork,
	mach_e_lwp_exit,
#ifdef __HAVE_SYSCALL_INTERN
	mach_syscall_intern,
#else
	syscall,
#endif
	NULL,
	NULL,

	uvm_default_mapaddr,
	NULL,
	NULL,
	0,
	NULL,
};

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
int
exec_darwin_copyargs(struct lwp *l, struct exec_package *pack, struct ps_strings *arginfo, char **stackp, void *argp)
{
	struct exec_macho_emul_arg *emea;
	struct exec_macho_object_header *macho_hdr;
	struct proc *p = l->l_proc;
	char **cpp, *dp, *sp, *progname;
	size_t len;
	void *nullp = NULL;
	long argc, envc;
	int error;

	/*
	 * Prepare the comm pages
	 */
	if ((error = darwin_commpage_map(p)) != 0)
		return error;

	/*
	 * Set up the stack
	 */
	*stackp = (char *)(((unsigned long)*stackp - 1) & ~0xfUL);

	emea = (struct exec_macho_emul_arg *)pack->ep_emul_arg;

	if (emea->dynamic == 1) {
		macho_hdr = (struct exec_macho_object_header *)emea->macho_hdr;
		error = copyout(&macho_hdr, *stackp, sizeof(macho_hdr));
		if (error != 0)
			return error;
		*stackp += sizeof(macho_hdr);
	}

	cpp = (char **)*stackp;
	argc = arginfo->ps_nargvstr;
	envc = arginfo->ps_nenvstr;
	if ((error = copyout(&argc, cpp++, sizeof(argc))) != 0)
		return error;

	dp = (char *) (cpp + argc + envc + 4);

	if ((error = copyoutstr(emea->filename, dp,
	    (ARG_MAX < MAXPATHLEN) ? ARG_MAX : MAXPATHLEN, &len)) != 0)
		return error;
	progname = dp;
	dp += len;

	sp = argp;
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

	if ((error = copyout(&progname, cpp++, sizeof(progname))) != 0)
		return error;

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0)
		return error;

	*stackp = (char *)cpp;

	/* We don't need this anymore */
	free(pack->ep_emul_arg, M_TEMP);
	pack->ep_emul_arg = NULL;

	return 0;
}

int
exec_darwin_probe(const char **path)
{
	*path = emul_darwin.e_path;
	return 0;
}

static void
darwin_e_proc_exec(struct proc *p, struct exec_package *epp)
{
	struct darwin_emuldata *ded;

	darwin_e_proc_init(p, p->p_vmspace);

	/* Setup the mach_emuldata part of darwin_emuldata */
	mach_e_proc_exec(p, epp);

	ded = (struct darwin_emuldata *)p->p_emuldata;
	if (p->p_pid == darwin_init_pid)
		ded->ded_fakepid = 1;
#ifdef DEBUG_DARWIN
	printf("pid %d exec'd: fakepid = %d\n", p->p_pid, ded->ded_fakepid);
#endif

	return;
}

static void
darwin_e_proc_fork(struct proc *p, struct proc *parent, int forkflags)
{
	struct darwin_emuldata *ded1;
	struct darwin_emuldata *ded2;
	char *ed1, *ed2;
	size_t len;

	p->p_emuldata = NULL;

	/* Use parent's vmspace because our vmspace may not be setup yet */
	darwin_e_proc_init(p, parent->p_vmspace);

	/*
	 * Setup the mach_emuldata part of darwin_emuldata
	 * The null third argument asks to not re-allocate
	 * p->p_emuldata again.
	 */
	mach_e_proc_fork1(p, parent, 0);

	ded1 = p->p_emuldata;
	ded2 = parent->p_emuldata;

	ed1 = (char *)ded1 + sizeof(struct mach_emuldata);;
	ed2 = (char *)ded2 + sizeof(struct mach_emuldata);;
	len = sizeof(struct darwin_emuldata) - sizeof(struct mach_emuldata);

	(void)memcpy(ed1, ed2, len);

	if ((ded2->ded_fakepid == 1) && (darwin_init_pid != 0)) {
		darwin_init_pid = 0;
		ded1->ded_fakepid = 2;
	} else {
		ded1->ded_fakepid = 0;
	}
#ifdef DEBUG_DARWIN
	printf("pid %d fork'd: fakepid = %d\n", p->p_pid, ded1->ded_fakepid);
#endif

	return;
}

static void
darwin_e_proc_init(struct proc *p, struct vmspace *vmspace)
{
	struct darwin_emuldata *ded;

	if (!p->p_emuldata) {
		p->p_emuldata = malloc(sizeof(struct darwin_emuldata),
		    M_EMULDATA, M_WAITOK | M_ZERO);

	}
	ded = (struct darwin_emuldata *)p->p_emuldata;
	ded->ded_fakepid = 0;
	ded->ded_wsdev = NODEV;
	ded->ded_vramoffset = NULL;

	/* Initalize the mach_emuldata part of darwin_emuldata */
	mach_e_proc_init(p, vmspace);

	return;
}

static void
darwin_e_proc_exit(struct proc *p)
{
	struct darwin_emuldata *ded;
	struct lwp *l;

	ded = p->p_emuldata;
	l = LIST_FIRST(&p->p_lwps);
	/*
	 * mach_init is setting the bootstrap port for other processes.
	 * If mach_init dies, we want to restore the original bootstrap
	 * port.
	 */
	if (ded->ded_fakepid == 2)
		mach_bootstrap_port = mach_saved_bootstrap_port;

	/*
	 * Terminate the iohidsystem kernel thread.
	 * We need to post a fake event in case
	 * the thread is sleeping for an event.
	 */
	if (ded->ded_hidsystem_finished != NULL) {
		*ded->ded_hidsystem_finished = 1;
		darwin_iohidsystem_postfake(l);
		wakeup(ded->ded_hidsystem_finished);
	}

#if defined(NWSDISPLAY) && NWSDISPLAY > 0
	/*
	 * Restore text mode and black and white colormap
	 */
	if (ded->ded_wsdev != NODEV) {
		int error, mode;

		mode = WSDISPLAYIO_MODE_EMUL;
		error = (*wsdisplay_cdevsw.d_ioctl)(ded->ded_wsdev,
		    WSDISPLAYIO_SMODE, (void *)&mode, 0, l);
#ifdef DEBUG_DARWIN
		if (error != 0)
			printf("Unable to switch back to text mode\n");
#endif
#if 0	/* Comment out stackgap use - this needs to be done another way */
	    {
		void *sg = stackgap_init(p, 0);
		struct wsdisplay_cmap cmap;
		u_char *red;
		u_char *green;
		u_char *blue;
		u_char kred[256];
		u_char kgreen[256];
		u_char kblue[256];
		red = stackgap_alloc(p, &sg, 256);
		green = stackgap_alloc(p, &sg, 256);
		blue = stackgap_alloc(p, &sg, 256);

		(void)memset(kred, 255, 256);
		(void)memset(kgreen, 255, 256);
		(void)memset(kblue, 255, 256);

		kred[0] = 0;
		kgreen[0] = 0;
		kblue[0] = 0;

		cmap.index = 0;
		cmap.count = 256;
		cmap.red = red;
		cmap.green = green;
		cmap.blue = blue;

		if (((error = copyout(kred, red, 256)) != 0) ||
		    ((error = copyout(kgreen, green, 256)) != 0) ||
		    ((error = copyout(kblue, blue, 256)) != 0))
			error = (*wsdisplay_cdevsw.d_ioctl)(ded->ded_wsdev,
			    WSDISPLAYIO_PUTCMAP, (void *)&cmap, 0, l);
#ifdef DEBUG_DARWIN
		if (error != 0)
			printf("Cannot revert colormap (error %d)\n", error);
#endif
	    }
#endif /* 0 */
	}
#endif /* defined(NWSDISPLAY) && NWSDISPLAY > 0 */

	/*
	 * Cleanup mach_emuldata part of darwin_emuldata
	 * It will also free p->p_emuldata.
	 */
	mach_e_proc_exit(p);

	return;
}

int
darwin_exec_setup_stack(struct lwp *l, struct exec_package *epp)
{
	u_long max_stack_size;
	u_long access_linear_min, access_size;
	u_long noaccess_linear_min, noaccess_size;

	if (epp->ep_flags & EXEC_32) {
		epp->ep_minsaddr = DARWIN_USRSTACK32;
		max_stack_size = MAXSSIZ;
	} else {
		epp->ep_minsaddr = DARWIN_USRSTACK;
		max_stack_size = MAXSSIZ;
	}
	epp->ep_maxsaddr = (u_long)STACK_GROW(epp->ep_minsaddr,
		max_stack_size);
	epp->ep_ssize = l->l_proc->p_rlimit[RLIMIT_STACK].rlim_cur;

	/*
	 * set up commands for stack.  note that this takes *two*, one to
	 * map the part of the stack which we can access, and one to map
	 * the part which we can't.
	 *
	 * arguably, it could be made into one, but that would require the
	 * addition of another mapping proc, which is unnecessary
	 */
	access_size = epp->ep_ssize;
	access_linear_min = (u_long)STACK_ALLOC(epp->ep_minsaddr, access_size);
	noaccess_size = max_stack_size - access_size;
	noaccess_linear_min = (u_long)STACK_ALLOC(STACK_GROW(epp->ep_minsaddr,
	    access_size), noaccess_size);
	if (noaccess_size > 0) {
		NEW_VMCMD2(&epp->ep_vmcmds, vmcmd_map_zero, noaccess_size,
		    noaccess_linear_min, NULL, 0, VM_PROT_NONE, VMCMD_STACK);
	}
	KASSERT(access_size > 0);
	NEW_VMCMD2(&epp->ep_vmcmds, vmcmd_map_zero, access_size,
	    access_linear_min, NULL, 0, VM_PROT_READ | VM_PROT_WRITE,
	    VMCMD_STACK);

	return 0;
}
