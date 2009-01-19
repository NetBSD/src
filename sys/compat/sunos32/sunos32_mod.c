/*	$NetBSD: sunos32_mod.c,v 1.1.8.2 2009/01/19 13:17:40 skrll Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sunos32_mod.c,v 1.1.8.2 2009/01/19 13:17:40 skrll Exp $");

#include <sys/param.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>
#include <sys/signalvar.h>

#include <machine/sunos_machdep.h>

#include <compat/sunos32/sunos32_exec.h>

#include <compat/netbsd32/netbsd32_exec.h>

MODULE(MODULE_CLASS_MISC, compat_sunos, "compat,compat_netbsd32,exec_aout");

static struct execsw sunos_execsw[] = {
	{ SUNOS32_AOUT_HDR_SIZE,
	  exec_sunos32_aout_makecmds,
	  { NULL },
	  &emul_sunos,
	  EXECSW_PRIO_ANY,
	  0,
	  netbsd32_copyargs,
	  NULL,
	  coredump_netbsd,
	  exec_setup_stack },
};

static int
compat_sunos_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return exec_add(sunos_execsw,
		    __arraycount(sunos_execsw));

	case MODULE_CMD_FINI:
		return exec_remove(sunos_execsw,
		    __arraycount(sunos_execsw));

	default:
		return ENOTTY;
	}
}
