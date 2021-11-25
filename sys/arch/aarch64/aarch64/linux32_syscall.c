/*	$NetBSD: linux32_syscall.c,v 1.1 2021/11/25 03:08:04 ryo Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ryo Shimizu.
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
__KERNEL_RCSID(0, "$NetBSD: linux32_syscall.c,v 1.1 2021/11/25 03:08:04 ryo Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_linux32.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <compat/linux32/linux32_syscall.h>
#include <compat/linux32/common/linux32_errno.h>

#define EMULNAME(x)		__CONCAT(linux32_,x)
#define EMULNAMEU(x)		__CONCAT(LINUX32_,x)

/*
 * syscall.c behavior options
 */

/*
 * EABI linux/arm always uses indirect syscall, and code of 'svc #<code>' must
 * be zero. The system call number is specified by the r7 register.
 */
#define NARGREG			7 /* Seven(all) arguments are passed in r0-r6 */
#define SYSCALL_CODE_REG	7 /* syscall number is passed in r7 */
#define SYSCALL_CODE_REG_SVC	0 /* Non-zero is OABI, but not supported */
#define SYSCALL_NO_INDIRECT

/* used when __HAVE_MINIMAL_EMUL is not defined */
#define SYSCALL_EMUL_ERRNO(x)	native_to_linux32_errno[x]

/* don't update x1 register with rval[1] */
#define SYSCALL_NO_RVAL1

#define SYSCALL_CODE_REMAP(code)	linux32_syscall_code_remap(code)

/*
 * see also comments in the bottom of
 * sys/compat/linux32/arch/aarch64/syscalls.master
 */
#define LINUX32_EARM_NR_BASE		0x0f0000
#define LINUX32_SYS_ARMBASE		480

static inline uint32_t
linux32_syscall_code_remap(uint32_t code)
{
	if (code > LINUX32_EARM_NR_BASE)
		code = code - LINUX32_EARM_NR_BASE + LINUX32_SYS_ARMBASE;
	return code;
}

#include "aarch32_syscall.c"
