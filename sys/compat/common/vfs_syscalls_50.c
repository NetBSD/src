/*	$NetBSD: vfs_syscalls_50.c,v 1.12 2012/01/29 06:29:04 dholland Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: vfs_syscalls_50.c,v 1.12 2012/01/29 06:29:04 dholland Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/dirent.h>
#include <sys/malloc.h>
#include <sys/kauth.h>
#include <sys/time.h>
#include <sys/vfs_syscalls.h>
#ifndef LFS
#define LFS
#endif
#include <sys/syscallargs.h>

#include <ufs/lfs/lfs_extern.h>

#include <sys/quota.h>
#include <quota/quotaprop.h>
#include <ufs/ufs/quota1.h>

#include <compat/common/compat_util.h>
#include <compat/sys/time.h>
#include <compat/sys/stat.h>
#include <compat/sys/dirent.h>
#include <compat/sys/mount.h>

static void cvtstat(struct stat30 *, const struct stat *);

/*
 * Convert from a new to an old stat structure.
 */
static void
cvtstat(struct stat30 *ost, const struct stat *st)
{

	ost->st_dev = st->st_dev;
	ost->st_ino = st->st_ino;
	ost->st_mode = st->st_mode;
	ost->st_nlink = st->st_nlink;
	ost->st_uid = st->st_uid;
	ost->st_gid = st->st_gid;
	ost->st_rdev = st->st_rdev;
	timespec_to_timespec50(&st->st_atimespec, &ost->st_atimespec);
	timespec_to_timespec50(&st->st_mtimespec, &ost->st_mtimespec);
	timespec_to_timespec50(&st->st_ctimespec, &ost->st_ctimespec);
	timespec_to_timespec50(&st->st_birthtimespec, &ost->st_birthtimespec);
	ost->st_size = st->st_size;
	ost->st_blocks = st->st_blocks;
	ost->st_blksize = st->st_blksize;
	ost->st_flags = st->st_flags;
	ost->st_gen = st->st_gen;
	memset(ost->st_spare, 0, sizeof(ost->st_spare));
}

/*
 * Get file status; this version follows links.
 */
/* ARGSUSED */
int
compat_50_sys___stat30(struct lwp *l, const struct compat_50_sys___stat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct stat30 *) ub;
	} */
	struct stat sb;
	struct stat30 osb;
	int error;

	error = do_sys_stat(SCARG(uap, path), FOLLOW, &sb);
	if (error)
		return error;
	cvtstat(&osb, &sb);
	error = copyout(&osb, SCARG(uap, ub), sizeof (osb));
	return error;
}


/*
 * Get file status; this version does not follow links.
 */
/* ARGSUSED */
int
compat_50_sys___lstat30(struct lwp *l, const struct compat_50_sys___lstat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(struct stat30 *) ub;
	} */
	struct stat sb;
	struct stat30 osb;
	int error;

	error = do_sys_stat(SCARG(uap, path), NOFOLLOW, &sb);
	if (error)
		return error;
	cvtstat(&osb, &sb);
	error = copyout(&osb, SCARG(uap, ub), sizeof (osb));
	return error;
}

/*
 * Return status information about a file descriptor.
 */
/* ARGSUSED */
int
compat_50_sys___fstat30(struct lwp *l, const struct compat_50_sys___fstat30_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct stat30 *) sb;
	} */
	struct stat sb;
	struct stat30 osb;
	int error;

	error = do_sys_fstat(SCARG(uap, fd), &sb);
	if (error)
		return error;
	cvtstat(&osb, &sb);
	error = copyout(&osb, SCARG(uap, sb), sizeof (osb));
	return error;
}

