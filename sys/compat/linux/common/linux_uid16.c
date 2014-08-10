/*	$NetBSD: linux_uid16.c,v 1.3.58.1 2014/08/10 06:54:33 tls Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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
__KERNEL_RCSID(0, "$NetBSD: linux_uid16.c,v 1.3.58.1 2014/08/10 06:54:33 tls Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/linux_syscallargs.h>

#define LINUXTOBSD_UID(u) \
	(((u) == (linux_uid16_t)-1) ? -1 : (u))
#define LINUXTOBSD_GID(g) \
	(((g) == (linux_gid16_t)-1) ? -1 : (g))

#define BSDTOLINUX_UID(u) \
	(((u) & ~0xffff) ? (linux_uid16_t)65534 : (linux_uid16_t)(u))
#define BSDTOLINUX_GID(g) \
	(((g) & ~0xffff) ? (linux_gid16_t)65534 : (linux_gid16_t)(g))

#ifndef COMPAT_LINUX32
int
linux_sys_chown16(struct lwp *l, const struct linux_sys_chown16_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(linux_uid16_t) uid;
		syscallarg(linux_gid16_t) gid;
	} */
	struct sys___posix_chown_args bca;

	SCARG(&bca, path) = SCARG(uap, path);
	SCARG(&bca, uid) = LINUXTOBSD_UID(SCARG(uap, uid));
	SCARG(&bca, gid) = LINUXTOBSD_GID(SCARG(uap, gid));

	return sys___posix_chown(l, &bca, retval);
}

int
linux_sys_fchown16(struct lwp *l, const struct linux_sys_fchown16_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(linux_uid16_t) uid;
		syscallarg(linux_gid16_t) gid;
	} */
	struct sys___posix_fchown_args bfa;

	SCARG(&bfa, fd) = SCARG(uap, fd);
	SCARG(&bfa, uid) = LINUXTOBSD_UID(SCARG(uap, uid));
	SCARG(&bfa, gid) = LINUXTOBSD_GID(SCARG(uap, gid));

	return sys___posix_fchown(l, &bfa, retval);
}

int
linux_sys_lchown16(struct lwp *l, const struct linux_sys_lchown16_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) path;
		syscallarg(linux_uid16_t) uid;
		syscallarg(linux_gid16_t) gid;
	} */
	struct sys___posix_lchown_args bla;

	SCARG(&bla, path) = SCARG(uap, path);
	SCARG(&bla, uid) = LINUXTOBSD_UID(SCARG(uap, uid));
	SCARG(&bla, gid) = LINUXTOBSD_GID(SCARG(uap, gid));

	return sys___posix_lchown(l, &bla, retval);
}

int
linux_sys_setreuid16(struct lwp *l, const struct linux_sys_setreuid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_uid16_t) ruid;
		syscallarg(linux_uid16_t) euid;
	} */
	struct sys_setreuid_args bsa;

	SCARG(&bsa, ruid) = LINUXTOBSD_UID(SCARG(uap, ruid));
	SCARG(&bsa, euid) = LINUXTOBSD_UID(SCARG(uap, euid));

	return sys_setreuid(l, &bsa, retval);
}

int
linux_sys_setregid16(struct lwp *l, const struct linux_sys_setregid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_gid16_t) rgid;
		syscallarg(linux_gid16_t) egid;
	} */
	struct sys_setregid_args bsa;

	SCARG(&bsa, rgid) = LINUXTOBSD_GID(SCARG(uap, rgid));
	SCARG(&bsa, egid) = LINUXTOBSD_GID(SCARG(uap, egid));

	return sys_setregid(l, &bsa, retval);
}

int
linux_sys_setresuid16(struct lwp *l, const struct linux_sys_setresuid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_uid16_t) ruid;
		syscallarg(linux_uid16_t) euid;
		syscallarg(linux_uid16_t) suid;
	} */
	struct linux_sys_setresuid_args lsa;

	SCARG(&lsa, ruid) = LINUXTOBSD_UID(SCARG(uap, ruid));
	SCARG(&lsa, euid) = LINUXTOBSD_UID(SCARG(uap, euid));
	SCARG(&lsa, suid) = LINUXTOBSD_UID(SCARG(uap, suid));

	return linux_sys_setresuid(l, &lsa, retval);
}

int
linux_sys_setresgid16(struct lwp *l, const struct linux_sys_setresgid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_gid16_t) rgid;
		syscallarg(linux_gid16_t) egid;
		syscallarg(linux_gid16_t) sgid;
	} */
	struct linux_sys_setresgid_args lsa;

	SCARG(&lsa, rgid) = LINUXTOBSD_GID(SCARG(uap, rgid));
	SCARG(&lsa, egid) = LINUXTOBSD_GID(SCARG(uap, egid));
	SCARG(&lsa, sgid) = LINUXTOBSD_GID(SCARG(uap, sgid));

	return linux_sys_setresgid(l, &lsa, retval);
}

