/*	$NetBSD: svr4_32_stat.c,v 1.13.2.1 2004/04/21 03:12:17 jmc Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: svr4_32_stat.c,v 1.13.2.1 2004/04/21 03:12:17 jmc Exp $");

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
#include <sys/unistd.h>

#include <sys/time.h>
#include <sys/ucred.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/svr4_32/svr4_32_types.h>
#include <compat/svr4_32/svr4_32_signal.h>
#include <compat/svr4_32/svr4_32_lwp.h>
#include <compat/svr4_32/svr4_32_ucontext.h>
#include <compat/svr4_32/svr4_32_syscallargs.h>
#include <compat/svr4_32/svr4_32_util.h>
#include <compat/svr4_32/svr4_32_stat.h>
#include <compat/svr4_32/svr4_32_ustat.h>
#include <compat/svr4_32/svr4_32_fuser.h>
#include <compat/svr4/svr4_utsname.h>
#include <compat/svr4/svr4_systeminfo.h>
#include <compat/svr4_32/svr4_32_time.h>
#include <compat/svr4_32/svr4_32_socket.h>

#if defined(__sparc__) || defined(__sparc_v9__) || defined(__sparc64__)
/* 
 * Solaris-2.4 on the sparc has the old stat call using the new
 * stat data structure...
 */
# define SVR4_NO_OSTAT
#endif

static void bsd_to_svr4_32_xstat __P((struct stat *, struct svr4_32_xstat *));
static void bsd_to_svr4_32_stat64 __P((struct stat *, struct svr4_32_stat64 *));
int svr4_32_ustat __P((struct lwp *, void *, register_t *));
static int svr4_32_to_bsd_pathconf __P((int));

/*
 * SVR4 uses named pipes as named sockets, so we tell programs
 * that sockets are named pipes with mode 0
 */
#define BSD_TO_SVR4_MODE(mode) (S_ISSOCK(mode) ? S_IFIFO : (mode))


#ifndef SVR4_NO_OSTAT
static void bsd_to_svr4_32_stat __P((struct stat *, struct svr4_32_stat *));

static void
bsd_to_svr4_32_stat(st, st4)
	struct stat		*st;
	struct svr4_32_stat 	*st4;
{
	memset(st4, 0, sizeof(*st4));
	st4->st_dev = bsd_to_svr4_odev_t(st->st_dev);
	st4->st_ino = st->st_ino;
	st4->st_mode = BSD_TO_SVR4_MODE(st->st_mode);
	st4->st_nlink = st->st_nlink;
	st4->st_uid = st->st_uid;
	st4->st_gid = st->st_gid;
	st4->st_rdev = bsd_to_svr4_odev_t(st->st_rdev);
	st4->st_size = st->st_size;
	st4->st_atim = st->st_atimespec.tv_sec;
	st4->st_mtim = st->st_mtimespec.tv_sec;
	st4->st_ctim = st->st_ctimespec.tv_sec;
}
#endif


static void
bsd_to_svr4_32_xstat(st, st4)
	struct stat		*st;
	struct svr4_32_xstat	*st4;
{
	memset(st4, 0, sizeof(*st4));
	st4->st_dev = bsd_to_svr4_dev_t(st->st_dev);
	st4->st_ino = st->st_ino;
	st4->st_mode = BSD_TO_SVR4_MODE(st->st_mode);
	st4->st_nlink = st->st_nlink;
	st4->st_uid = st->st_uid;
	st4->st_gid = st->st_gid;
	st4->st_rdev = bsd_to_svr4_dev_t(st->st_rdev);
	st4->st_size = st->st_size;
	st4->st_atim.tv_sec = st->st_atimespec.tv_sec;
	st4->st_atim.tv_nsec = st->st_atimespec.tv_nsec;
	st4->st_mtim.tv_sec = st->st_mtimespec.tv_sec;
	st4->st_mtim.tv_nsec = st->st_mtimespec.tv_nsec;
	st4->st_ctim.tv_sec = st->st_ctimespec.tv_sec;
	st4->st_ctim.tv_nsec = st->st_ctimespec.tv_nsec;
	st4->st_blksize = st->st_blksize;
	st4->st_blocks = st->st_blocks;
	strlcpy(st4->st_fstype, "unknown", sizeof(st4->st_fstype));
}