/* ARGSUSED */
int
compat_50_sys___fhstat40(struct lwp *l, const struct compat_50_sys___fhstat40_args *uap, register_t *retval)
{
	/* {
		syscallarg(const void *) fhp;
		syscallarg(size_t) fh_size;
		syscallarg(struct stat30 *) sb;
	} */
	struct stat sb;
	struct stat30 osb;
	int error;

	error = do_fhstat(l, SCARG(uap, fhp), SCARG(uap, fh_size), &sb);
	if (error)
		return error;
	cvtstat(&osb, &sb);
	error = copyout(&osb, SCARG(uap, sb), sizeof (osb));
	return error;
}

static int
compat_50_do_sys_utimes(struct lwp *l, struct vnode *vp, const char *path,
    int flag, const struct timeval50 *tptr)
{
	struct timeval tv[2], *tvp;
	struct timeval50 tv50[2];
	if (tptr) {
		int error = copyin(tptr, tv50, sizeof(tv50));
		if (error)
			return error;
		timeval50_to_timeval(&tv50[0], &tv[0]);
		timeval50_to_timeval(&tv50[1], &tv[1]);
		tvp = tv;
	} else
		tvp = NULL;
	return do_sys_utimes(l, vp, path, flag, tvp, UIO_SYSSPACE);
}
    
/*
 * Set the access and modification times given a path name; this
 * version follows links.
 */
/* ARGSUSED */
int
compat_50_sys_utimes(struct lwp *l, const struct compat_50_sys_utimes_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(const struct timeval50 *) tptr;
	} */

	return compat_50_do_sys_utimes(l, NULL, SCARG(uap, path), FOLLOW,
	    SCARG(uap, tptr));
}

/*
 * Set the access and modification times given a file descriptor.
 */
/* ARGSUSED */
int
compat_50_sys_futimes(struct lwp *l,
    const struct compat_50_sys_futimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const struct timeval50 *) tptr;
	} */
	int error;
	struct file *fp;

	/* fd_getvnode() will use the descriptor for us */
	if ((error = fd_getvnode(SCARG(uap, fd), &fp)) != 0)
		return error;
	error = compat_50_do_sys_utimes(l, fp->f_data, NULL, 0,
	    SCARG(uap, tptr));
	fd_putfile(SCARG(uap, fd));
	return error;
}

/*
 * Set the access and modification times given a path name; this
 * version does not follow links.
 */
int
compat_50_sys_lutimes(struct lwp *l,
    const struct compat_50_sys_lutimes_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(const struct timeval50 *) tptr;
	} */

	return compat_50_do_sys_utimes(l, NULL, SCARG(uap, path), NOFOLLOW,
	    SCARG(uap, tptr));
}

int
compat_50_sys_lfs_segwait(struct lwp *l,
    const struct compat_50_sys_lfs_segwait_args *uap, register_t *retval)
{
	/* {
		syscallarg(fsid_t *) fsidp;
		syscallarg(struct timeval50 *) tv;
	} */
#ifdef notyet
	struct timeval atv;
	struct timeval50 atv50;
	fsid_t fsid;
	int error;

	/* XXX need we be su to segwait? */
	if ((error = kauth_authorize_generic(l->l_cred, KAUTH_GENERIC_ISSUSER,
	    NULL)) != 0)
		return (error);
	if ((error = copyin(SCARG(uap, fsidp), &fsid, sizeof(fsid_t))) != 0)
		return (error);

	if (SCARG(uap, tv)) {
		error = copyin(SCARG(uap, tv), &atv50, sizeof(atv50));
		if (error)
			return (error);
		timeval50_to_timeval(&atv50, &atv);
		if (itimerfix(&atv))
			return (EINVAL);
	} else /* NULL or invalid */
		atv.tv_sec = atv.tv_usec = 0;
	return lfs_segwait(&fsid, &atv);
#else
	return ENOSYS;
#endif
}

int
compat_50_sys_mknod(struct lwp *l,
    const struct compat_50_sys_mknod_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(mode_t) mode;
		syscallarg(uint32_t) dev;
	} */
	return do_sys_mknod(l, SCARG(uap, path), SCARG(uap, mode),
	    SCARG(uap, dev), retval, UIO_USERSPACE);
}

