/*	$NetBSD: linux32_exec.h,v 1.2 2021/11/26 08:56:28 ryo Exp $	*/

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

#ifndef _AARCH64_LINUX32_EXEC_H_
#define _AARCH64_LINUX32_EXEC_H_

#include <sys/exec_elf.h>

#define LINUX32_ELF_AUX_ENTRIES	20
#define LINUX32_PLATFORM	"v7l"

/* The extra data (ELF auxiliary table and platform name) on stack */
struct linux32_extra_stack_data {
	Aux32Info ai[LINUX32_ELF_AUX_ENTRIES];
	uint32_t randbytes[4];
	char hw_platform[8];	/* sizeof(LINUX32_PLATFORM) + align */
};

#define LINUX32_ELF_AUX_ARGSIZ	sizeof(struct linux32_extra_stack_data)

#define LINUX32_ARM_KUSER_HELPER_ADDR	0xffff0000
#define LINUX32_ARM_KUSER_HELPER_SIZE	0x00001000
/* avoid 0xffff0000-. linux/arm uses this area as kuser_helper */
#define LINUX32_USRSTACK	((vaddr_t)0xfffef000)
#define LINUX32_MAXSSIZ		MAXSSIZ32	/* from machine/vmparam.h */

int linux32_exec_setup_stack(struct lwp *, struct exec_package *);

#define LINUX32_GO_RT0_SIGNATURE

#endif /* _AARCH64_LINUX32_EXEC_H_ */
