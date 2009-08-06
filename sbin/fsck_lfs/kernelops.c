/*	$NetBSD: kernelops.c,v 1.1 2009/08/06 00:51:55 pooka Exp $	*/

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
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
#ifndef lint
__RCSID("$NetBSD: kernelops.c,v 1.1 2009/08/06 00:51:55 pooka Exp $");
#endif /* !lint */

/*
 * Select operations vector for kernel communication -- either regular
 * system calls or rump system calls.  The latter are use when the
 * cleaner is run as part of rump_lfs.
 *
 * The selection is controlled by an ifdef for now, since I see no
 * value in making it dynamic.
 */

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/statvfs.h>

#include <fcntl.h>
#include <unistd.h>

#include "kernelops.h"

#ifdef USE_RUMP

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

struct kernelops kops = {
	.ko_open = rump_sys_open,
	.ko_fcntl = rump_sys_fcntl,
	.ko_statvfs = rump_sys_statvfs1,
	.ko_fhopen = rump_sys_fhopen,
	.ko_close = rump_sys_close,

	.ko_pread = rump_sys_pread,
	.ko_pwrite = rump_sys_pwrite,
};

#else

struct kernelops kops = {
	.ko_open = (void *)open, /* XXX: fix rump_syscalls */
	.ko_fcntl = (void *)fcntl,
	.ko_statvfs = statvfs1,
	.ko_fhopen = fhopen,
	.ko_close = close,

	.ko_pread = pread,
	.ko_pwrite = pwrite,
};
#endif
