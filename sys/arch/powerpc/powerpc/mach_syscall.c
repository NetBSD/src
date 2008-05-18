/*	$NetBSD: mach_syscall.c,v 1.8.76.1 2008/05/18 12:32:38 yamt Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#include "opt_compat_mach.h"
#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: mach_syscall.c,v 1.8.76.1 2008/05/18 12:32:38 yamt Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/syscall.h>

#include <compat/mach/mach_syscall.h> 
#include <compat/mach/arch/powerpc/ppccalls/mach_ppccalls_syscall.h>
#include <compat/mach/arch/powerpc/fasttraps/mach_fasttraps_syscall.h>

extern const struct sysent mach_sysent[];
extern const struct sysent mach_ppccalls_sysent[];
extern const struct sysent mach_fasttraps_sysent[];

#define EMULNAME(x)	__CONCAT(mach_,x)
#define EMULNAMEU(x)	(x)	/* COMPAT_MACH uses the native syscalls */

#define MACH_FASTTRAPS		0x00007ff0
#define MACH_PPCCALLS		0x00006000
#define MACH_ODD_SYSCALL_MASK	0x0000fff0

static inline const struct sysent *mach_syscall_dispatch(register_t *);

#include "syscall.c"

static inline const struct sysent *
mach_syscall_dispatch(register_t *code_p)
{
	const struct sysent *callp = NULL;
	register_t code = *code_p;

	switch (code & MACH_ODD_SYSCALL_MASK) {
	case MACH_PPCCALLS:
		code = (code - MACH_PPCCALLS) & (MACH_PPCCALLS_SYS_NSYSENT-1);
		callp = mach_ppccalls_sysent;
		break;

	case MACH_FASTTRAPS:
		code = (code - MACH_FASTTRAPS) & (MACH_FASTTRAPS_SYS_NSYSENT-1);
		callp = mach_fasttraps_sysent;
		break;

	default:
		if (code < 0) {
			code = -code & (MACH_SYS_NSYSENT-1);
			callp = mach_sysent;
		}
		break;
	}

	if (callp != NULL) {
		*code_p = code;
		callp += code;
	}

	return callp;
}
