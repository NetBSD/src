/* $NetBSD: vfs_syscalls_10.c,v 1.1.2.4 2018/10/15 10:44:27 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_syscalls_10.c,v 1.1.2.4 2018/10/15 10:44:27 pgoyette Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif
 
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/vfs_syscalls.h>
#include <sys/compat_stub.h>

#include <compat/common/compat_mod.h>

static int
real_sys_openat_10(struct pathbuf **pb)
{

	*pb = pathbuf_create(".");
	return (*pb == NULL ? ENOMEM : 0);
}

MODULE_SET_HOOK(compat_10_openat_hook, "openat_10", real_sys_openat_10);
MODULE_UNSET_HOOK(compat_10_openat_hook);

void vfs_syscalls_10_init(void)
{

	compat_10_openat_hook_set();
}

void vfs_syscalls_10_fini(void)
{

	compat_10_openat_hook_unset();
}
