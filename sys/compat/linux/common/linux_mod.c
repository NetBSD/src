/*	$NetBSD: linux_mod.c,v 1.10 2017/12/10 00:43:05 kre Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_mod.c,v 1.10 2017/12/10 00:43:05 kre Exp $");

#ifdef _KERNEL_OPT
#include "opt_execfmt.h"
#endif

#ifndef ELFSIZE
#define ELFSIZE ARCH_ELFSIZE
#endif

#include <sys/param.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/signalvar.h>
#include <sys/sysctl.h>

#include <compat/linux/common/linux_sysctl.h>
#include <compat/linux/common/linux_futex.h>
#include <compat/linux/common/linux_exec.h>

#if defined(EXEC_ELF32) && ELFSIZE == 32
# define	MD1	",exec_elf32"
#else
# define	MD1	""
#endif
#if defined(EXEC_ELF64)
# define	MD2	",exec_elf64"
#else
# define	MD2	""
#endif
#if defined(EXEC_AOUT)
#  define	MD3	",exec_aout"
#else
# define	MD3	""
#endif

MODULE(MODULE_CLASS_EXEC, compat_linux, "compat,compat_ossaudio,sysv_ipc"
	MD1 MD2 MD3);

static struct execsw linux_execsw[] = {
#if defined(EXEC_ELF32) && ELFSIZE == 32
	{
		.es_hdrsz = sizeof (Elf32_Ehdr),
		.es_makecmds = exec_elf32_makecmds,
		.u = {
			.elf_probe_func = linux_elf32_probe,
		},
		.es_emul = &emul_linux,
		.es_prio = EXECSW_PRIO_ANY,
		.es_arglen = LINUX_ELF_AUX_ARGSIZ,
		.es_copyargs = linux_elf32_copyargs,
		.es_setregs = NULL,
		.es_coredump = coredump_elf32,
		.es_setup_stack = linux_exec_setup_stack,
	},
#elif defined(EXEC_ELF64)
	{
		.es_hdrsz = sizeof (Elf64_Ehdr),
		.es_makecmds = exec_elf64_makecmds,
		.u = {
			.elf_probe_func = linux_elf64_probe,
		},
		.es_emul = &emul_linux,
		.es_prio = EXECSW_PRIO_ANY,
		.es_arglen = LINUX_ELF_AUX_ARGSIZ,
		.es_copyargs = linux_elf64_copyargs,
		.es_setregs = NULL,
 		.es_coredump = coredump_elf64,
		.es_setup_stack = linux_exec_setup_stack,
	},
#endif
#ifdef EXEC_AOUT
	{
		.es_hdrsz = LINUX_AOUT_HDR_SIZE,
		.es_makecmds = exec_linux_aout_makecmds,
		.u = {
			.elf_probe_func = NULL,
		},
		.es_emul = &emul_linux,
		.es_prio = EXECSW_PRIO_LAST,
		.es_arglen = LINUX_AOUT_AUX_ARGSIZ,
		.es_copyargs = linux_aout_copyargs,
		.es_setregs = NULL,
		.es_coredump = coredump_netbsd,
		.es_setup_stack = linux_exec_setup_stack,
	},
#endif
};

int linux_enabled = 1;

int
linux_sysctl_enable(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error, val;

	val = *(int *)rnode->sysctl_data;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	if (val == *(int *)rnode->sysctl_data)
		return 0;

	if (val == 1)
		error = exec_add(linux_execsw, __arraycount(linux_execsw));
	else if (val == 0)
		error = exec_remove(linux_execsw, __arraycount(linux_execsw));
	else 
		error = EINVAL;

	if (error)
		return error;

	*(int *)rnode->sysctl_data = val;

	return 0;
}

static int
compat_linux_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		linux_futex_init();
		linux_sysctl_init();
		error = exec_add(linux_execsw, __arraycount(linux_execsw));
		if (error != 0) 	 
			linux_sysctl_fini();
		return error;

	case MODULE_CMD_FINI:
		error = exec_remove(linux_execsw, __arraycount(linux_execsw));
		if (error)
			return error;
		linux_sysctl_fini();
		linux_futex_fini();
		return 0;

	default:
		return ENOTTY;
	}
}