static void
bsd_to_svr4_32_stat64(st, st4)
	struct stat		*st;
	struct svr4_32_stat64	*st4;
{
	memset(st4, 0, sizeof(*st4));
	st4->st_dev = bsd_to_svr4_dev_t(st->st_dev);
	st4->st_ino = st->st_ino;
	st4->st_mode = BSD_TO_SVR4_MODE(st->st_mode);
	st4->st_nlink = st->st_nlink;
	st4->st_uid = st->st_uid;
	st4->st_gid = st->st_gid;
	st4->st_rdev = bsd_to_svr4_dev_t(st->st_rdev);
	st4->st_size = st->st_size;
	st4->st_atim.tv_sec = st->st_atimespec.tv_sec;
	st4->st_atim.tv_nsec = st->st_atimespec.tv_nsec;
	st4->st_mtim.tv_sec = st->st_mtimespec.tv_sec;
	st4->st_mtim.tv_nsec = st->st_mtimespec.tv_nsec;
	st4->st_ctim.tv_sec = st->st_ctimespec.tv_sec;
	st4->st_ctim.tv_nsec = st->st_ctimespec.tv_nsec;
	st4->st_blocks = st->st_blocks;
	strlcpy(st4->st_fstype, "unknown", sizeof(st4->st_fstype));
}


int
svr4_32_sys_stat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_stat_args *uap = v;
#ifdef SVR4_NO_OSTAT
	struct svr4_32_sys_xstat_args_noconst {
		syscallarg(int) two;
		syscallarg(netbsd32_charp) path;
		syscallarg(svr4_32_xstat_tp) ub;
	} cup;

	SCARG(&cup, two) = 2;
	SCARG(&cup, path) = SCARG(uap, path);
	SCARG(&cup, ub) = SCARG(uap, ub);
	return svr4_32_sys_xstat(l, &cup, retval);
#else
	struct proc *p = l->l_proc;
	struct stat		st;
	struct svr4_32_stat	svr4_st;
	struct sys___stat13_args	cup;
	int			error;

	caddr_t sg = stackgap_init(p, 0);
	SCARG(&cup, ub) = stackgap_alloc(p, &sg, sizeof(struct stat));
	SCARG(&cup, path) = SCARG(uap, path);
	SCARG(&cup, path) = (char *)(u_long)SCARG(uap, path);
	CHECK_ALT_EXIST(p, &sg, SCARG(&cup, path));

	if ((error = sys___stat13(p, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, ub), &st, sizeof st)) != 0)
		return error;

	bsd_to_svr4_32_stat(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(p, SCARG(&cup, path), &st);

	if ((error = copyout(&svr4_st, (vaddr_t)(u_long)SCARG(uap, ub), 
			     sizeof svr4_st)) != 0)
		return error;

	return 0;
#endif
}


int
svr4_32_sys_lstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_lstat_args *uap = v;
#ifdef SVR4_NO_OSTAT
	struct svr4_32_sys_lxstat_args_noconst {
		syscallarg(int) two;
		syscallarg(netbsd32_charp) path;
		syscallarg(svr4_32_xstat_tp) ub;
	} cup;

	SCARG(&cup, two) = 2;
	SCARG(&cup, path) = SCARG(uap, path);
	SCARG(&cup, ub) = SCARG(uap, ub);
	return svr4_32_sys_lxstat(l, &cup, retval);
