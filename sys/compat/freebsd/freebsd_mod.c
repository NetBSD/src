/*	$NetBSD: freebsd_mod.c,v 1.2.22.1 2014/08/20 00:03:31 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: freebsd_mod.c,v 1.2.22.1 2014/08/20 00:03:31 tls Exp $");

#ifdef _KERNEL_OPT
#include "opt_execfmt.h"
#endif

#include <sys/param.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/exec_aout.h>
#include <sys/signalvar.h>

#include <compat/freebsd/freebsd_exec.h>
#include <compat/freebsd/freebsd_sysctl.h>

#if defined(EXEC_ELF32)
# define	MD1	",exec_elf32"
#else
# define	MD1	""
#endif
#if defined(EXEC_AOUT)
#  define	MD2	",exec_aout"
#else
# define	MD2	""
#endif

#define ELF32_AUXSIZE (howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), \
    sizeof(Elf32_Addr)) + MAXPATHLEN + ALIGN(1))

MODULE(MODULE_CLASS_EXEC, compat_freebsd, "compat,compat_ossaudio" MD1 MD2);

static struct execsw freebsd_execsw[] = {
#ifdef EXEC_ELF32
	/* FreeBSD Elf32 (probe not 64-bit safe) */
	{
		.es_hdrsz = sizeof (Elf32_Ehdr),
		.es_makecmds = exec_elf32_makecmds,
		.u = {
			.elf_probe_func = freebsd_elf32_probe,
		},
		.es_emul = &emul_freebsd,
		.es_prio = EXECSW_PRIO_ANY,
		.es_arglen = ELF32_AUXSIZE,
		.es_copyargs = elf32_copyargs,
		.es_setregs = NULL,
		.es_coredump = coredump_elf32,
		.es_setup_stack = exec_setup_stack,
	},
#endif
#ifdef EXEC_AOUT
	/* FreeBSD a.out (native word size) */
	{
		.es_hdrsz = FREEBSD_AOUT_HDR_SIZE,
		.es_makecmds = exec_freebsd_aout_makecmds,
		.u = {
			.elf_probe_func = NULL,
		},
		.es_emul = &emul_freebsd,
		.es_prio = EXECSW_PRIO_ANY,
		.es_arglen = 0,
		.es_copyargs = copyargs,
		.es_setregs = NULL,
		.es_coredump = coredump_netbsd,
		.es_setup_stack = exec_setup_stack,
	},
#endif
};

static int
compat_freebsd_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		freebsd_sysctl_init();
		error = exec_add(freebsd_execsw,
		    __arraycount(freebsd_execsw));
		if (error != 0)
			freebsd_sysctl_fini();
		return error;

	case MODULE_CMD_FINI:
		error = exec_remove(freebsd_execsw,
		    __arraycount(freebsd_execsw));
		if (error == 0)
			freebsd_sysctl_fini();
		return error;

	default:
		return ENOTTY;
	}
}
