/*	$NetBSD: irix_mod.c,v 1.2.8.2 2009/01/19 13:17:22 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: irix_mod.c,v 1.2.8.2 2009/01/19 13:17:22 skrll Exp $");

#ifndef ELFSIZE
#define ELFSIZE ARCH_ELFSIZE
#endif

#include <sys/param.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <compat/irix/irix_sysctl.h>
#include <compat/irix/irix_exec.h>

#if defined(EXEC_ELF32)
# define	MD1	",exec_elf32"
#else
# define	MD1	""
#endif

MODULE(MODULE_CLASS_MISC, compat_irix, "compat,compat_svr4" MD1);

static struct execsw irix_execsw[] = {
#if defined(EXEC_ELF32)
	/* IRIX Elf32 n32 ABI */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { irix_elf32_probe_n32 },
	  &emul_irix,
	  EXECSW_PRIO_ANY,
	  IRIX_AUX_ARGSIZ,
	  irix_elf32_copyargs,
	  irix_n32_setregs,
	  coredump_elf32,
	  exec_setup_stack },
	/* IRIX Elf32 o32 ABI */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { irix_elf32_probe_o32 },
	  &emul_irix,
	  EXECSW_PRIO_ANY,
	  IRIX_AUX_ARGSIZ,
	  irix_elf32_copyargs,
	  NULL,
	  coredump_elf32,
	  exec_setup_stack },
#endif
};

static int
compat_irix_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		irix_sysctl_init();
		error = exec_add(irix_execsw, __arraycount(irix_execsw));
		if (error != 0)
			irix_sysctl_fini();
		return error;

	case MODULE_CMD_FINI:
		error = exec_remove(irix_execsw, __arraycount(irix_execsw));
		if (error == 0)
			irix_sysctl_fini();
		return error;

	default:
		return ENOTTY;
	}
}