#else
	struct stat		st;
	struct svr4_32_stat	svr4_st;
	struct sys___lstat13_args	cup;
	int			error;

	caddr_t sg = stackgap_init(p, 0);
	SCARG(&cup, ub) = stackgap_alloc(p, &sg, sizeof(struct stat));
	SCARG(&cup, path) = (char *)(u_long)SCARG(uap, path);
	CHECK_ALT_EXIST(p, &sg, SCARG(&cup, path));

	if ((error = sys___lstat13(p, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, ub), &st, sizeof st)) != 0)
		return error;

	bsd_to_svr4_32_stat(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(p, SCARG(&cup, path), &st);

	if ((error = copyout(&svr4_st, (caddr_t)(u_long)SCARG(uap, ub), 
			     sizeof svr4_st)) != 0)
		return error;

	return 0;
#endif
}


int
svr4_32_sys_fstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_fstat_args *uap = v;
#ifdef SVR4_NO_OSTAT
	struct svr4_32_sys_fxstat_args cup;

	SCARG(&cup, two) = 2;
	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, sb) = SCARG(uap, sb);
	return svr4_32_sys_fxstat(l, &cup, retval);
#else
	struct stat		st;
	struct svr4_32_stat	svr4_st;
	struct sys___fstat13_args	cup;
	int			error;

	caddr_t sg = stackgap_init(p, 0);

	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, sb) = stackgap_alloc(p, &sg, sizeof(struct stat));

	if ((error = sys___fstat13(p, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, sb), &st, sizeof st)) != 0)
		return error;

	bsd_to_svr4_32_stat(&st, &svr4_st);

	if ((error = copyout(&svr4_st, (caddr_t)(u_long)SCARG(uap, sb), 
			     sizeof svr4_st)) != 0)
		return error;

	return 0;
#endif
}


int
svr4_32_sys_xstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_xstat_args *uap = v;
	struct proc *p = l->l_proc;
	struct stat		st;
	struct svr4_32_xstat	svr4_st;
	struct sys___stat13_args	cup;
	int			error;

	caddr_t sg = stackgap_init(p, 0);

	SCARG(&cup, ub) = stackgap_alloc(p, &sg, sizeof(struct stat));
	SCARG(&cup, path) = (const char *)(u_long)SCARG(uap, path);
	CHECK_ALT_EXIST(p, &sg, SCARG(&cup, path));

	if ((error = sys___stat13(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, ub), &st, sizeof st)) != 0)
		return error;

	bsd_to_svr4_32_xstat(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(p, (const char *)(u_long)SCARG(uap, path), 
				       &st);

	if ((error = copyout(&svr4_st, (caddr_t)(u_long)SCARG(uap, ub), 
			     sizeof svr4_st)) != 0)
		return error;

	return 0;
}


int
svr4_32_sys_lxstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_lxstat_args *uap = v;
	struct proc *p = l->l_proc;
	struct stat		st;
	struct svr4_32_xstat	svr4_st;
	struct sys___lstat13_args	cup;
	int			error;

	caddr_t sg = stackgap_init(p, 0);

	SCARG(&cup, ub) = stackgap_alloc(p, &sg, sizeof(struct stat));
	SCARG(&cup, path) = (const char *)(u_long)SCARG(uap, path);
	CHECK_ALT_EXIST(p, &sg, SCARG(&cup, path));

	if ((error = sys___lstat13(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, ub), &st, sizeof st)) != 0)
		return error;

	bsd_to_svr4_32_xstat(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(p, (const char *)(u_long)SCARG(uap, path), 
				       &st);

	if ((error = copyout(&svr4_st, (caddr_t)(u_long)SCARG(uap, ub), 
			     sizeof svr4_st)) != 0)
		return error;

	return 0;
}


int
svr4_32_sys_fxstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_fxstat_args *uap = v;
	struct proc *p = l->l_proc;
	struct stat		st;
	struct svr4_32_xstat	svr4_st;
	struct sys___fstat13_args	cup;
	int			error;

	caddr_t sg = stackgap_init(p, 0);

	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, sb) = stackgap_alloc(p, &sg, sizeof(struct stat));

	if ((error = sys___fstat13(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, sb), &st, sizeof st)) != 0)
		return error;

	bsd_to_svr4_32_xstat(&st, &svr4_st);

	if ((error = copyout(&svr4_st, (caddr_t)(u_long)SCARG(uap, sb), 
			     sizeof svr4_st)) != 0)
		return error;

	return 0;
}


