/*	$NetBSD: linux_mod.c,v 1.3 2013/09/19 18:50:35 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux_mod.c,v 1.3 2013/09/19 18:50:35 christos Exp $");

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

MODULE(MODULE_CLASS_EXEC, compat_linux, "compat,compat_ossaudio" MD1 MD2 MD3);

static struct execsw linux_execsw[] = {
#if defined(EXEC_ELF32) && ELFSIZE == 32
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { linux_elf32_probe },
	  &emul_linux,
	  EXECSW_PRIO_ANY,
	  LINUX_ELF_AUX_ARGSIZ,
	  linux_elf32_copyargs,
	  NULL,
	  coredump_elf32,
	  linux_exec_setup_stack },
#elif defined(EXEC_ELF64)
	{ sizeof (Elf64_Ehdr),
	  exec_elf64_makecmds,
	  { linux_elf64_probe },
	  &emul_linux,
	  EXECSW_PRIO_ANY,
	  LINUX_ELF_AUX_ARGSIZ,
	  linux_elf64_copyargs,
	  NULL,
 	  coredump_elf64,
	  linux_exec_setup_stack },
#endif
#ifdef EXEC_AOUT
	{ LINUX_AOUT_HDR_SIZE,
	  exec_linux_aout_makecmds,
	  { NULL },
	  &emul_linux,
	  EXECSW_PRIO_LAST,
	  LINUX_AOUT_AUX_ARGSIZ,
	  linux_aout_copyargs,
	  NULL,
	  coredump_netbsd,
	  linux_exec_setup_stack },
#endif
};

static int
compat_linux_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		linux_futex_init();
		linux_sysctl_init();
		error = exec_add(linux_execsw,
		    __arraycount(linux_execsw));
		if (error != 0)
			linux_sysctl_fini();
		return error;

	case MODULE_CMD_FINI:
		error = exec_remove(linux_execsw,
		    __arraycount(linux_execsw));
		if (error == 0) {
			linux_sysctl_fini();
			linux_futex_fini();
		}
		return error;

	default:
		return ENOTTY;
	}
}
