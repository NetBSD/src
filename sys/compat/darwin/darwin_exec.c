/*	$NetBSD: darwin_exec.c,v 1.19 2003/09/10 16:44:56 christos Exp $ */

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

#include "opt_compat_darwin.h" /* For COMPAT_DARWIN in mach_port.h */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_exec.c,v 1.19 2003/09/10 16:44:56 christos Exp $");

#include "opt_syscall_debug.h"

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

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_exec.h>
#include <compat/mach/mach_port.h>

#include <compat/darwin/darwin_exec.h>
#include <compat/darwin/darwin_signal.h>
#include <compat/darwin/darwin_syscall.h>
#include <compat/darwin/darwin_sysctl.h>

/* Redefined from sys/dev/wscons/wsdisplay.c */
extern const struct cdevsw wsdisplay_cdevsw;

static void darwin_e_proc_exec(struct proc *, struct exec_package *);
static void darwin_e_proc_fork(struct proc *, struct proc *);
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

#if !defined(__HAVE_SIGINFO) || defined(COMPAT_16)
extern char sigcode[], esigcode[];
struct uvm_object *emul_darwin_object;
#endif

const struct emul emul_darwin = {
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
	trapsignal,
#if !defined(__HAVE_SIGINFO) || defined(COMPAT_16)
	sigcode,
	esigcode,
	&emul_darwin_object,
#else
	NULL,
	NULL,
	NULL,
#endif
	setregs,
	darwin_e_proc_exec,
	darwin_e_proc_fork,
	darwin_e_proc_exit,
#ifdef __HAVE_SYSCALL_INTERN
	mach_syscall_intern,
#else
	syscall,
#endif
	darwin_sysctl,
	NULL,
};

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
int
exec_darwin_copyargs(p, pack, arginfo, stackp, argp)
	struct proc *p;
	struct exec_package *pack;
	struct ps_strings *arginfo;
	char **stackp;
	void *argp;
{
	struct exec_macho_emul_arg *emea;
	struct exec_macho_object_header *macho_hdr;
	char **cpp, *dp, *sp, *progname;
	size_t len;
	void *nullp = NULL;
	long argc, envc;
	int error;

	*stackp = (char *)(((unsigned long)*stackp - 1) & ~0xfUL);

	emea = (struct exec_macho_emul_arg *)pack->ep_emul_arg;
	macho_hdr = (struct exec_macho_object_header *)emea->macho_hdr;
	if ((error = copyout(&macho_hdr, *stackp, sizeof(macho_hdr))) != 0)
		return error;
	*stackp += sizeof(macho_hdr);

	cpp = (char **)*stackp;
	argc = arginfo->ps_nargvstr;
	envc = arginfo->ps_nenvstr;
	if ((error = copyout(&argc, cpp++, sizeof(argc))) != 0)
		return error;

	dp = (char *) (cpp + argc + envc + 4);

	if ((error = copyoutstr(emea->filename, dp, ARG_MAX, &len)) != 0)
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
	free(pack->ep_emul_arg, M_EXEC);
	pack->ep_emul_arg = NULL;

	return 0;
}

int
exec_darwin_probe(path)
	char **path;
{
	*path = (char *)emul_darwin.e_path;
	return 0;
}

static void 
darwin_e_proc_exec(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct darwin_emuldata *ded;

	darwin_e_proc_init(p, p->p_vmspace);

	ded = (struct darwin_emuldata *)p->p_emuldata;
	if (p->p_pid == darwin_init_pid)
		ded->ded_fakepid = 1;

#ifdef DEBUG_DARWIN
	printf("pid %d exec'd: fakepid = %d\n", p->p_pid, ded->ded_fakepid);
#endif
	return;
}

static void 
darwin_e_proc_fork(p, parent)
	struct proc *p;
	struct proc *parent;
{
	struct darwin_emuldata *ded1;
	struct darwin_emuldata *ded2;
	char *ed1, *ed2;
	size_t len;

	p->p_emuldata = NULL;

	/* Use parent's vmspace because our vmspace may not be setup yet */
	darwin_e_proc_init(p, parent->p_vmspace);

	ded1 = p->p_emuldata;
	ded2 = parent->p_emuldata;

	ed1 = (char *)((u_long)ded1 + sizeof(struct mach_emuldata));
	ed2 = (char *)((u_long)ded2 + sizeof(struct mach_emuldata));
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
darwin_e_proc_init(p, vmspace)
	struct proc *p;
	struct vmspace *vmspace;
{
	struct darwin_emuldata *ded;

	if (!p->p_emuldata) {
		p->p_emuldata = malloc(sizeof(struct darwin_emuldata),
		    M_EMULDATA, M_WAITOK | M_ZERO);

	}
	ded = (struct darwin_emuldata *)p->p_emuldata;
	ded->ded_fakepid = 0;
	ded->ded_wsdev = NODEV;

	mach_e_proc_init(p, vmspace);

	return;
}

static void 
darwin_e_proc_exit(p)
	struct proc *p;
{
	struct darwin_emuldata *ded;
	int error, mode;

	ded = p->p_emuldata;

	/*
	 * mach_init is setting the bootstrap port for other processes.
	 * If mach_init dies, we want to restore the original bootstrap 
	 * port.
	 */
	if (ded->ded_fakepid == 2)
		mach_bootstrap_port = mach_saved_bootstrap_port;

	if (ded->ded_wsdev != NODEV) {
		mode = WSDISPLAYIO_MODE_EMUL;
		error = (*wsdisplay_cdevsw.d_ioctl)(ded->ded_wsdev,
		    WSDISPLAYIO_SMODE, (caddr_t)&mode, 0, p);
#ifdef DEBUG_DARWIN
		if (error != 0)
			printf("Unable to switch back to text mode\n");
#endif
	}
		
	mach_e_proc_exit(p);

	return;
}