int
svr4_32_sys_stat64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_stat64_args *uap = v;
	struct proc *p = l->l_proc;
	struct stat		st;
	struct svr4_32_stat64	svr4_st;
	struct sys___stat13_args	cup;
	int			error;

	caddr_t sg = stackgap_init(p, 0);

	SCARG(&cup, ub) = stackgap_alloc(p, &sg, sizeof(struct stat));
	SCARG(&cup, path) = (const char *)(u_long)SCARG(uap, path);
	CHECK_ALT_EXIST(p, &sg, SCARG(&cup, path));

	if ((error = sys___stat13(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, ub), &st, sizeof st)) != 0)
		return error;

	bsd_to_svr4_32_stat64(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(p, (const char *)(u_long)SCARG(uap, path), 
				       &st);

	if ((error = copyout(&svr4_st,  (caddr_t)(u_long)SCARG(uap, sb), 
			     sizeof svr4_st)) != 0)
		return error;

	return 0;
}


int
svr4_32_sys_lstat64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_lstat64_args *uap = v;
	struct proc *p = l->l_proc;
	struct stat		st;
	struct svr4_32_stat64	svr4_st;
	struct sys___lstat13_args	cup;
	int			error;

	caddr_t sg = stackgap_init(p, 0);

	SCARG(&cup, ub) = stackgap_alloc(p, &sg, sizeof(struct stat));
	SCARG(&cup, path) = (const char *)(u_long)SCARG(uap, path);
	CHECK_ALT_EXIST(p, &sg, SCARG(&cup, path));

	if ((error = sys___lstat13(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, ub), &st, sizeof st)) != 0)
		return error;

	bsd_to_svr4_32_stat64(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(p, (const char *)(u_long)SCARG(uap, path), 
				       &st);

	if ((error = copyout(&svr4_st, (caddr_t)(u_long)SCARG(uap, sb), 
			     sizeof svr4_st)) != 0)
		return error;

	return 0;
}


int
svr4_32_sys_fstat64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_fstat64_args *uap = v;
	struct proc *p = l->l_proc;
	struct stat		st;
	struct svr4_32_stat64	svr4_st;
	struct sys___fstat13_args	cup;
	int			error;

	caddr_t sg = stackgap_init(p, 0);

	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, sb) = stackgap_alloc(p, &sg, sizeof(struct stat));

	if ((error = sys___fstat13(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&cup, sb), &st, sizeof st)) != 0)
		return error;

	bsd_to_svr4_32_stat64(&st, &svr4_st);

	if ((error = copyout(&svr4_st, (caddr_t)(u_long)SCARG(uap, sb), 
			     sizeof svr4_st)) != 0)
		return error;

	return 0;
}


struct svr4_32_ustat_args {
	syscallarg(svr4_dev_t)		dev;
	syscallarg(svr4_32_ustatp)	name;
};

int
svr4_32_ustat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_ustat_args /* {
		syscallarg(svr4_dev_t)		dev;
		syscallarg(svr4_32_ustatp)	name;
	} */ *uap = v;
	struct svr4_32_ustat	us;
	int			error;

	memset(&us, 0, sizeof us);

	/*
         * XXX: should set f_tfree and f_tinode at least
         * How do we translate dev -> fstat? (and then to svr4_32_ustat)
         */
	if ((error = copyout(&us, (caddr_t)(u_long)SCARG(uap, name), 
			     sizeof us)) != 0)
		return (error);

	return 0;
}



int
svr4_32_sys_uname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_uname_args *uap = v;
	struct svr4_utsname	sut;

	memset(&sut, 0, sizeof(sut));

	strncpy(sut.sysname, ostype, sizeof(sut.sysname));
	sut.sysname[sizeof(sut.sysname) - 1] = '\0';

	strncpy(sut.nodename, hostname, sizeof(sut.nodename));
	sut.nodename[sizeof(sut.nodename) - 1] = '\0';

	strncpy(sut.release, osrelease, sizeof(sut.release));
	sut.release[sizeof(sut.release) - 1] = '\0';

	strncpy(sut.version, version, sizeof(sut.version));
	sut.version[sizeof(sut.version) - 1] = '\0';

	strncpy(sut.machine, machine, sizeof(sut.machine));
	sut.machine[sizeof(sut.machine) - 1] = '\0';

	return copyout((caddr_t) &sut, (caddr_t)(u_long)SCARG(uap, name),
		       sizeof(struct svr4_utsname));
}


