/*	$NetBSD: netbsd32_compat_12.c,v 1.6.2.1 2000/12/08 09:08:32 bouyer Exp $	*/

/*
 * Copyright (c) 1998 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

static void netbsd32_stat12_to_netbsd32 __P((struct stat12 *,
		struct netbsd32_stat12 *));

/* for use with {,fl}stat() */
static void
netbsd32_stat12_to_netbsd32(sp12, sp32)
	struct stat12 *sp12;
	struct netbsd32_stat12 *sp32;
{

	sp32->st_dev = sp12->st_dev;
	sp32->st_ino = sp12->st_ino;
	sp32->st_mode = sp12->st_mode;
	sp32->st_nlink = sp12->st_nlink;
	sp32->st_uid = sp12->st_uid;
	sp32->st_gid = sp12->st_gid;
	sp32->st_rdev = sp12->st_rdev;
	if (sp12->st_size < (quad_t)1 << 32)
		sp32->st_size = sp12->st_size;
	else
		sp32->st_size = -2;
	sp32->st_atimespec.tv_sec = sp12->st_atimespec.tv_sec;
	sp32->st_atimespec.tv_nsec = sp12->st_atimespec.tv_nsec;
	sp32->st_mtimespec.tv_sec = sp12->st_mtimespec.tv_sec;
	sp32->st_mtimespec.tv_nsec = sp12->st_mtimespec.tv_nsec;
	sp32->st_ctimespec.tv_sec = sp12->st_ctimespec.tv_sec;
	sp32->st_ctimespec.tv_nsec = sp12->st_ctimespec.tv_nsec;
	sp32->st_blocks = sp12->st_blocks;
	sp32->st_blksize = sp12->st_blksize;
	sp32->st_flags = sp12->st_flags;
	sp32->st_gen = sp12->st_gen;
}

int
compat_12_netbsd32_reboot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_12_netbsd32_reboot_args /* {
		syscallarg(int) opt;
	} */ *uap = v;
	struct compat_12_sys_reboot_args ua;

	NETBSD32TO64_UAP(opt);
	return (compat_12_sys_reboot(p, &ua, retval));
}

int
compat_12_netbsd32_msync(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_12_netbsd32_msync_args /* {
		syscallarg(netbsd32_caddr_t) addr;
		syscallarg(netbsd32_size_t) len;
	} */ *uap = v;
	struct sys___msync13_args ua;

	NETBSD32TOX64_UAP(addr, caddr_t);
	NETBSD32TOX_UAP(len, size_t);
	SCARG(&ua, flags) = MS_SYNC | MS_INVALIDATE;
	return (sys___msync13(p, &ua, retval));
}

int
compat_12_netbsd32_oswapon(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_12_netbsd32_oswapon_args /* {
		syscallarg(const netbsd32_charp) name;
	} */ *uap = v;
	struct sys_swapctl_args ua;

	SCARG(&ua, cmd) = SWAP_ON;
	SCARG(&ua, arg) = (void *)(u_long)SCARG(uap, name);
	SCARG(&ua, misc) = 0;	/* priority */
	return (sys_swapctl(p, &ua, retval));
}

int
compat_12_netbsd32_stat12(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_12_netbsd32_stat12_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat12p_t) ub;
	} */ *uap = v;
	struct netbsd32_stat12 *sp32;
	struct stat12 sb12;
	struct stat12 *sp12 = &sb12;
	struct compat_12_sys_stat_args ua;
	caddr_t sg;
	int rv;

	NETBSD32TOP_UAP(path, const char);
	SCARG(&ua, ub) = &sb12;
	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	rv = compat_12_sys_stat(p, &ua, retval);

	sp32 = (struct netbsd32_stat12 *)(u_long)SCARG(uap, ub);
	netbsd32_stat12_to_netbsd32(sp12, sp32);

	return (rv);
}

int
compat_12_netbsd32_fstat12(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_12_netbsd32_fstat12_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_stat12p_t) sb;
	} */ *uap = v;
	struct netbsd32_stat12 *sp32;
	struct stat12 sb12;
	struct stat12 *sp12 = &sb12;
	struct compat_12_sys_fstat_args ua;
	int rv;

	NETBSD32TO64_UAP(fd);
	SCARG(&ua, sb) = &sb12;
	rv = compat_12_sys_fstat(p, &ua, retval);

	sp32 = (struct netbsd32_stat12 *)(u_long)SCARG(uap, sb);
	netbsd32_stat12_to_netbsd32(sp12, sp32);

	return (rv);
}

int
compat_12_netbsd32_lstat12(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_12_netbsd32_lstat12_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat12p_t) ub;
	} */ *uap = v;
	struct netbsd32_stat12 *sp32;
	struct stat12 sb12;
	struct stat12 *sp12 = &sb12;
	struct compat_12_sys_lstat_args ua;
	caddr_t sg;
	int rv;

	NETBSD32TOP_UAP(path, const char);
	SCARG(&ua, ub) = &sb12;
	sg = stackgap_init(p->p_emul);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	rv = compat_12_sys_lstat(p, &ua, retval);

	sp32 = (struct netbsd32_stat12 *)(u_long)SCARG(uap, ub);
	netbsd32_stat12_to_netbsd32(sp12, sp32);

	return (rv);
}

int
compat_12_netbsd32_getdirentries(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct compat_12_netbsd32_getdirentries_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) buf;
		syscallarg(u_int) count;
		syscallarg(netbsd32_longp) basep;
	} */ *uap = v;
	struct compat_12_sys_getdirentries_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, char);
	NETBSD32TO64_UAP(count);
	NETBSD32TOP_UAP(basep, long);

	return (compat_12_sys_getdirentries(p, &ua, retval));
}
