/*	$NetBSD: linux_exec.c,v 1.49 2001/05/06 19:09:52 manu Exp $	*/

/*-
 * Copyright (c) 1994, 1995, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas, Frank van der Linden, Eric Haszlakiewicz and
 * Thor Lancelot Simon.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <sys/mman.h>
#include <sys/syscallargs.h>

#include <machine/cpu.h>
#include <machine/reg.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_exec.h>
#include <compat/linux/common/linux_machdep.h>

#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_syscall.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_errno.h>
#include <compat/linux/common/linux_emuldata.h>

extern struct sysent linux_sysent[];
extern const char * const linux_syscallnames[];
extern char linux_sigcode[], linux_esigcode[];
#ifndef __HAVE_SYSCALL_INTERN
void syscall __P((void));
#endif

static void linux_e_proc_exec __P((struct proc *, struct exec_package *));
static void linux_e_proc_fork __P((struct proc *, struct proc *));
static void linux_e_proc_exit __P((struct proc *));
static void linux_e_proc_init __P((struct proc *, struct vmspace *));

/*
 * Execve(2). Just check the alternate emulation path, and pass it on
 * to the NetBSD execve().
 */
int
linux_sys_execve(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_execve_args /* {
		syscallarg(const char *) path;
		syscallarg(char **) argv;
		syscallarg(char **) envp;
	} */ *uap = v;
	struct sys_execve_args ap;
	caddr_t sg;

	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return sys_execve(p, &ap, retval);
}

/*
 * Emulation switch.
 */
const struct emul emul_linux = {
	"linux",
	"/emul/linux",
#ifndef __HAVE_MINIMAL_EMUL
	EMUL_NO_SIGIO_ON_READ,
	(int*)native_to_linux_errno,
	LINUX_SYS_syscall,
	LINUX_SYS_MAXSYSCALL,
#endif
	linux_sysent,
	linux_syscallnames,
	linux_sendsig,
	linux_sigcode,
	linux_esigcode,
	linux_e_proc_exec,
	linux_e_proc_fork,
	linux_e_proc_exit,
#ifdef __HAVE_SYSCALL_INTERN
	linux_syscall_intern,
#else
	syscall,
#endif
};

static void
linux_e_proc_init(p, vmspace)
	struct proc *p;
	struct vmspace *vmspace;
{
	if (!p->p_emuldata) {
		/* allocate new Linux emuldata */
		MALLOC(p->p_emuldata, void *, sizeof(struct linux_emuldata),
			M_EMULDATA, M_WAITOK);
	}

	memset(p->p_emuldata, '\0', sizeof(struct linux_emuldata));
	
	/* Set the process idea of the break to the real value */
	((struct linux_emuldata*)(p->p_emuldata))->p_break = 
	    vmspace->vm_daddr + ctob(vmspace->vm_dsize);
}

/*
 * Allocate per-process structures. Called when executing Linux
 * process. We can reuse the old emuldata - if it's not null,
 * the executed process is of same emulation as original forked one.
 */
static void
linux_e_proc_exec(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	/* exec, use our vmspace */
	linux_e_proc_init(p, p->p_vmspace);
}

/*
 * Emulation per-process exit hook.
 */
static void
linux_e_proc_exit(p)
	struct proc *p;
{
	/* free Linux emuldata and set the pointer to null */
	FREE(p->p_emuldata, M_EMULDATA);
	p->p_emuldata = NULL;
}

/*
 * Emulation fork hook.
 */
static void
linux_e_proc_fork(p, parent)
	struct proc *p, *parent;
{
	/*
	 * It could be desirable to copy some stuff from parent's
	 * emuldata. We don't need anything like that for now.
	 * So just allocate new emuldata for the new process.
	 */
	p->p_emuldata = NULL;

	/* fork, use parent's vmspace (our vmspace may not be setup yet) */
	linux_e_proc_init(p, parent->p_vmspace);
}