int
svr4_32_sys_systeminfo(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_systeminfo_args *uap = v;
	struct proc *p = l->l_proc;
	const char *str = NULL;
	int name[2];
	int error;
	size_t len;
	char buf[256];

	u_int rlen = SCARG(uap, len);

	switch (SCARG(uap, what)) {
	case SVR4_SI_SYSNAME:
		str = ostype;
		break;

	case SVR4_SI_HOSTNAME:
		str = hostname;
		break;

	case SVR4_SI_RELEASE:
		str = osrelease;
		break;

	case SVR4_SI_VERSION:
		str = version;
		break;

	case SVR4_SI_MACHINE:
		str = "sun4u"; /* XXXX -- this only runs on sparcv9 */
		break;

	case SVR4_SI_ARCHITECTURE:
		str = machine_arch;
		break;

	case SVR4_SI_HW_SERIAL:
		snprintf(buf, sizeof(buf), "%lu", hostid);
		str = buf;
		break;

	case SVR4_SI_HW_PROVIDER:
		str = ostype;
		break;

	case SVR4_SI_SRPC_DOMAIN:
		str = domainname;
		break;

	case SVR4_SI_PLATFORM:
#ifdef __i386__
		str = "i86pc";
#elif __sparc__
		{
			extern char machine_model[];

			str = machine_model;
		}
#else
		str = "unknown";
#endif
		break;

	case SVR4_SI_KERB_REALM:
		str = "unsupported";
		break;

	case SVR4_SI_SET_HOSTNAME:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return error;
		name[1] = KERN_HOSTNAME;
		break;

	case SVR4_SI_SET_SRPC_DOMAIN:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return error;
		name[1] = KERN_DOMAINNAME;
		break;

	case SVR4_SI_SET_KERB_REALM:
		return 0;

	default:
		DPRINTF(("Bad systeminfo command %d\n", SCARG(uap, what)));
		return ENOSYS;
	}

	if (str) {
		len = strlen(str) + 1;
		if (len > rlen)
			len = rlen;

		if (SCARG(uap, buf)) {
			error = copyout(str, (caddr_t)(u_long)SCARG(uap, buf), len);
			if (error)
				return error;
			/* make sure we are NULL terminated */
			buf[0] = '\0';
			error = copyout(buf, &(((caddr_t)(u_long)
						SCARG(uap, buf))[len - 1]), 1);
		}
		else
			error = 0;
	}
	else {
		error = copyinstr((caddr_t)(u_long)SCARG(uap, buf), buf, 
				  sizeof(buf), &len);
		if (error)
			return error;
		name[0] = CTL_KERN;
		error = old_sysctl(&name[0], 1, 0, 0, buf, len, NULL);
	}

	*retval = len;
	return error;
}


int
svr4_32_sys_utssys(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_utssys_args *uap = v;

	switch (SCARG(uap, sel)) {
	case 0:		/* uname(2)  */
		{
			struct svr4_32_sys_uname_args ua;
			SCARG(&ua, name) = SCARG(uap, a1);
			return svr4_32_sys_uname(l, &ua, retval);
		}

	case 2:		/* ustat(2)  */
		{
			struct svr4_32_ustat_args ua;
			SCARG(&ua, dev) = (svr4_32_dev_t) SCARG(uap, a2);
			SCARG(&ua, name) = SCARG(uap, a1);
			return svr4_32_ustat(l, &ua, retval);
		}

	case 3:		/* fusers(2) */
		return ENOSYS;

	default:
		return ENOSYS;
	}
	return ENOSYS;
}


