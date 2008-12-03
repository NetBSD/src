/*	$NetBSD: linux32_mod.c,v 1.2 2008/12/03 12:51:11 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: linux32_mod.c,v 1.2 2008/12/03 12:51:11 ad Exp $");

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

#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/common/linux32_exec.h>

#if defined(EXEC_ELF32)
# define	MD1	",exec_elf32,compat_netbsd32"
#else
# define	MD1	""
#endif

MODULE(MODULE_CLASS_MISC, compat_linux32, "compat_linux" MD1);

static struct execsw linux32_execsw[] = {
#if defined(EXEC_ELF32)
        { sizeof (Elf32_Ehdr),
          exec_elf32_makecmds,
          { linux32_elf32_probe },
          &emul_linux32,
          EXECSW_PRIO_FIRST, 
	  LINUX32_ELF_AUX_ARGSIZ,
          linux32_elf32_copyargs,
          NULL,
          coredump_elf32,
          exec_setup_stack },
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
