/*	$NetBSD: netbsd32_mod.c,v 1.23 2020/08/08 19:08:48 christos Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * Copyright (c) 1998, 2000, 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, and by Maxime Villard.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_mod.c,v 1.23 2020/08/08 19:08:48 christos Exp $");

#ifdef _KERNEL_OPT
#include "opt_execfmt.h"
#endif

#ifndef ELFSIZE
#define ELFSIZE ARCH_ELFSIZE
#endif

#include <sys/param.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/module_hook.h>
#include <sys/compat_stub.h>

#include <compat/netbsd32/netbsd32_sysctl.h>
#include <compat/netbsd32/netbsd32_kern_proc.h>
#include <compat/netbsd32/netbsd32_exec.h>

#define ELF32_AUXSIZE (howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), \
    sizeof(Elf32_Addr)) + MAXPATHLEN + ALIGN(1))

struct compat32_80_modctl_hook_t compat32_80_modctl_hook;

# define	DEPS1	"ksem,compat_util"

#if defined(EXEC_ELF32)
# define	DEPS2	",exec_elf32"
#else
# define	DEPS2	""
#endif

MODULE(MODULE_CLASS_EXEC, compat_netbsd32, DEPS1 DEPS2);

static struct execsw netbsd32_execsw[] = {
#ifdef EXEC_AOUT
	{
		.es_hdrsz = sizeof(struct netbsd32_exec),
		.es_makecmds = exec_netbsd32_makecmds,
		.u = {
			.elf_probe_func = NULL,
		},
		.es_emul = &emul_netbsd32,
		.es_prio = EXECSW_PRIO_FIRST,
		.es_arglen = 0,
		.es_copyargs = netbsd32_copyargs,
		.es_setregs = NULL,
		.es_coredump = coredump_netbsd32,
		.es_setup_stack = exec_setup_stack,
	},
#endif
#ifdef EXEC_ELF32
	{
		.es_hdrsz = sizeof (Elf32_Ehdr),
		.es_makecmds = exec_elf32_makecmds,
		.u = {
			.elf_probe_func = netbsd32_elf32_probe,
		},
		.es_emul = &emul_netbsd32,
		.es_prio = EXECSW_PRIO_ANY,
		.es_arglen = ELF32_AUXSIZE,
		.es_copyargs = netbsd32_elf32_copyargs,
		.es_setregs = NULL,
		.es_coredump = coredump_elf32,
		.es_setup_stack = exec_setup_stack,
	},
#endif
};

#if defined(__amd64__)
#include <x86/cpu.h>

/* This code was moved here, from $SRC/arch/amd64/amd64/trap.c */

static int
amd64_oosyscall_handle(struct proc *p, struct trapframe *frame)
{
	int error = EPASSTHROUGH;
#define LCALLSZ	7

	/* Check for the oosyscall lcall instruction. */
	if (p->p_emul == &emul_netbsd32 &&
	    frame->tf_rip < VM_MAXUSER_ADDRESS32 - LCALLSZ && 
	    (error = x86_cpu_is_lcall((void *)frame->tf_rip)) == 0)
	{
		/* Advance past the lcall and save instruction size. */
		frame->tf_rip += LCALLSZ;
		frame->tf_err = LCALLSZ;
		return 0;
	}

	return error;
}
#endif /* defined(__amd64__) */

static int
compat_netbsd32_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = exec_add(netbsd32_execsw,
		    __arraycount(netbsd32_execsw));
		if (error == 0) {
			netbsd32_machdep_md_init();
			netbsd32_kern_proc_32_init();
#if defined(__amd64__)
			MODULE_HOOK_SET(amd64_oosyscall_hook,
			    amd64_oosyscall_handle);
#endif /* defined(__amd64__) */
		}
		return error;

	case MODULE_CMD_FINI:
#if defined(__amd64__)
		MODULE_HOOK_UNSET(amd64_oosyscall_hook);
#endif /* defined(__amd64__) */
		netbsd32_machdep_md_fini();
		netbsd32_sysctl_fini();
		netbsd32_kern_proc_32_fini();

		error = exec_remove(netbsd32_execsw,
		    __arraycount(netbsd32_execsw));
		if (error) {
			netbsd32_kern_proc_32_init();
			netbsd32_machdep_md_init();
#if defined(__amd64__)
			MODULE_HOOK_SET(amd64_oosyscall_hook,
			    amd64_oosyscall_handle);
#endif /* defined(__amd64__) */
		}
		return error;

	default:
		return ENOTTY;
	}
}
