/*	$NetBSD: null.c,v 1.3 2007/01/11 17:48:21 pooka Exp $	*/

/*
 * Copyright (c) 2007  Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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
#if !defined(lint)
__RCSID("$NetBSD: null.c,v 1.3 2007/01/11 17:48:21 pooka Exp $");
#endif /* !lint */

/*
 * A "nullfs" using puffs, i.e. maps one location in the hierarchy
 * to another using standard system calls.
 */

#include <sys/types.h>
#include <sys/time.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <puffs.h>
#include <stdio.h>
#include <unistd.h>

PUFFSOP_PROTOS(puffs_null)

static void *
pathcmp(struct puffs_node *pn, void *arg)
{

	if (strcmp(arg, pn->pn_path) == 0)
		return pn;
	return NULL;
}

/*
 * set attributes to what is specified.  XXX: no rollback in case of failure
 */
static int
processvattr(const char *path, const struct vattr *va, int regular)
{
	struct timeval tv[2];

	/* XXX: -1 == PUFFS_VNOVAL, but shouldn't trust that */
	if (va->va_uid != (unsigned)-1 || va->va_gid != (unsigned)-1)
		if (lchown(path, va->va_uid, va->va_gid) == -1)
			return errno;

	if (va->va_mode != (unsigned)PUFFS_VNOVAL)
		if (lchmod(path, va->va_mode) == -1)
			return errno;

	/* sloppy */
	if (va->va_atime.tv_sec != (unsigned)PUFFS_VNOVAL
	    || va->va_mtime.tv_sec != (unsigned)PUFFS_VNOVAL) {
		TIMESPEC_TO_TIMEVAL(&tv[0], &va->va_atime);
		TIMESPEC_TO_TIMEVAL(&tv[1], &va->va_mtime);

		if (lutimes(path, tv) == -1)
			return errno;
	}

	if (regular && va->va_size != (u_quad_t)PUFFS_VNOVAL)
		if (truncate(path, (off_t)va->va_size) == -1)
			return errno;

	return 0;
}

/*
 * Kludge to open files which aren't writable *any longer*.  This kinda
 * works because the vfs layer does validation checks based on the file's
 * permissions to allow writable opening before opening them.  However,
 * the problem arises if we want to create a file, write to it (cache),
 * adjust permissions and then flush the file.
 */
static int
writeableopen(const char *path)
{
	struct stat sb;
	mode_t origmode;
	int sverr = 0;
	int fd;

	fd = open(path, O_WRONLY);
	if (fd == -1) {
		if (errno == EACCES) {
			if (stat(path, &sb) == -1)
				return errno;
			origmode = sb.st_mode & ALLPERMS;

			if (chmod(path, 0200) == -1)
				return errno;

			fd = open(path, O_WRONLY);
			if (fd == -1)
				sverr = errno;

			chmod(path, origmode);
			if (sverr)
				errno = sverr;
		} else
			return errno;
	}

	return fd;
}

/*ARGSUSED*/
int
puffs_null_fs_statvfs(struct puffs_cc *pcc, struct statvfs *svfsb, pid_t pid)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);

	if (statvfs(pu->pu_pn_root->pn_path, svfsb) == -1)
		return errno;

	return 0;
}

/*
 * Wow, this kinda looks like a real lookup since it bounces all
 * over the place
 */
