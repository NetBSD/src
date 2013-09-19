/*	$NetBSD: ibcs2_mod.c,v 1.2 2013/09/19 18:50:35 christos Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ibcs2_mod.c,v 1.2 2013/09/19 18:50:35 christos Exp $");

#ifdef _KERNEL_OPT
#include "opt_execfmt.h"
#endif

#include <sys/param.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/exec_coff.h>
#include <sys/signalvar.h>

#include <machine/ibcs2_machdep.h>

#include <compat/ibcs2/ibcs2_exec.h>
#include <compat/ibcs2/ibcs2_exec.h>

#if defined(EXEC_ELF32)
# define	MD1	",exec_elf32"
#else
# define	MD1	""
#endif

MODULE(MODULE_CLASS_EXEC, compat_ibcs2, "compat" MD1);

#define ELF32_AUXSIZE (howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), \
    sizeof(Elf32_Addr)) + MAXPATHLEN + ALIGN(1))
#define ELF64_AUXSIZE (howmany(ELF_AUX_ENTRIES * sizeof(Aux64Info), \
    sizeof(Elf64_Addr)) + MAXPATHLEN + ALIGN(1))

static struct execsw ibcs2_execsw[] = {
#ifdef EXEC_ELF32
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { ibcs2_elf32_probe },
	  &emul_ibcs2,
	  EXECSW_PRIO_ANY,
	  ELF32_AUXSIZE,
	  elf32_copyargs,
	  NULL,
	  coredump_elf32,
	  exec_setup_stack },
#endif
	{ COFF_HDR_SIZE,
	  exec_ibcs2_coff_makecmds,
	  { NULL },
	  &emul_ibcs2,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
	  coredump_netbsd,
	  ibcs2_exec_setup_stack },

	{ XOUT_HDR_SIZE,
	  exec_ibcs2_xout_makecmds,
	  { NULL },
	  &emul_ibcs2,
	  EXECSW_PRIO_ANY,
	  0,
	  copyargs,
	  NULL,
	  coredump_netbsd,
	  ibcs2_exec_setup_stack },

};

static int
compat_ibcs2_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return exec_add(ibcs2_execsw, __arraycount(ibcs2_execsw));

	case MODULE_CMD_FINI:
		return exec_remove(ibcs2_execsw, __arraycount(ibcs2_execsw));

	default:
		return ENOTTY;
	}
}
