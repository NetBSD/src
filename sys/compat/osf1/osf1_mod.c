/*	$NetBSD: osf1_mod.c,v 1.2.6.2 2009/01/17 13:28:47 mjf Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: osf1_mod.c,v 1.2.6.2 2009/01/17 13:28:47 mjf Exp $");

#include <sys/param.h>
#include <sys/module.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/exec_ecoff.h>
#include <sys/signalvar.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_exec.h>

MODULE(MODULE_CLASS_MISC, compat_osf1, "compat,exec_ecoff");

static struct execsw osf1_execsw[] = {
	{ ECOFF_HDR_SIZE,
	  exec_ecoff_makecmds,
	  { .ecoff_probe_func = osf1_exec_ecoff_probe },
	  &emul_osf1,
	  EXECSW_PRIO_ANY,
  	  howmany(OSF1_MAX_AUX_ENTRIES * sizeof (struct osf1_auxv) +
	    2 * (MAXPATHLEN + 1), sizeof (char *)), /* exec & loader names */
	  osf1_copyargs,
	  cpu_exec_ecoff_setregs,
	  coredump_netbsd,
	  exec_setup_stack },
};

static int
compat_osf1_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return exec_add(osf1_execsw,
		    __arraycount(osf1_execsw));

	case MODULE_CMD_FINI:
		return exec_remove(osf1_execsw,
		    __arraycount(osf1_execsw));

	default:
		return ENOTTY;
	}
}
