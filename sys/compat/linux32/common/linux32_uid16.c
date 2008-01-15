/* $NetBSD: linux32_uid16.c,v 1.1 2008/01/15 22:38:35 njoly Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
 * Copyright (c) 2008 Nicolas Joly, all rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux32_uid16.c,v 1.1 2008/01/15 22:38:35 njoly Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/kauth.h>

#include <compat/netbsd32/netbsd32.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/linux32_syscallargs.h>

#define LINUX32TOBSD_UID(u) \
	(((u) == (linux32_uid16_t)-1) ? -1 : (u))
#define LINUX32TOBSD_GID(g) \
	(((g) == (linux32_gid16_t)-1) ? -1 : (g))

#define BSDTOLINUX32_UID(u) \
	(((u) & ~0xffff) ? (linux32_uid16_t)65534 : (linux32_uid16_t)(u))
#define BSDTOLINUX32_GID(g) \
	(((g) & ~0xffff) ? (linux32_gid16_t)65534 : (linux32_gid16_t)(g))

int
linux32_sys_chown16(struct lwp *l, const struct linux32_sys_chown16_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(linux32_uid16_t) uid;
		syscallarg(linux32_gid16_t) gid;
	} */
        struct sys___posix_chown_args ua;

	NETBSD32TOP_UAP(path, const char);
	SCARG(&ua, uid) = LINUX32TOBSD_UID(SCARG(uap, uid));
	SCARG(&ua, gid) = LINUX32TOBSD_GID(SCARG(uap, gid));

        return sys___posix_chown(l, &ua, retval);
}

int
linux32_sys_lchown16(struct lwp *l, const struct linux32_sys_lchown16_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(linux32_uid16_t) uid;
		syscallarg(linux32_gid16_t) gid;
	} */
	struct sys___posix_lchown_args ua;

	NETBSD32TOP_UAP(path, const char);
	SCARG(&ua, uid) = LINUX32TOBSD_UID(SCARG(uap, uid));
	SCARG(&ua, gid) = LINUX32TOBSD_GID(SCARG(uap, gid));

	return sys___posix_lchown(l, &ua, retval);
}

int
linux32_sys_fchown16(struct lwp *l, const struct linux32_sys_fchown16_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(linux32_uid16_t) uid;
		syscallarg(linux32_gid16_t) gid;
	} */
	struct sys___posix_fchown_args ua;

	SCARG(&ua, fd) = SCARG(uap, fd);
	SCARG(&ua, uid) = LINUX32TOBSD_UID(SCARG(uap, uid));
	SCARG(&ua, gid) = LINUX32TOBSD_GID(SCARG(uap, gid));

	return sys___posix_fchown(l, &ua, retval);
}

int
linux32_sys_getgroups16(struct lwp *l, const struct linux32_sys_getgroups16_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) gidsetsize;
		syscallarg(linux32_gid16p_t) gidset;
	} */
	struct linux_sys_getgroups16_args ua;

	NETBSD32TO64_UAP(gidsetsize);
	NETBSD32TOP_UAP(gidset, linux_gid16_t);
	
	return linux_sys_getgroups16(l, &ua, retval);
}

int
linux32_sys_setgroups16(struct lwp *l, const struct linux32_sys_setgroups16_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) gidsetsize;
		syscallarg(linux32_gid16p_t) gidset;
	} */
	struct linux_sys_setgroups16_args ua;

	NETBSD32TO64_UAP(gidsetsize);
	NETBSD32TOP_UAP(gidset, linux_gid16_t);
	
	return linux_sys_setgroups16(l, &ua, retval);
}

int
linux32_sys_setreuid16(struct lwp *l, const struct linux32_sys_setreuid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_uid16_t) ruid;
		syscallarg(linux32_uid16_t) euid;
	} */
	struct sys_setreuid_args bsa;

	SCARG(&bsa, ruid) = LINUX32TOBSD_UID(SCARG(uap, ruid));
	SCARG(&bsa, euid) = LINUX32TOBSD_UID(SCARG(uap, euid));

	return sys_setreuid(l, &bsa, retval);
}

