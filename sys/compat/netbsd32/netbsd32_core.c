/*	$NetBSD: netbsd32_core.c,v 1.18 2020/11/08 07:30:09 rin Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993 The Regents of the University of California.
 * Copyright (c) 1988 University of Utah.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: vm_unix.c 1.1 89/11/07$
 *      @(#)vm_unix.c   8.1 (Berkeley) 6/11/93
 * from: NetBSD: uvm_unix.c,v 1.25 2001/11/10 07:37:01 lukem Exp
 */

/*
 * core_netbsd.c: Support for the historic NetBSD core file format.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_core.c,v 1.18 2020/11/08 07:30:09 rin Exp $");

#include <sys/compat_stub.h>
#include <sys/exec_elf.h>
#include <sys/lwp.h>
#include <sys/module.h>

#ifdef _KERNEL_OPT
#include "opt_execfmt.h"
#endif

#define DEPS "compat_netbsd32,compat_netbsd32_ptrace,coredump"

MODULE(MODULE_CLASS_MISC, compat_netbsd32_coredump, DEPS);

#define	CORENAME(x)	__CONCAT(x,32)
#define	COREINC		<compat/netbsd32/netbsd32.h>

#include "../../kern/core_netbsd.c"

static int
compat_netbsd32_coredump_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		MODULE_HOOK_SET(coredump_netbsd32_hook, real_coredump_netbsd32);
#if defined(EXEC_ELF32) && defined(_LP64)
		MODULE_HOOK_SET(coredump_elf32_hook, real_coredump_elf32);
#endif
		return 0;
	case MODULE_CMD_FINI:
		MODULE_HOOK_UNSET(coredump_netbsd32_hook);
#if defined(EXEC_ELF32) && defined(_LP64)
		MODULE_HOOK_UNSET(coredump_elf32_hook);
#endif
		return 0;
	default:
		return ENOTTY;
	}
}