int
puffs_null_node_lookup(struct puffs_cc *pcc, void *opc, void **newnode,
	enum vtype *newtype, voff_t *newsize, dev_t *newrdev,
	const struct puffs_cn *pcn)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn = opc, *pn_res;
	struct dirent entry, *result;
	char dpath[MAXPATHLEN+1];
	char *npath;
	struct stat sb;
	DIR *dp;
	int rv;

	assert(pn->pn_va.va_type == VDIR);

	if (pcn->pcn_flags & PUFFS_ISDOTDOT) {
		char *p;
		strcpy(dpath, pn->pn_path);
		p = strrchr(dpath, '/');
		assert(p != NULL);
		*p = '\0';

		pn_res = puffs_pn_nodewalk(pu, pathcmp, dpath);
		if (pn_res)
			goto success;
		npath = dpath;
		goto getnew;
	}

	dp = opendir(pn->pn_path);
	if (dp == NULL)
		return errno;

	/* more readable than for(), IMHO */
	rv = readdir_r(dp, &entry, &result);
	while (rv == 0 && result) {
		if (strcmp(result->d_name, pcn->pcn_name) == 0)
			break;
		rv = readdir_r(dp, &entry, &result);
	}
	closedir(dp);

	if (rv)
		return rv;
	if (!result)
		return ENOENT;

	pn_res = puffs_pn_nodewalk(pu, pathcmp, pcn->pcn_fullpath);
	if (pn_res)
		goto success;
	npath = pcn->pcn_fullpath;

 getnew:
	rv = lstat(npath, &sb);
	if (rv)
		return rv;
	pn_res = puffs_pn_new(pu, NULL);
	if (pn_res == NULL)
		return ENOMEM;
	puffs_stat2vattr(&pn_res->pn_va, &sb);

 success:
	*newnode = pn_res;
	*newtype = pn_res->pn_va.va_type;
	*newsize = pn_res->pn_va.va_size;
	*newrdev = pn_res->pn_va.va_rdev;

	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_create(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn;
	int fd, rv;

	fd = open(pcn->pcn_fullpath, O_RDWR | O_CREAT | O_TRUNC);
	if (fd == -1)
		return errno;
	close(fd);
	if ((rv = processvattr(pcn->pcn_fullpath, va, 1)) != 0) {
		unlink(pcn->pcn_fullpath);
		return rv;
	}

	pn = puffs_pn_new(pu, NULL);
	if (!pn) {
		unlink(pcn->pcn_fullpath);
		return ENOMEM;
	}
	puffs_setvattr(&pn->pn_va, va);

	*newnode = pn;
	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_mknod(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn;
	int rv;

	if (mknod(pcn->pcn_fullpath, va->va_mode, va->va_rdev) == -1)
		return errno;

	if ((rv = processvattr(pcn->pcn_fullpath, va, 0)) != 0) {
		unlink(pcn->pcn_fullpath);
		return rv;
	}

	pn = puffs_pn_new(pu, NULL);
	if (!pn) {
		unlink(pcn->pcn_fullpath);
		return ENOMEM;
	}
	puffs_setvattr(&pn->pn_va, va);

	*newnode = pn;
	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_getattr(struct puffs_cc *pcc, void *opc, struct vattr *va,
	const struct puffs_cred *pcred, pid_t pid)
{
	struct puffs_node *pn = opc;
	struct stat sb;

	if (lstat(pn->pn_path, &sb) == -1)
		return errno;
	puffs_stat2vattr(va, &sb);

	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_setattr(struct puffs_cc *pcc, void *opc,
	const struct vattr *va, const struct puffs_cred *pcred, pid_t pid)
{
	struct puffs_node *pn = opc;
	int rv;

	rv = processvattr(pn->pn_path, va, pn->pn_va.va_type == VREG);
	if (rv)
		return rv;

	puffs_setvattr(&pn->pn_va, va);

	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_fsync(struct puffs_cc *pcc, void *opc,
	const struct puffs_cred *pcred, int how,
	off_t offlo, off_t offhi, pid_t pid)
{
	struct puffs_node *pn = opc;
	int fd, rv;
	int fflags;

	rv = 0;
	fd = writeableopen(pn->pn_path);
	if (fd == -1)
		return errno;

	if (how & PUFFS_FSYNC_DATAONLY)
		fflags = FDATASYNC;
	else
		fflags = FFILESYNC;
	if (how & PUFFS_FSYNC_CACHE)
		fflags |= FDISKSYNC;

	if (fsync_range(fd, fflags, offlo, offhi - offlo) == -1)
		rv = errno;

	close(fd);

	return rv;
}

/*ARGSUSED*/
int
puffs_null_node_remove(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node *pn_targ = targ;

	if (unlink(pn_targ->pn_path) == -1)
		return errno;

	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_link(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node *pn_targ = targ;

	if (link(pn_targ->pn_path, pcn->pcn_fullpath) == -1)
		return errno;

	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_rename(struct puffs_cc *pcc, void *opc, void *src,
	const struct puffs_cn *pcn_src, void *targ_dir, void *targ,
	const struct puffs_cn *pcn_targ)
{

	if (rename(pcn_src->pcn_fullpath, pcn_targ->pcn_fullpath) == -1)
		return errno;

	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_mkdir(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn;
	int rv;

	if (mkdir(pcn->pcn_fullpath, va->va_mode) == -1)
		return errno;

	if ((rv = processvattr(pcn->pcn_fullpath, va, 0)) != 0) {
		unlink(pcn->pcn_fullpath);
		return rv;
	}

	pn = puffs_pn_new(pu, NULL);
	if (pn == NULL) {
		rmdir(pcn->pcn_fullpath);
		return ENOMEM;
	}
	puffs_setvattr(&pn->pn_va, va);

	*newnode = pn;
	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_rmdir(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node *pn_targ = targ;

	if (rmdir(pn_targ->pn_path) == -1)
		return errno;

	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_symlink(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va,
	const char *linkname)
{
	struct puffs_usermount *pu = puffs_cc_getusermount(pcc);
	struct puffs_node *pn;
	int rv;

	if (symlink(linkname, pcn->pcn_fullpath) == -1)
		return errno;

	if ((rv = processvattr(pcn->pcn_fullpath, va, 0)) != 0) {
		unlink(pcn->pcn_fullpath);
		return rv;
	}

	pn = puffs_pn_new(pu, NULL);
	if (pn == NULL) {
		rmdir(pcn->pcn_fullpath);
		return ENOMEM;
	}
	puffs_setvattr(&pn->pn_va, va);

	*newnode = pn;
	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_readlink(struct puffs_cc *pcc, void *opc,
	const struct puffs_cred *pcred, char *linkname, size_t *linklen)
{
	struct puffs_node *pn = opc;
	ssize_t rv;

	rv = readlink(pn->pn_path, linkname, *linklen);
	if (rv == -1)
		return errno;

	*linklen -= rv;
	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_readdir(struct puffs_cc *pcc, void *opc, struct dirent *de,
	const struct puffs_cred *pcred, off_t *off, size_t *reslen)
{
	struct puffs_node *pn = opc;
	struct dirent entry, *result;
	DIR *dp;
	off_t i;
	int rv;

	dp = opendir(pn->pn_path);
	if (dp == NULL)
		return errno;

	rv = 0;
	i = *off;

	/*
	 * XXX: need to do trickery here, telldir/seekdir would be nice, but
	 * then we'd need to keep state, which I'm too lazy to keep
	 */
	while (i--) {
		rv = readdir_r(dp, &entry, &result);
		if (rv || !result)
			goto out;
	}

	for (;;) {
		rv = readdir_r(dp, &entry, &result);
		if (rv != 0)
			goto out;

		if (!result)
			goto out;

		if (_DIRENT_SIZE(result) > *reslen)
			goto out;

		*de = *result;
		*reslen -= _DIRENT_SIZE(result);
		de = _DIRENT_NEXT(de);

		(*off)++;
	}

 out:
	closedir(dp);
	return 0;
}

/*ARGSUSED*/
int
puffs_null_node_read(struct puffs_cc *pcc, void *opc, uint8_t *buf,
	off_t offset, size_t *buflen, const struct puffs_cred *pcred,
	int ioflag)
{
	struct puffs_node *pn = opc;
	ssize_t n;
	off_t off;
	int fd, rv;

	rv = 0;
	fd = open(pn->pn_path, O_RDONLY);
	if (fd == -1)
		return errno;
	off = lseek(fd, offset, SEEK_SET);
	if (off == -1) {
		rv = errno;
		goto out;
	}

	n = read(fd, buf, *buflen);
	if (n == -1)
		rv = errno;
	else
		*buflen -= n;

 out:
	close(fd);
	return rv;
}

/*ARGSUSED*/
int
puffs_null_node_write(struct puffs_cc *pcc, void *opc, uint8_t *buf,
	off_t offset, size_t *buflen, const struct puffs_cred *pcred,
	int ioflag)
{
	struct puffs_node *pn = opc;
	ssize_t n;
	off_t off;
	int fd, rv;

	rv = 0;
	fd = writeableopen(pn->pn_path);
	if (fd == -1)
		return errno;

	off = lseek(fd, offset, SEEK_SET);
	if (off == -1) {
		rv = errno;
		goto out;
	}

	n = write(fd, buf, *buflen);
	if (n == -1)
		rv = errno;
	else
		*buflen -= n;

 out:
	close(fd);
	return rv;
}
