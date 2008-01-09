/*	$NetBSD: svr4_32_stat.c,v 1.29.8.1 2008/01/09 01:52:01 matt Exp $	 */

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
__KERNEL_RCSID(0, "$NetBSD: svr4_32_stat.c,v 1.29.8.1 2008/01/09 01:52:01 matt Exp $");

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
#include <sys/namei.h>
#include <sys/unistd.h>
#include <sys/kauth.h>

#include <sys/time.h>
#include <sys/ucred.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>
#include <sys/vfs_syscalls.h>

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

static void bsd_to_svr4_32_xstat(struct stat *, struct svr4_32_xstat *);
static void bsd_to_svr4_32_stat64(struct stat *, struct svr4_32_stat64 *);
static int svr4_32_to_bsd_pathconf(int);

/*
 * SVR4 uses named pipes as named sockets, so we tell programs
 * that sockets are named pipes with mode 0
 */
#define BSD_TO_SVR4_MODE(mode) (S_ISSOCK(mode) ? S_IFIFO : (mode))


#ifndef SVR4_NO_OSTAT
static void bsd_to_svr4_32_stat(struct stat *, struct svr4_32_stat *);

static void
bsd_to_svr4_32_stat(struct stat *st, struct svr4_32_stat *st4)
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
bsd_to_svr4_32_xstat(struct stat *st, struct svr4_32_xstat *st4)
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
bsd_to_svr4_32_stat64(struct stat *st, struct svr4_32_stat64 *st4)
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
svr4_32_sys_stat(struct lwp *l, const struct svr4_32_sys_stat_args *uap, register_t *retval)
{
#ifdef SVR4_NO_OSTAT
	struct svr4_32_sys_xstat_args cup;

	SCARG(&cup, two) = 2;
	SCARG(&cup, path) = SCARG(uap, path);
	SCARG(&cup, ub) = SCARG(uap, ub);
	return svr4_32_sys_xstat(l, &cup, retval);
#else
	struct stat		st;
	struct svr4_32_stat	svr4_st;
	int			error;

	error = do_sys_stat(l, SCARG(&cup, path), FOLLOW, &st);
	if (error != 0)
		return error;

	bsd_to_svr4_32_stat(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(p, SCARG(&cup, path), &st);

	return copyout(&svr4_st, SCARG_P32(uap, ub),
			     sizeof svr4_st);
#endif
}


int
svr4_32_sys_lstat(struct lwp *l, const struct svr4_32_sys_lstat_args *uap, register_t *retval)
{
#ifdef SVR4_NO_OSTAT
	struct svr4_32_sys_lxstat_args cup;

	SCARG(&cup, two) = 2;
	SCARG(&cup, path) = SCARG(uap, path);
	SCARG(&cup, ub) = SCARG(uap, ub);
	return svr4_32_sys_lxstat(l, &cup, retval);
#else
	struct stat		st;
	struct svr4_32_stat	svr4_st;
	int			error;

	error = do_sys_stat(l, SCARG(&cup, path), NOFOLLOW, &st);
	if (error != 0)
		return error;

	bsd_to_svr4_32_stat(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(p, SCARG(&cup, path), &st);

	return copyout(&svr4_st, SCARG_P32(uap, ub),
			     sizeof svr4_st);
#endif
}


int
svr4_32_sys_fstat(struct lwp *l, const struct svr4_32_sys_fstat_args *uap, register_t *retval)
{
#ifdef SVR4_NO_OSTAT
	struct svr4_32_sys_fxstat_args cup;

	SCARG(&cup, two) = 2;
	SCARG(&cup, fd) = SCARG(uap, fd);
	SCARG(&cup, sb) = SCARG(uap, sb);
	return svr4_32_sys_fxstat(l, &cup, retval);
#else
	struct stat		st;
	struct svr4_32_stat	svr4_st;
	int			error;

	error = do_sys_fstat(l, SCARG(uap, fd), &st);
	if (error != 0)
		return error;

	bsd_to_svr4_32_stat(&st, &svr4_st);

	return copyout(&svr4_st, SCARG_P32(uap, ub),
			     sizeof svr4_st);
#endif
}


int
svr4_32_sys_xstat(struct lwp *l, const struct svr4_32_sys_xstat_args *uap, register_t *retval)
{
	struct stat		st;
	struct svr4_32_xstat	svr4_st;
	int			error;
	const char *path = SCARG_P32(uap, path);

	error = do_sys_stat(l, path, FOLLOW, &st);
	if (error != 0)
		return error;

	bsd_to_svr4_32_xstat(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(l->l_proc, path, &st);

	return copyout(&svr4_st, SCARG_P32(uap, ub),
			     sizeof svr4_st);
}


int
svr4_32_sys_lxstat(struct lwp *l, const struct svr4_32_sys_lxstat_args *uap, register_t *retval)
{
	struct stat		st;
	struct svr4_32_xstat	svr4_st;
	int			error;
	const char *path = SCARG_P32(uap, path);

	error = do_sys_stat(l, path, NOFOLLOW, &st);
	if (error != 0)
		return error;

	bsd_to_svr4_32_xstat(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(l->l_proc, path, &st);

	return copyout(&svr4_st, SCARG_P32(uap, ub),
			     sizeof svr4_st);
}


int
svr4_32_sys_fxstat(struct lwp *l, const struct svr4_32_sys_fxstat_args *uap, register_t *retval)
{
	struct stat		st;
	struct svr4_32_xstat	svr4_st;
	int			error;

	error = do_sys_fstat(l, SCARG(uap, fd), &st);
	if (error != 0)
		return error;

	bsd_to_svr4_32_xstat(&st, &svr4_st);

	return copyout(&svr4_st, SCARG_P32(uap, sb),
			     sizeof svr4_st);
}


int
svr4_32_sys_stat64(struct lwp *l, const struct svr4_32_sys_stat64_args *uap, register_t *retval)
{
	struct stat		st;
	struct svr4_32_stat64	svr4_st;
	int			error;
	const char *path = SCARG_P32(uap, path);

	error = do_sys_stat(l, path, FOLLOW, &st);
	if (error != 0)
		return error;

	bsd_to_svr4_32_stat64(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(l->l_proc, path, &st);

	return copyout(&svr4_st,  SCARG_P32(uap, sb),
			     sizeof svr4_st);
}


int
svr4_32_sys_lstat64(struct lwp *l, const struct svr4_32_sys_lstat64_args *uap, register_t *retval)
{
	struct stat		st;
	struct svr4_32_stat64	svr4_st;
	int			error;
	const char *path = SCARG_P32(uap, path);

	error = do_sys_stat(l, path, NOFOLLOW, &st);
	if (error != 0)
		return error;

	bsd_to_svr4_32_stat64(&st, &svr4_st);

	if (S_ISSOCK(st.st_mode))
		(void) svr4_add_socket(l->l_proc, path, &st);

	return copyout(&svr4_st, SCARG_P32(uap, sb),
			     sizeof svr4_st);
}


int
svr4_32_sys_fstat64(struct lwp *l, const struct svr4_32_sys_fstat64_args *uap, register_t *retval)
{
	struct stat		st;
	struct svr4_32_stat64	svr4_st;
	int			error;

	error = do_sys_fstat(l, SCARG(uap, fd), &st);
	if (error != 0)
		return error;

	bsd_to_svr4_32_stat64(&st, &svr4_st);

	return copyout(&svr4_st, SCARG_P32(uap, sb),
			     sizeof svr4_st);
}


struct svr4_32_ustat_args {
	syscallarg(svr4_dev_t)		dev;
	syscallarg(svr4_32_ustatp)	name;
};

static int
svr4_32_ustat(struct lwp *l, const struct svr4_32_ustat_args *uap, register_t *retval)
{
	/* {
		syscallarg(svr4_dev_t)		dev;
		syscallarg(svr4_32_ustatp)	name;
	} */
	struct svr4_32_ustat	us;
	int			error;

	memset(&us, 0, sizeof us);

	/*
         * XXX: should set f_tfree and f_tinode at least
         * How do we translate dev -> fstat? (and then to svr4_32_ustat)
         */
	if ((error = copyout(&us, SCARG_P32(uap, name),
			     sizeof us)) != 0)
		return (error);

	return 0;
}



int
svr4_32_sys_uname(struct lwp *l, const struct svr4_32_sys_uname_args *uap, register_t *retval)
{
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

	return copyout((void *) &sut, SCARG_P32(uap, name),
		       sizeof(struct svr4_utsname));
}


int
svr4_32_sys_systeminfo(struct lwp *l, const struct svr4_32_sys_systeminfo_args *uap, register_t *retval)
{
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
		str = "sun4m"; /* Lie, pretend we are 4m */
		break;

	case SVR4_SI_ARCHITECTURE:
#if defined(__sparc__)
		str = "sparc";
#else
		str = machine_arch;
#endif
		break;

	case SVR4_SI_ISALIST:
#if defined(__sparc__)
		str = "sparcv8 sparcv8-fsmuld sparcv7 sparc";
#elif defined(__i386__)
		str = "i386";
#else
		str = "unknown";
#endif
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
#if defined(__i386__)
		str = "i86pc";
#elif defined(__sparc__)
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
		if ((error = kauth_authorize_generic(l->l_cred,
		    KAUTH_GENERIC_ISSUSER, NULL)) != 0)
			return error;
		name[1] = KERN_HOSTNAME;
		break;

	case SVR4_SI_SET_SRPC_DOMAIN:
		if ((error = kauth_authorize_generic(l->l_cred,
		    KAUTH_GENERIC_ISSUSER, NULL)) != 0)
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
		if (len < rlen)
			rlen = len;

		if (SCARG_P32(uap, buf)) {
			error = copyout(str, SCARG_P32(uap, buf),
			    rlen);
			if (error)
				return error;
			if (rlen > 0) {
				/* make sure we are NULL terminated */
				buf[0] = '\0';
				error = copyout(buf, &(((char *)
				    SCARG_P32(uap, buf))[rlen - 1]), 1);
			}
		}
		else
			error = 0;
	}
	else {
		error = copyinstr(SCARG_P32(uap, buf), buf,
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
svr4_32_sys_utssys(struct lwp *l, const struct svr4_32_sys_utssys_args *uap, register_t *retval)
{

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
			SCARG(&ua, dev) =  (uintptr_t)SCARG_P32(uap, a2);
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
svr4_32_sys_utime(struct lwp *l, const struct svr4_32_sys_utime_args *uap, register_t *retval)
{
	struct svr4_32_utimbuf ub;
	struct timeval tbuf[2], *tvp;
	int error;

	if (SCARG_P32(uap, ubuf)) {
		if ((error = copyin(SCARG_P32(uap, ubuf),
				    &ub, sizeof(ub))) != 0)
			return error;
		tbuf[0].tv_sec = ub.actime;
		tbuf[0].tv_usec = 0;
		tbuf[1].tv_sec = ub.modtime;
		tbuf[1].tv_usec = 0;
		tvp = tbuf;
	}
	else
		tvp = NULL;
	return do_sys_utimes(l, NULL, SCARG_P32(uap, path), FOLLOW,
			    tvp, UIO_SYSSPACE);
}


int
svr4_32_sys_utimes(struct lwp *l, const struct svr4_32_sys_utimes_args *uap, register_t *retval)
{
	struct sys_utimes_args ua;
	SCARG(&ua, path) = SCARG_P32(uap, path);
	SCARG(&ua, tptr) = SCARG_P32(uap, tptr);

	return sys_utimes(l, &ua, retval);
}


static int
svr4_32_to_bsd_pathconf(int name)
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
svr4_32_sys_pathconf(struct lwp *l, const struct svr4_32_sys_pathconf_args *uap, register_t *retval)
{
	struct sys_pathconf_args /* {
		syscallarg(char *) path;
		syscallarg(int) name;
	} */ ua;

	SCARG(&ua, path) = SCARG_P32(uap, path);
	SCARG(&ua, name) = svr4_32_to_bsd_pathconf(SCARG(&ua, name));

	switch (SCARG(&ua, name)) {
	case -1:
		*retval = -1;
		return EINVAL;
	case 0:
		*retval = 0;
		return 0;
	default:
		return sys_pathconf(l, &ua, retval);
	}
}


int
svr4_32_sys_fpathconf(struct lwp *l, const struct svr4_32_sys_fpathconf_args *uap, register_t *retval)
{
	struct sys_fpathconf_args ua;

	SCARG(&ua, fd) = SCARG(uap, fd);
	SCARG(&ua, name) = svr4_32_to_bsd_pathconf(SCARG(uap, name));

	switch (SCARG(&ua, name)) {
	case -1:
		*retval = -1;
		return EINVAL;
	case 0:
		*retval = 0;
		return 0;
	default:
		return sys_fpathconf(l, &ua, retval);
	}
}
