/*	$NetBSD: compat_60_mod.c,v 1.1.2.14 2018/03/26 10:49:45 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Paul Goyette
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

/*
 * Linkage for the compat module: spaghetti.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_60_mod.c,v 1.1.2.14 2018/03/26 10:49:45 pgoyette Exp $");

#include <sys/systm.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_mod.h>

#include <compat/sys/ccdvar.h>
#include <compat/sys/cpuio.h>

int
compat_60_init(void)
{
	int error = 0;

	error = kern_time_60_init();
	if (error != 0)
		return error;

	error = kern_sa_60_init();
	if (error != 0) {
		kern_time_60_fini();
		return 0;
	}

	kern_tty_60_init();
	ccd_60_init();
#ifdef CPU_UCODE
	kern_cpu_60_init();
#endif

	return error;
}

int
compat_60_fini(void)
{
	int error = 0;

#ifdef NOTYET
#ifdef CPU_UCODE
	kern_cpu_60_fini();
#endif
#endif
	ccd_60_fini();
	kern_tty_60_fini();


	error = kern_sa_60_fini();
	if (error != 0) {
		kern_tty_60_init();
		ccd_60_init();
#ifdef NOTYET
#ifdef CPU_UCODE
		kern_cpu_60_init();
#endif
#endif
		return error;
	}

	error = kern_time_60_fini();
	if (error != 0) {
		kern_sa_60_init();
		kern_tty_60_init();
		ccd_60_init();
#ifdef NOTYET
#ifdef CPU_UCODE
		kern_cpu_60_init();
#endif
#endif
		return error;
	}

	return error;
}

#ifdef _MODULE

#define REQUIRED_60 "compat_70,compat_80"
MODULE(MODULE_CLASS_EXEC, compat_60, REQUIRED_60);

static int
compat_60_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return compat_60_init();
	case MODULE_CMD_FINI:
		return compat_60_init();
	default:
		return ENOTTY;
	}
}
#endif