int
svr4_32_sys_utime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_utime_args *uap = v;
	struct proc *p = l->l_proc;
	struct svr4_32_utimbuf ub;
	struct timeval tbuf[2];
	struct sys_utimes_args ap;
	int error;
	void *ttp;
	caddr_t sg = stackgap_init(p, 0);

	ttp = stackgap_alloc(p, &sg, sizeof(tbuf));
	SCARG(&ap, path) = (const char *)(u_long)SCARG(uap, path);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ap, path));
	if (SCARG(uap, ubuf)) {
		if ((error = copyin((caddr_t)(u_long)SCARG(uap, ubuf), 
				    &ub, sizeof(ub))) != 0)
			return error;
		tbuf[0].tv_sec = ub.actime;
		tbuf[0].tv_usec = 0;
		tbuf[1].tv_sec = ub.modtime;
		tbuf[1].tv_usec = 0;
		error = copyout(tbuf, ttp, sizeof(tbuf));
		if (error)
			return error;
		SCARG(&ap, tptr) = ttp;
	}
	else
		SCARG(&ap, tptr) = NULL;
	return sys_utimes(l, &ap, retval);
}


int
svr4_32_sys_utimes(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_utimes_args *uap = v;
	struct proc *p = l->l_proc;
	struct sys_utimes_args ua;
	caddr_t sg = stackgap_init(p, 0);
	SCARG(&ua, path) = (const char *)(u_long)SCARG(uap, path);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));
	SCARG(&ua, tptr) = (const struct timeval *)(u_long)SCARG(uap, tptr);

	return sys_utimes(l, &ua, retval);
}


static int
svr4_32_to_bsd_pathconf(name)
	int name;
{
	switch (name) {
	case SVR4_PC_LINK_MAX:
	    	return _PC_LINK_MAX;

	case SVR4_PC_MAX_CANON:
		return _PC_MAX_CANON;

	case SVR4_PC_MAX_INPUT:
		return _PC_MAX_INPUT;

	case SVR4_PC_NAME_MAX:
		return _PC_NAME_MAX;

	case SVR4_PC_PATH_MAX:
		return _PC_PATH_MAX;

	case SVR4_PC_PIPE_BUF:
		return _PC_PIPE_BUF;

	case SVR4_PC_NO_TRUNC:
		return _PC_NO_TRUNC;

	case SVR4_PC_VDISABLE:
		return _PC_VDISABLE;

	case SVR4_PC_CHOWN_RESTRICTED:
		return _PC_CHOWN_RESTRICTED;

	case SVR4_PC_SYNC_IO:
		return _PC_SYNC_IO;

	case SVR4_PC_FILESIZEBITS:
		return _PC_FILESIZEBITS;

	case SVR4_PC_ASYNC_IO:
	case SVR4_PC_PRIO_IO:
		/* Not supported */
		return 0;

	default:
		/* Invalid */
		return -1;
	}
}


int
svr4_32_sys_pathconf(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_pathconf_args *uap = v;
	struct proc *p = l->l_proc;
	struct sys_pathconf_args /* { 
		syscallarg(char *) path;
		syscallarg(int) name;
	} */ ua;
	caddr_t sg = stackgap_init(p, 0);

	SCARG(&ua, path) = (char *)(u_long)SCARG(uap, path);

	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	SCARG(&ua, name) = svr4_32_to_bsd_pathconf(SCARG(&ua, name));

	switch (SCARG(&ua, name)) {
	case -1:
		*retval = -1;
		return EINVAL;
	case 0:
		*retval = 0;
		return 0;
	default:
		return sys_pathconf(l, uap, retval);
	}
}


int
svr4_32_sys_fpathconf(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys_fpathconf_args *uap = v;

	SCARG(uap, name) = svr4_32_to_bsd_pathconf(SCARG(uap, name));

	switch (SCARG(uap, name)) {
	case -1:
		*retval = -1;
		return EINVAL;
	case 0:
		*retval = 0;
		return 0;
	default:
		return sys_fpathconf(l, uap, retval);
	}
}