/* ARGSUSED */
int   
compat_50_sys_quotactl(struct lwp *l, const struct compat_50_sys_quotactl_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(void *) arg; 
	} */
	struct mount *mp;
	int error;
	uint8_t error8;
	struct vnode *vp;
	int q1cmd = SCARG(uap, cmd);
	prop_dictionary_t dict, data, cmd;
	prop_array_t cmds, datas;
	char *bufpath;
	struct dqblk dqblk;
	struct quotaval qv[QUOTA_NLIMITS];
	uint64_t *values[QUOTA_NLIMITS];

	values[QUOTA_LIMIT_BLOCK] = &qv[QUOTA_LIMIT_BLOCK].qv_hardlimit;
	values[QUOTA_LIMIT_FILE] = &qv[QUOTA_LIMIT_FILE].qv_hardlimit;

	error = namei_simple_user(SCARG(uap, path),
				NSM_FOLLOW_TRYEMULROOT, &vp);
	if (error != 0)
		return (error);       
	error = ENOMEM;
	mp = vp->v_mount;
	if ((dict = quota_prop_create()) == NULL)
		goto out;
	if ((cmds = prop_array_create()) == NULL)
		goto out_dict;
	if ((datas = prop_array_create()) == NULL)
		goto out_cmds;

	switch((q1cmd & ~SUBCMDMASK) >> SUBCMDSHIFT) {
	case Q_QUOTAON:
		data = prop_dictionary_create();
		if (data == NULL)
			goto out_datas;
		bufpath = malloc(PATH_MAX * sizeof(char), M_TEMP, M_WAITOK);
		if (bufpath == NULL)
			goto out_data;
		error = copyinstr(SCARG(uap, arg), bufpath, PATH_MAX, NULL);
		if (error != 0) {
			free(bufpath, M_TEMP);
			goto out_data;
		}
		if (!prop_dictionary_set_cstring(data, "quotafile", bufpath))
			error = ENOMEM;
		free(bufpath, M_TEMP);
		if (error)
			goto out_data;
		error = ENOMEM;
		if (!prop_array_add_and_rel(datas, data))
			goto out_datas;
		if (!quota_prop_add_command(cmds, "quotaon",
		    ufs_quota_class_names[qtype2ufsclass(q1cmd & SUBCMDMASK)],
		    datas))
			goto out_cmds;
		goto do_quotaonoff;

	case Q_QUOTAOFF:
		error = ENOMEM;
		if (!quota_prop_add_command(cmds, "quotaoff",
		    ufs_quota_class_names[qtype2ufsclass(q1cmd & SUBCMDMASK)],
		    datas))
			goto out_cmds;
do_quotaonoff:
		if (!prop_dictionary_set_and_rel(dict, "commands", cmds))
			goto out_dict;
		error = vfs_quotactl(mp, dict);
		if (error)
			goto out_dict;
		if ((error = quota_get_cmds(dict, &cmds)) != 0)
			goto out_dict;
		cmd = prop_array_get(cmds, 0);
		if (cmd == NULL) {
			error = EINVAL;
			goto out_dict;
		}
		if (!prop_dictionary_get_int8(cmd, "return", &error8)) {
			error = EINVAL;
			goto out_dict;
		}
		error = error8;
		goto out_dict;

	case Q_GETQUOTA:
		error = ENOMEM;
		data = prop_dictionary_create();
		if (data == NULL)
			goto out_datas;
		if (!prop_dictionary_set_uint32(data, "id", SCARG(uap, uid)))
			goto out_data;
		if (!prop_array_add_and_rel(datas, data))
			goto out_datas;
		if (!quota_prop_add_command(cmds, "get",
		    ufs_quota_class_names[qtype2ufsclass(q1cmd & SUBCMDMASK)],
		    datas))
			goto out_cmds;
		if (!prop_dictionary_set_and_rel(dict, "commands", cmds))
			goto out_dict;
		error = vfs_quotactl(mp, dict);
		if (error)
			goto out_dict;
		if ((error = quota_get_cmds(dict, &cmds)) != 0)
			goto out_dict;
		cmd = prop_array_get(cmds, 0);
		if (cmd == NULL) {
			error = EINVAL;
			goto out_dict;
		}
		if (!prop_dictionary_get_int8(cmd, "return", &error8)) {
			error = EINVAL;
			goto out_dict;
		}
		error = error8;
		if (error)
			goto out_dict;
		datas = prop_dictionary_get(cmd, "data");
		error = EINVAL;
		if (datas == NULL)
			goto out_dict;
		data = prop_array_get(datas, 0);
		if (data == NULL)
			goto out_dict;
		error = proptoquota64(data, values,
		    ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
		    ufs_quota_limit_names, QUOTA_NLIMITS);
		if (error)
			goto out_dict;
		quotavals_to_dqblk(&qv[QUOTA_LIMIT_BLOCK],
				   &qv[QUOTA_LIMIT_FILE],
				   &dqblk);
		error = copyout(&dqblk, SCARG(uap, arg), sizeof(dqblk));
		goto out_dict;
		
	case Q_SETQUOTA:
		error = copyin(SCARG(uap, arg), &dqblk, sizeof(dqblk));
		if (error)
			goto out_datas;
		dqblk_to_quotavals(&dqblk, &qv[QUOTA_LIMIT_BLOCK],
				   &qv[QUOTA_LIMIT_FILE]);

		error = ENOMEM;
		data = quota64toprop(SCARG(uap, uid), 0, values,
		    ufs_quota_entry_names, UFS_QUOTA_NENTRIES,
		    ufs_quota_limit_names, QUOTA_NLIMITS);
		if (data == NULL)
			goto out_data;
		if (!prop_array_add_and_rel(datas, data))
			goto out_datas;
		if (!quota_prop_add_command(cmds, "set",
		    ufs_quota_class_names[qtype2ufsclass(q1cmd & SUBCMDMASK)],
		    datas))
			goto out_cmds;
		if (!prop_dictionary_set_and_rel(dict, "commands", cmds))
			goto out_dict;
		error = vfs_quotactl(mp, dict);
		if (error)
			goto out_dict;
		if ((error = quota_get_cmds(dict, &cmds)) != 0)
			goto out_dict;
		cmd = prop_array_get(cmds, 0);
		if (cmd == NULL) {
			error = EINVAL;
			goto out_dict;
		}
		if (!prop_dictionary_get_int8(cmd, "return", &error8)) {
			error = EINVAL;
			goto out_dict;
		}
		error = error8;
		goto out_dict;
		
	case Q_SYNC:
		/*
		 * not supported but used only to see if quota is supported,
		 * emulate with a "get version"
		 */
		error = ENOMEM;
		if (!quota_prop_add_command(cmds, "get version",
		    ufs_quota_class_names[qtype2ufsclass(q1cmd & SUBCMDMASK)],
		    datas))
			goto out_cmds;
		if (!prop_dictionary_set_and_rel(dict, "commands", cmds))
			goto out_dict;
		error = vfs_quotactl(mp, dict);
		if (error)
			goto out_dict;
		if ((error = quota_get_cmds(dict, &cmds)) != 0)
			goto out_dict;
		cmd = prop_array_get(cmds, 0);
		if (cmd == NULL) {
			error = EINVAL;
			goto out_dict;
		}
		if (!prop_dictionary_get_int8(cmd, "return", &error8)) {
			error = EINVAL;
			goto out_dict;
		}
		error = error8;
		goto out_dict;

	case Q_SETUSE:
	default:
		error = EOPNOTSUPP;
		goto out_datas;
	}
out_data:
	prop_object_release(data);
out_datas:
	prop_object_release(datas);
out_cmds:
	prop_object_release(cmds);
out_dict:
	prop_object_release(dict);
out:
	vrele(vp);
	return error;
}
