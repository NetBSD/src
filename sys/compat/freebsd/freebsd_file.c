/*	$NetBSD: freebsd_file.c,v 1.20 2004/04/21 01:05:36 christos Exp $	*/

/*
 * Copyright (c) 1995 Frank van der Linden
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project
 *      by Frank van der Linden
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	from: linux_file.c,v 1.3 1995/04/04 04:21:30 mycroft Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: freebsd_file.c,v 1.20 2004/04/21 01:05:36 christos Exp $");

#if defined(_KERNEL_OPT)
#include "fs_nfs.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/malloc.h>

#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/freebsd/freebsd_syscallargs.h>
#include <compat/common/compat_util.h>

#define	ARRAY_LENGTH(array)	(sizeof(array)/sizeof(array[0]))

static const char * convert_from_freebsd_mount_type __P((int));

static const char *
convert_from_freebsd_mount_type(type)
	int type;
{
	static const char * const netbsd_mount_type[] = {
		NULL,     /*  0 = MOUNT_NONE */
		"ffs",	  /*  1 = "Fast" Filesystem */
		"nfs",	  /*  2 = Network Filesystem */
		"mfs",	  /*  3 = Memory Filesystem */
		"msdos",  /*  4 = MSDOS Filesystem */
		"lfs",	  /*  5 = Log-based Filesystem */
		"lofs",	  /*  6 = Loopback filesystem */
		"fdesc",  /*  7 = File Descriptor Filesystem */
		"portal", /*  8 = Portal Filesystem */
		"null",	  /*  9 = Minimal Filesystem Layer */
		"umap",	  /* 10 = User/Group Identifier Remapping Filesystem */
		"kernfs", /* 11 = Kernel Information Filesystem */
		"procfs", /* 12 = /proc Filesystem */
		"afs",	  /* 13 = Andrew Filesystem */
		"cd9660", /* 14 = ISO9660 (aka CDROM) Filesystem */
		"union",  /* 15 = Union (translucent) Filesystem */
		NULL,     /* 16 = "devfs" - existing device Filesystem */
#if 0 /* These filesystems don't exist in FreeBSD */
		"adosfs", /* ?? = AmigaDOS Filesystem */
#endif
	};

	if (type < 0 || type >= ARRAY_LENGTH(netbsd_mount_type))
		return (NULL);
	return (netbsd_mount_type[type]);
}

int
freebsd_sys_mount(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_mount_args /* {
		syscallarg(int) type;
		syscallarg(char *) path;
		syscallarg(int) flags;
		syscallarg(caddr_t) data;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;
	const char *type;
	char *s;
	caddr_t sg = stackgap_init(p, 0);
	struct sys_mount_args bma;

	if ((type = convert_from_freebsd_mount_type(SCARG(uap, type))) == NULL)
		return ENODEV;
	s = stackgap_alloc(p, &sg, MFSNAMELEN + 1);
	if ((error = copyout(type, s, strlen(type) + 1)) != 0)
		return error;
	SCARG(&bma, type) = s;
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	SCARG(&bma, path) = SCARG(uap, path);
	SCARG(&bma, flags) = SCARG(uap, flags);
	SCARG(&bma, data) = SCARG(uap, data);
	return sys_mount(l, &bma, retval);
}

/*
 * The following syscalls are only here because of the alternate path check.
 */

/* XXX - UNIX domain: int freebsd_sys_bind(int s, caddr_t name, int namelen); */
/* XXX - UNIX domain: int freebsd_sys_connect(int s, caddr_t name, int namelen); */


int
freebsd_sys_open(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_open_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	if (SCARG(uap, flags) & O_CREAT)
		CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	else
		CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_open(l, uap, retval);
}

int
compat_43_freebsd_sys_creat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_freebsd_sys_creat_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg  = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return compat_43_sys_creat(l, uap, retval);
}

