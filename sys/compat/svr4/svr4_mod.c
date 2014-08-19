/*	$NetBSD: svr4_mod.c,v 1.1.34.1 2014/08/20 00:03:33 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: svr4_mod.c,v 1.1.34.1 2014/08/20 00:03:33 tls Exp $");

#ifndef ELFSIZE
#define ELFSIZE ARCH_ELFSIZE
#endif

#include <sys/param.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_exec.h>

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

MODULE(MODULE_CLASS_EXEC, compat_svr4, "compat" MD1 MD2);

#define ELF32_AUXSIZE (howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), \
    sizeof(Elf32_Addr)) + MAXPATHLEN + ALIGN(1))
#define ELF64_AUXSIZE (howmany(ELF_AUX_ENTRIES * sizeof(Aux64Info), \
    sizeof(Elf64_Addr)) + MAXPATHLEN + ALIGN(1))

static struct execsw svr4_execsw[] = {
#if defined(EXEC_ELF32) && ELFSIZE == 32
	{
		.es_hdrsz = sizeof (Elf32_Ehdr),
		.es_makecmds = exec_elf32_makecmds,
		.u = {
			.elf_probe_func = svr4_elf32_probe,
		},
		.es_emul = &emul_svr4,
		.es_prio = EXECSW_PRIO_LAST,
		.es_arglen = ELF32_AUXSIZE,
		.es_copyargs = elf32_copyargs,
		.es_setregs = NULL,
		.es_coredump = coredump_elf32,
		.es_setup_stack = exec_setup_stack,
	},
#elif defined(EXEC_ELF64)
	{
		.es_hdrsz = sizeof (Elf64_Ehdr),
		.es_makecmds = exec_elf64_makecmds,
		.u = {
			.elf_probe_func = svr4_elf64_probe,
		},
		.es_emul = &emul_svr4,
		.es_prio = EXECSW_PRIO_LAST,
		.es_arglen = ELF64_AUXSIZE,
		.es_copyargs = elf64_copyargs,
		.es_setregs = NULL,
		.es_coredump = coredump_elf64,
		.es_setup_stack = exec_setup_stack,
	},
#endif
};

static int
compat_svr4_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		svr4_md_init();
		error = exec_add(svr4_execsw, __arraycount(svr4_execsw));
		if (error != 0)
			svr4_md_fini();
		return error;

	case MODULE_CMD_FINI:
		error = exec_remove(svr4_execsw, __arraycount(svr4_execsw));
		if (error == 0)
			svr4_md_fini();
		return error;

	default:
		return ENOTTY;
	}
}
