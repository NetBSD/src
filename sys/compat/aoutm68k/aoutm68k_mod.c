/*	$NetBSD: aoutm68k_mod.c,v 1.1.34.1 2014/08/20 00:03:31 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: aoutm68k_mod.c,v 1.1.34.1 2014/08/20 00:03:31 tls Exp $");

#include <sys/param.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>
#include <sys/signalvar.h>

extern struct emul emul_netbsd_aoutm68k;

MODULE(MODULE_CLASS_EXEC, compat_aoutm68k, "exec_aout");

static struct execsw aoutm68k_execsw = {
	.es_hdrsz = sizeof(struct exec),
	.es_makecmds = exec_aout_makecmds,
	.u = {
		.elf_probe_func = NULL,
	},
	.es_emul = &emul_netbsd_aoutm68k,
	.es_prio = EXECSW_PRIO_FIRST,		/* before exec_aout */
	.es_arglen = 0,
	.es_copyargs = copyargs,
	.es_setregs = NULL,
	.es_coredump = coredump_netbsd,
	.es_setup_stack = exec_setup_stack,
};

static int
compat_aoutm68k_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return exec_add(&aoutm68k_execsw, 1);

	case MODULE_CMD_FINI:
		return exec_remove(&aoutm68k_execsw, 1);

	default:
		return ENOTTY;
	}
}