int
freebsd_sys_link(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_link_args /* {
		syscallarg(char *) path;
		syscallarg(char *) link;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, link));
	return sys_link(l, uap, retval);
}

int
freebsd_sys_unlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_unlink_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_unlink(l, uap, retval);
}

int
freebsd_sys_chdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_chdir_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chdir(l, uap, retval);
}

int
freebsd_sys_mknod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_mknod_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return sys_mknod(l, uap, retval);
}

int
freebsd_sys_chmod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_chmod_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chmod(l, uap, retval);
}

int
freebsd_sys_chown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_chown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chown(l, uap, retval);
}

int
freebsd_sys_lchown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_lchown_args /* {
		syscallarg(char *) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));
	return sys_lchown(l, uap, retval);
}

int
freebsd_sys_unmount(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_unmount_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_unmount(l, uap, retval);
}

int
freebsd_sys_access(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_access_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_access(l, uap, retval);
}

int
freebsd_sys_chflags(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_chflags_args /* {
		syscallarg(char *) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chflags(l, uap, retval);
}

int
compat_43_freebsd_sys_stat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_freebsd_sys_stat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat43 *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_43_sys_stat(l, uap, retval);
}

int
compat_43_freebsd_sys_lstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_freebsd_sys_lstat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat43 *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_43_sys_lstat(l, uap, retval);
}

int
freebsd_sys_revoke(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_revoke_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_revoke(l, uap, retval);
}

int
freebsd_sys_symlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_symlink_args /* {
		syscallarg(char *) path;
		syscallarg(char *) link;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, link));
	return sys_symlink(l, uap, retval);
}

int
freebsd_sys_readlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_readlink_args /* {
		syscallarg(char *) path;
		syscallarg(char *) buf;
		syscallarg(int) count;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_SYMLINK(p, &sg, SCARG(uap, path));
	return sys_readlink(l, uap, retval);
}

int
freebsd_sys_execve(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_execve_args /* {
		syscallarg(char *) path;
		syscallarg(char **) argp;
		syscallarg(char **) envp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sys_execve_args ap;
	caddr_t sg;

	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&ap, path) = SCARG(uap, path);
	SCARG(&ap, argp) = SCARG(uap, argp);
	SCARG(&ap, envp) = SCARG(uap, envp);

	return sys_execve(l, &ap, retval);
}

int
freebsd_sys_chroot(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_chroot_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_chroot(l, uap, retval);
}

int
freebsd_sys_rename(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_rename_args /* {
		syscallarg(char *) from;
		syscallarg(char *) to;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, from));
	CHECK_ALT_CREAT(p, &sg, SCARG(uap, to));
	return sys_rename(l, uap, retval);
}

int
compat_43_freebsd_sys_truncate(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_freebsd_sys_truncate_args /* {
		syscallarg(char *) path;
		syscallarg(long) length;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_43_sys_truncate(l, uap, retval);
}

int
freebsd_sys_mkfifo(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_mkfifo_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return sys_mkfifo(l, uap, retval);
}

int
freebsd_sys_mkdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_mkdir_args /* {
		syscallarg(char *) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_CREAT(p, &sg, SCARG(uap, path));
	return sys_mkdir(l, uap, retval);
}

int
freebsd_sys_rmdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_rmdir_args /* {
		syscallarg(char *) path;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_rmdir(l, uap, retval);
}

int
freebsd_sys_statfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_stat_args /* {
		syscallarg(char *) path;
		syscallarg(struct statfs12 *) buf;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_20_sys_statfs(l, uap, retval);
}

#ifdef NFS
int
freebsd_sys_getfh(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_getfh_args /* {
		syscallarg(char *) fname;
		syscallarg(fhandle_t *) fhp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, fname));
	return sys_getfh(l, uap, retval);
}
#endif /* NFS */

int
freebsd_sys_stat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_stat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat12 *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_12_sys_stat(l, uap, retval);
}

int
freebsd_sys_lstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_lstat_args /* {
		syscallarg(char *) path;
		syscallarg(struct stat12 *) ub;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return compat_12_sys_lstat(l, uap, retval);
}

int
freebsd_sys_pathconf(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_pathconf_args /* {
		syscallarg(char *) path;
		syscallarg(int) name;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_pathconf(l, uap, retval);
}

int
freebsd_sys_truncate(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct freebsd_sys_truncate_args /* {
		syscallarg(char *) path;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));
	return sys_truncate(l, uap, retval);
}
