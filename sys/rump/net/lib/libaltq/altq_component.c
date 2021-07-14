/*	$NetBSD: altq_component.c,v 1.1 2021/07/14 03:19:24 ozaki-r Exp $	*/

/*
 * Copyright (c) 2019 Internet Initiative Japan Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: altq_component.c,v 1.1 2021/07/14 03:19:24 ozaki-r Exp $");

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/filedesc.h>

#include <sys/vfs_syscalls.h>

#include <net/if.h>

#include <altq/altq.h>

#include <rump-sys/kern.h>
#include <rump-sys/vfs.h>

static void
create_altq_devs(void)
{
	extern const struct cdevsw altq_cdevsw;
	devmajor_t bmajor = NODEVMAJOR, cmajor = NODEVMAJOR;
	int error;

	error = devsw_attach("altq", NULL, &bmajor,
	    &altq_cdevsw, &cmajor);
	if (error != 0)
		panic("altq devsw attach failed: %d", error);

	do_sys_mkdir("/dev/altq", 0755, UIO_SYSSPACE);

	error = rump_vfs_makeonedevnode(S_IFCHR, "/dev/altq/altq", cmajor, 0);
	if (error != 0)
		panic("cannot create altq device node: %d", error);

	error = rump_vfs_makeonedevnode(S_IFCHR, "/dev/altq/cbq", cmajor, ALTQT_CBQ);
	if (error != 0)
		panic("cannot create altq/cbq device node: %d", error);
}

RUMP_COMPONENT(RUMP_COMPONENT_NET_IF)
{

	create_altq_devs();
}
