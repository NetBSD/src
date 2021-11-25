/*	$NetBSD: linux32_missing.h,v 1.1 2021/11/25 03:08:04 ryo Exp $	*/

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

#ifndef _AARCH64_LINUX32_MISSING_H_
#define _AARCH64_LINUX32_MISSING_H_

#include <compat/netbsd32/netbsd32.h>
#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_machdep.h>

/*
 * system calls not defined for COMPAT_LINUX/aarch64
 * but needed for COMPAT_LINUX32
 */
struct linux_sys_old_mmap_args {
	syscallarg(struct linux_oldmmap *) lmp;
};
int linux_sys_old_mmap(struct lwp *, const struct linux_sys_old_mmap_args *, register_t *);

struct linux_sys_getgroups16_args {
	syscallarg(int) gidsetsize;
	syscallarg(linux_gid16_t *) gidset;
};
struct linux_sys_setgroups16_args {
	syscallarg(int) gidsetsize;
	syscallarg(linux_gid16_t *) gidset;
};
int linux_sys_getgroups16(struct lwp *, const struct linux_sys_getgroups16_args *, register_t *);
int linux_sys_setgroups16(struct lwp *, const struct linux_sys_setgroups16_args *, register_t *);

#endif /* _AARCH64_LINUX32_MISSING_H_ */
