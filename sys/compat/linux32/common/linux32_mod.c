/*	$NetBSD: linux32_mod.c,v 1.5.2.1 2014/08/10 06:54:33 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux32_mod.c,v 1.5.2.1 2014/08/10 06:54:33 tls Exp $");

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

#include <compat/linux/common/linux_exec.h>

#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/common/linux32_exec.h>

#if defined(EXEC_ELF32)
# define	MD1	",exec_elf32,compat_netbsd32"
#else
# define	MD1	""
#endif

MODULE(MODULE_CLASS_EXEC, compat_linux32, "compat_linux" MD1);

static struct execsw linux32_execsw[] = {
#if defined(EXEC_ELF32)
	{
		.es_hdrsz = sizeof (Elf32_Ehdr),
		.es_makecmds = exec_elf32_makecmds,
		.u = {
			.elf_probe_func = linux32_elf32_probe, 
		},
		.es_emul = &emul_linux32,
		.es_prio = EXECSW_PRIO_ANY, 
		.es_arglen = LINUX32_ELF_AUX_ARGSIZ,
		.es_copyargs = linux32_elf32_copyargs,
		.es_setregs = NULL,
		.es_coredump = coredump_elf32,
		.es_setup_stack = linux_exec_setup_stack,
	},
#endif
};

static int
compat_linux32_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		linux32_sysctl_init();
		error = exec_add(linux32_execsw,
		    __arraycount(linux32_execsw));
		if (error != 0)
			linux32_sysctl_fini();
		return error;

	case MODULE_CMD_FINI:
		error = exec_remove(linux32_execsw,
		    __arraycount(linux32_execsw));
		if (error == 0)
			linux32_sysctl_fini();
		return error;

	default:
		return ENOTTY;
	}
}