int
linux32_sys_setregid16(struct lwp *l, const struct linux32_sys_setregid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_gid16_t) rgid;
		syscallarg(linux32_gid16_t) egid;
	} */
	struct sys_setregid_args bsa;

	SCARG(&bsa, rgid) = LINUX32TOBSD_GID(SCARG(uap, rgid));
	SCARG(&bsa, egid) = LINUX32TOBSD_GID(SCARG(uap, egid));

	return sys_setregid(l, &bsa, retval);
}

int
linux32_sys_setresuid16(struct lwp *l, const struct linux32_sys_setresuid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_uid16_t) ruid;
		syscallarg(linux32_uid16_t) euid;
		syscallarg(linux32_uid16_t) suid;
	} */
	struct linux32_sys_setresuid_args lsa;

	SCARG(&lsa, ruid) = LINUX32TOBSD_UID(SCARG(uap, ruid));
	SCARG(&lsa, euid) = LINUX32TOBSD_UID(SCARG(uap, euid));
	SCARG(&lsa, suid) = LINUX32TOBSD_UID(SCARG(uap, suid));

	return linux32_sys_setresuid(l, &lsa, retval);
}

int
linux32_sys_setresgid16(struct lwp *l, const struct linux32_sys_setresgid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_gid16_t) rgid;
		syscallarg(linux32_gid16_t) egid;
		syscallarg(linux32_gid16_t) sgid;
	} */
	struct linux32_sys_setresgid_args lsa;

	SCARG(&lsa, rgid) = LINUX32TOBSD_GID(SCARG(uap, rgid));
	SCARG(&lsa, egid) = LINUX32TOBSD_GID(SCARG(uap, egid));
	SCARG(&lsa, sgid) = LINUX32TOBSD_GID(SCARG(uap, sgid));

	return linux32_sys_setresgid(l, &lsa, retval);
}

int
linux32_sys_getresuid16(struct lwp *l, const struct linux32_sys_getresuid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_uid16p_t) ruid;
		syscallarg(linux32_uid16p_t) euid;
		syscallarg(linux32_uid16p_t) suid;
	} */
	kauth_cred_t pc = l->l_cred;
	int error;
	uid_t buid;
	linux32_uid16_t luid;

	buid = kauth_cred_getuid(pc);
	luid = BSDTOLINUX32_UID(buid);
	if ((error = copyout(&luid, SCARG_P32(uap, ruid), sizeof(luid))) != 0)
		return error;

	buid = kauth_cred_geteuid(pc);
	luid = BSDTOLINUX32_UID(buid);
	if ((error = copyout(&luid, SCARG_P32(uap, euid), sizeof(luid))) != 0)
		return error;

	buid = kauth_cred_getsvuid(pc);
	luid = BSDTOLINUX32_UID(buid);
	return (copyout(&luid, SCARG_P32(uap, suid), sizeof(luid)));
}

int
linux32_sys_getresgid16(struct lwp *l, const struct linux32_sys_getresgid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_gid16p_t) rgid;
		syscallarg(linux32_gid16p_t) egid;
		syscallarg(linux32_gid16p_t) sgid;
	} */
	kauth_cred_t pc = l->l_cred;
	int error;
	gid_t bgid;
	linux32_gid16_t lgid;

	bgid = kauth_cred_getgid(pc);
	lgid = BSDTOLINUX32_GID(bgid);
	if ((error = copyout(&lgid, SCARG_P32(uap, rgid), sizeof(lgid))) != 0)
		return error;

	bgid = kauth_cred_getegid(pc);
	lgid = BSDTOLINUX32_GID(bgid);
	if ((error = copyout(&lgid, SCARG_P32(uap, egid), sizeof(lgid))) != 0)
		return error;

	bgid = kauth_cred_getsvgid(pc);
	lgid = BSDTOLINUX32_GID(bgid);
	return (copyout(&lgid, SCARG_P32(uap, sgid), sizeof(lgid)));
}
