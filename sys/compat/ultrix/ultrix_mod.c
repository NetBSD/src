/*	$NetBSD: ultrix_mod.c,v 1.3.34.1 2014/08/20 00:03:34 tls Exp $	*/

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

#ifdef _KERNEL_OPT
#include "opt_execfmt.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ultrix_mod.c,v 1.3.34.1 2014/08/20 00:03:34 tls Exp $");

#include <sys/param.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/signalvar.h>
#include <sys/exec_elf.h>
#ifdef EXEC_ECOFF
#include <sys/exec_ecoff.h>
#endif

#include <compat/ultrix/ultrix_exec.h>

#ifdef EXEC_ECOFF
#define	MD1	",exec_ecoff"
#else
#define	MD1	""
#endif

MODULE(MODULE_CLASS_EXEC, compat_ultrix, "compat" MD1);

static struct execsw ultrix_execsw[] = {
#ifdef EXEC_ECOFF
	{
		.es_hdrsz = ECOFF_HDR_SIZE,
		.es_makecmds = exec_ecoff_makecmds,
		.u = {
			.ecoff_probe_func = ultrix_exec_ecoff_probe,
		},
		.es_emul = &emul_ultrix,
		.es_prio = EXECSW_PRIO_LAST,
		.es_arglen = 0,
		.es_copyargs = copyargs,
		.es_setregs = cpu_exec_ecoff_setregs,
		.es_coredump = coredump_netbsd,
		.es_setup_stack = exec_setup_stack,
	},
#endif
};

static int
compat_ultrix_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return exec_add(ultrix_execsw,
		    __arraycount(ultrix_execsw));

	case MODULE_CMD_FINI:
		return exec_remove(ultrix_execsw,
		    __arraycount(ultrix_execsw));

	default:
		return ENOTTY;
	}
}
