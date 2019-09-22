/*	$NetBSD: netbsd32_compat_90.c,v 1.1 2019/09/22 22:59:38 christos Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_90.c,v 1.1 2019/09/22 22:59:38 christos Exp $");

#include <sys/param.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/filedesc.h>
#include <sys/statvfs.h>
#include <sys/vfs_syscalls.h>
#include <sys/syscallvar.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

static int
netbsd32_copyout_statvfs90(const void *kp, void *up, size_t len)
{
	struct netbsd32_statvfs90 *sbuf_32;
	int error;

	sbuf_32 = kmem_alloc(sizeof(*sbuf_32), KM_SLEEP);
	netbsd32_from_statvfs90(kp, sbuf_32);
	error = copyout(sbuf_32, up, sizeof(*sbuf_32));
	kmem_free(sbuf_32, sizeof(*sbuf_32));

	return error;
}

int
compat_90_netbsd32_getvfsstat(struct lwp *l,
    const struct compat_90_netbsd32_getvfsstat_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_statvfs90p_t) buf;
		syscallarg(netbsd32_size_t) bufsize;
		syscallarg(int) flags;
	} */

	return do_sys_getvfsstat(l, SCARG_P32(uap, buf), SCARG(uap, bufsize),
	    SCARG(uap, flags), netbsd32_copyout_statvfs90,
	    sizeof(struct netbsd32_statvfs90), retval);
}

int
compat_90_netbsd32_statvfs1(struct lwp *l,
    const struct compat_90_netbsd32_statvfs1_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_statvfs90p_t) buf;
		syscallarg(int) flags;
	} */
	struct statvfs *sb;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_pstatvfs(l, SCARG_P32(uap, path), SCARG(uap, flags), sb);
	if (error == 0)
		error = netbsd32_copyout_statvfs90(sb, SCARG_P32(uap, buf),
		    sizeof(struct netbsd32_statvfs90));
	STATVFSBUF_PUT(sb);
	return error;
}

int
compat_90_netbsd32_fstatvfs1(struct lwp *l,
    const struct compat_90_netbsd32_fstatvfs1_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_statvfs90p_t) buf;
		syscallarg(int) flags;
	} */
	struct statvfs *sb;
	int error;

	sb = STATVFSBUF_GET();
	error = do_sys_fstatvfs(l, SCARG(uap, fd), SCARG(uap, flags), sb);
	if (error == 0)
		error = netbsd32_copyout_statvfs90(sb, SCARG_P32(uap, buf), 0);
	STATVFSBUF_PUT(sb);
	return error;
}


int
compat_90_netbsd32_fhstatvfs1(struct lwp *l,
    const struct compat_90_netbsd32_fhstatvfs1_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_pointer_t) fhp;
		syscallarg(netbsd32_size_t) fh_size;
		syscallarg(netbsd32_statvfs90p_t) buf;
		syscallarg(int) flags;
	} */
	struct statvfs *sb;
	int error;

	sb = STATVFSBUF_GET();
	error = do_fhstatvfs(l, SCARG_P32(uap, fhp), SCARG(uap, fh_size), sb,
	    SCARG(uap, flags));

	if (error == 0)
		error = netbsd32_copyout_statvfs90(sb, SCARG_P32(uap, buf), 0);
	STATVFSBUF_PUT(sb);

	return error;
}

static struct syscall_package compat_netbsd32_90_syscalls[] = {
	{ NETBSD32_SYS_compat_90_netbsd32_getvfsstat, 0,
	    (sy_call_t *)compat_90_netbsd32_getvfsstat },
	{ NETBSD32_SYS_compat_90_netbsd32_statvfs1, 0,
	    (sy_call_t *)compat_90_netbsd32_statvfs1 },
	{ NETBSD32_SYS_compat_90_netbsd32_fstatvfs1, 0,
	    (sy_call_t *)compat_90_netbsd32_fstatvfs1 },
	{ NETBSD32_SYS_compat_90_netbsd32_fhstatvfs1, 0,
	    (sy_call_t *)compat_90_netbsd32_fhstatvfs1 },
	{ 0, 0, NULL }
}; 

MODULE(MODULE_CLASS_EXEC, compat_netbsd32_90, "compat_netbsd32,compat_90");

static int
compat_netbsd32_90_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return syscall_establish(&emul_netbsd32,
		    compat_netbsd32_90_syscalls);

	case MODULE_CMD_FINI:
		return syscall_disestablish(&emul_netbsd32,
		    compat_netbsd32_90_syscalls);

	default:
		return ENOTTY;
	}
}
