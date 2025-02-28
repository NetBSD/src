/*	$NetBSD: netbsd32_compat_16.c,v 1.9 2025/02/28 18:30:06 christos Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_16.c,v 1.9 2025/02/28 18:30:06 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/module.h>
#include <sys/dirent.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

struct uvm_object *emul_netbsd32_object;

#if  defined(__amd64__) || defined(__arm__) || defined(__mips__) || defined(__sparc64__)
#define __HAVE_MD_NETBSD32_SIGRETURN14
#endif

#if defined(__amd64__) || defined(__arm__) || defined(__mips__) || defined(__powerpc__) || defined(__riscv__)
#define __HAVE_MD_NETBSD32_SIGCODE
#endif


#ifdef __HAVE_MD_NETBSD32_SIGRETURN14
static const struct syscall_package netbsd32_kern_sig_16_syscalls[] = {
        /* compat_16_netbsd32___sigreturn14 is in MD code! */
        { NETBSD32_SYS_compat_16_netbsd32___sigreturn14, 0,
            (sy_call_t *)compat_16_netbsd32___sigreturn14 },
        { 0, 0, NULL }
};
#endif

static int
compat_netbsd32_16_init(void)
{
#if defined(__HAVE_MD_NETBSD32_SIGRETURN14) || defined(__HAVE_MD_NETBSD32_SIGCODE)
	int error;
#endif

#ifdef __HAVE_MD_NETBSD32_SIGRETURN14
	error = syscall_establish(&emul_netbsd32,
	    netbsd32_kern_sig_16_syscalls);
	if (error)
		return error;
#endif

#ifdef __HAVE_MD_NETBSD32_SIGCODE
	rw_enter(&exec_lock, RW_WRITER);
	emul_netbsd32.e_sigcode = netbsd32_sigcode;
	emul_netbsd32.e_esigcode = netbsd32_esigcode;
	emul_netbsd32.e_sigobject = &emul_netbsd32_object;
	error = exec_sigcode_alloc(&emul_netbsd);
	if (error) {
		emul_netbsd32.e_sigcode = NULL;
		emul_netbsd32.e_esigcode = NULL;
		emul_netbsd32.e_sigobject = NULL;
	}
	rw_exit(&exec_lock);
	if (error)
		return error;
	netbsd32_machdep_md_16_init();
#endif
	return 0;
}

static int
compat_netbsd32_16_fini(void)
{
#if defined(__HAVE_MD_NETBSD32_SIGRETURN14)
	proc_t *p;
	int error;
#endif

#ifdef __HAVE_MD_NETBSD32_SIGRETURN14
	error = syscall_disestablish(&emul_netbsd32,
	    netbsd32_kern_sig_16_syscalls);
	if (error)
		return error;
	/*
	 * Ensure sendsig_sigcontext() is not being used.
	 * module_lock prevents the flag being set on any
	 * further processes while we are here.  See
	 * sigaction1() for the opposing half.
	 */
	mutex_enter(&proc_lock);
	PROCLIST_FOREACH(p, &allproc) {
		if ((p->p_lflag & PL_SIGCOMPAT) != 0) {
			break;
		}
	}
	mutex_exit(&proc_lock);
	if (p != NULL) {
		syscall_establish(&emul_netbsd32,
		    netbsd32_kern_sig_16_syscalls);
		return EBUSY;
	}
#endif

#ifdef __HAVE_MD_NETBSD32_SIGCODE
	rw_enter(&exec_lock, RW_WRITER);
	exec_sigcode_free(&emul_netbsd);
	emul_netbsd32.e_sigcode = NULL;
	emul_netbsd32.e_esigcode = NULL;
	emul_netbsd32.e_sigobject = NULL;
	rw_exit(&exec_lock);
	netbsd32_machdep_md_16_fini();
#endif
	return 0;
}

MODULE(MODULE_CLASS_EXEC, compat_netbsd32_16, "compat_netbsd32_20,compat_16");

static int
compat_netbsd32_16_modcmd(modcmd_t cmd, void *arg)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		return compat_netbsd32_16_init();

	case MODULE_CMD_FINI:
		return compat_netbsd32_16_fini();

	default:
		return ENOTTY;
	}
}