int
linux_sys_getresuid16(struct lwp *l, const struct linux_sys_getresuid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_uid16_t *) ruid;
		syscallarg(linux_uid16_t *) euid;
		syscallarg(linux_uid16_t *) suid;
	} */
	kauth_cred_t pc = l->l_cred;
	int error;
	uid_t buid;
	linux_uid16_t luid;

	buid = kauth_cred_getuid(pc);
	luid = BSDTOLINUX_UID(buid);
	if ((error = copyout(&luid, SCARG(uap, ruid), sizeof(luid))) != 0)
		return error;

	buid = kauth_cred_geteuid(pc);
	luid = BSDTOLINUX_UID(buid);
	if ((error = copyout(&luid, SCARG(uap, euid), sizeof(luid))) != 0)
		return error;

	buid = kauth_cred_getsvuid(pc);
	luid = BSDTOLINUX_UID(buid);
	return (copyout(&luid, SCARG(uap, suid), sizeof(luid)));
}

int
linux_sys_getresgid16(struct lwp *l, const struct linux_sys_getresgid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux_gid16_t *) rgid;
		syscallarg(linux_gid16_t *) egid;
		syscallarg(linux_gid16_t *) sgid;
	} */
	kauth_cred_t pc = l->l_cred;
	int error;
	gid_t bgid;
	linux_gid16_t lgid;

	bgid = kauth_cred_getgid(pc);
	lgid = BSDTOLINUX_GID(bgid);
	if ((error = copyout(&lgid, SCARG(uap, rgid), sizeof(lgid))) != 0)
		return error;

	bgid = kauth_cred_getegid(pc);
	lgid = BSDTOLINUX_GID(bgid);
	if ((error = copyout(&lgid, SCARG(uap, egid), sizeof(lgid))) != 0)
		return error;

	bgid = kauth_cred_getsvgid(pc);
	lgid = BSDTOLINUX_GID(bgid);
	return (copyout(&lgid, SCARG(uap, sgid), sizeof(lgid)));
}
#endif /* !COMPAT_LINUX32 */

int
linux_sys_getgroups16(struct lwp *l, const struct linux_sys_getgroups16_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) gidsetsize;
		syscallarg(linux_gid16_t *) gidset;
	} */
	linux_gid16_t lset[16];
	linux_gid16_t *gidset;
	unsigned int ngrps;
	int i, n, j;
	int error;

	ngrps = kauth_cred_ngroups(l->l_cred);
	*retval = ngrps;
	if (SCARG(uap, gidsetsize) == 0)
		return 0;
	if (SCARG(uap, gidsetsize) < (int)ngrps)
		return EINVAL;

	gidset = SCARG(uap, gidset);
	for (i = 0; i < (n = ngrps); i += n, gidset += n) {
		n -= i;
		if (n > __arraycount(lset))
			n = __arraycount(lset);
		for (j = 0; j < n; j++)
			lset[j] = kauth_cred_group(l->l_cred, i + j);
		error = copyout(lset, gidset, n * sizeof(lset[0]));
		if (error != 0)
			return error;
	}

	return 0;
}

/*
 * It is very unlikly that any problem using 16bit groups is written
 * to allow for more than 16 of them, so don't bother trying to
 * support that.
 */
#define COMPAT_NGROUPS16 16

int
linux_sys_setgroups16(struct lwp *l, const struct linux_sys_setgroups16_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) gidsetsize;
		syscallarg(linux_gid16_t *) gidset;
	} */
	linux_gid16_t lset[COMPAT_NGROUPS16];
	kauth_cred_t ncred;
	int error;
	gid_t grbuf[COMPAT_NGROUPS16];
	unsigned int i, ngroups = SCARG(uap, gidsetsize);

	if (ngroups > COMPAT_NGROUPS16)
		return EINVAL;
	error = copyin(SCARG(uap, gidset), lset, ngroups);
	if (error != 0)
		return error;

	for (i = 0; i < ngroups; i++)
		grbuf[i] = lset[i];

	ncred = kauth_cred_alloc();
	error = kauth_cred_setgroups(ncred, grbuf, SCARG(uap, gidsetsize),
	    -1, UIO_SYSSPACE);
	if (error != 0) {
		kauth_cred_free(ncred);
		return error;
	}

	return kauth_proc_setgroups(l, ncred);
}
