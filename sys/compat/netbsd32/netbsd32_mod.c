/*	$NetBSD: netbsd32_mod.c,v 1.1.6.2 2008/11/19 18:36:06 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_mod.c,v 1.1.6.2 2008/11/19 18:36:06 ad Exp $");

#ifdef _KERNEL_OPT
#include "opt_execfmt.h"
#define	MODDEPS	"compat"
#else
#define	MODDEPS	"compat,ksem"
#endif

#ifndef ELFSIZE
#define ELFSIZE ARCH_ELFSIZE
#endif

#include <sys/param.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>

#include <compat/netbsd32/netbsd32_sysctl.h>
#include <compat/netbsd32/netbsd32_exec.h>

#define ELF32_AUXSIZE (howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), \
    sizeof(Elf32_Addr)) + MAXPATHLEN + ALIGN(1))

MODULE(MODULE_CLASS_MISC, compat_netbsd32, MODDEPS);

static struct execsw netbsd32_execsw[] = {
#ifdef EXEC_AOUT
	{ sizeof(struct netbsd32_exec),
	  exec_netbsd32_makecmds,
	  { NULL },
	  &emul_netbsd32,
	  EXECSW_PRIO_FIRST,
	  0,
	  netbsd32_copyargs,
	  NULL,
	  coredump_netbsd32,
	  exec_setup_stack },
#endif
#ifdef EXEC_ELF32
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { netbsd32_elf32_probe },
	  &emul_netbsd32,
	  EXECSW_PRIO_FIRST,
	  ELF32_AUXSIZE,
	  netbsd32_elf32_copyargs,
	  NULL,
	  coredump_elf32,
	  exec_setup_stack },		/* XXX XXX XXX */
#endif
};

static int
compat_netbsd32_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		netbsd32_sysctl_init();
		error = exec_add(netbsd32_execsw,
		    __arraycount(netbsd32_execsw));
		if (error != 0)
			netbsd32_sysctl_fini();
		return error;

	case MODULE_CMD_FINI:
		error = exec_remove(netbsd32_execsw,
		    __arraycount(netbsd32_execsw));
		if (error == 0)
			netbsd32_sysctl_fini();
		return error;

	default:
		return ENOTTY;
	}
}
