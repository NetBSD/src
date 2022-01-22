/*	$NetBSD: refuse.c,v 1.114 2022/01/22 08:09:39 pho Exp $	*/

/*
 * Copyright © 2007 Alistair Crooks.  All rights reserved.
 * Copyright © 2007 Antti Kantee.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: refuse.c,v 1.114 2022/01/22 08:09:39 pho Exp $");
#endif /* !lint */

#include <sys/types.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fuse_internal.h>
#include <fuse_opt.h>
#include <paths.h>
#include <puffs.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef MULTITHREADED_REFUSE
#include <pthread.h>
#endif

typedef uint64_t	 fuse_ino_t;

struct refuse_config {
	int debug;
	char *fsname;
};

#define REFUSE_OPT(t, p, v) \
	{ t, offsetof(struct refuse_config, p), v }

static struct fuse_opt refuse_opts[] = {
	REFUSE_OPT("debug"    , debug , 1),
	REFUSE_OPT("fsname=%s", fsname, 0),
	FUSE_OPT_END
};

struct puffs_fuse_dirh {
	void *dbuf;
	struct dirent *d;

	size_t reslen;
	size_t bufsize;
};

struct refusenode {
	struct fuse_file_info	file_info;
	struct puffs_fuse_dirh	dirh;
	int opencount;
	int flags;
};
#define RN_ROOT		0x01
#define RN_OPEN		0x02	/* XXX: could just use opencount */

static int fuse_setattr(struct fuse *, struct puffs_node *,
			const char *, const struct vattr *);

static struct puffs_node *
newrn(struct puffs_usermount *pu)
{
	struct puffs_node *pn;
	struct refusenode *rn;

	if ((rn = calloc(1, sizeof(*rn))) == NULL) {
		err(EXIT_FAILURE, "newrn");
	}
	pn = puffs_pn_new(pu, rn);

	return pn;
}

static void
nukern(struct puffs_node *pn)
{
	struct refusenode *rn = pn->pn_data;

	free(rn->dirh.dbuf);
	free(rn);
	puffs_pn_put(pn);
}

/* XXX - not threadsafe */
static ino_t fakeino = 3;

/***************** start of pthread context routines ************************/

/*
 * Notes on fuse_context:
 * we follow fuse's lead and use the pthread specific information to hold
 * a reference to the fuse_context structure for this thread.
 */
#ifdef MULTITHREADED_REFUSE
static pthread_mutex_t		context_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t		context_key;
static unsigned long		context_refc;
#endif

/* return the fuse_context struct related to this thread */
struct fuse_context *
fuse_get_context(void)
{
#ifdef MULTITHREADED_REFUSE
	struct fuse_context	*ctxt;

	if ((ctxt = pthread_getspecific(context_key)) == NULL) {
		if ((ctxt = calloc(1, sizeof(struct fuse_context))) == NULL) {
			abort();
		}
		pthread_setspecific(context_key, ctxt);
	}
	return ctxt;
#else
	static struct fuse_context	fcon;

	return &fcon;
#endif
}

/* used as a callback function */
#ifdef MULTITHREADED_REFUSE
static void
free_context(void *ctxt)
{   
	free(ctxt);
}
#endif

/*
 * Create the pthread key.  The reason for the complexity is to
 * enable use of multiple fuse instances within a single process.
 */
static int
create_context_key(void)
{   
#ifdef MULTITHREADED_REFUSE
	int rv;

	rv = pthread_mutex_lock(&context_mutex);
	assert(rv == 0);

	if (context_refc == 0) {
		if (pthread_key_create(&context_key, free_context) != 0) {
			warnx("create_context_key: pthread_key_create failed");
			pthread_mutex_unlock(&context_mutex);
			return 0;
		}
	}
	context_refc += 1;
	pthread_mutex_unlock(&context_mutex);
	return 1;
#else
	return 1;
#endif
}

/* struct fuse_context is potentially reused among different
 * invocations of fuse_new() / fuse_destroy() pair. Clear its content
 * on fuse_destroy() so that no dangling pointers remain in the
 * context. */
static void
clear_context(void)
{
	struct fuse_context	*ctx;

	ctx = fuse_get_context();
	memset(ctx, 0, sizeof(*ctx));
}

static void
delete_context_key(void)
{   
#ifdef MULTITHREADED_REFUSE
	pthread_mutex_lock(&context_mutex);
	/* If we are the last fuse instances using the key, delete it */
	if (--context_refc == 0) {
		free(pthread_getspecific(context_key));
		pthread_key_delete(context_key);
	}
	pthread_mutex_unlock(&context_mutex);
#endif
}

/* set the uid and gid of the calling process in the current fuse context */
static void
set_fuse_context_uid_gid(const struct puffs_cred *cred)
{
	struct fuse_context	*fusectx;
	uid_t			 uid;
	gid_t			 gid;

	fusectx = fuse_get_context();
	if (puffs_cred_getuid(cred, &uid) == 0) {
		fusectx->uid = uid;
	}
	if (puffs_cred_getgid(cred, &gid) == 0) {
		fusectx->gid = gid;
	}
}

/* set the pid of the calling process in the current fuse context */
static void
set_fuse_context_pid(struct puffs_usermount *pu)
{
	struct puffs_cc		*pcc = puffs_cc_getcc(pu);
	struct fuse_context	*fusectx;

	fusectx = fuse_get_context();
	puffs_cc_getcaller(pcc, &fusectx->pid, NULL);
}

/***************** end of pthread context routines ************************/

#define DIR_CHUNKSIZE 4096
static int
fill_dirbuf(struct puffs_fuse_dirh *dh, const char *name, ino_t dino,
	uint8_t dtype)
{

	/* initial? */
	if (dh->bufsize == 0) {
		if ((dh->dbuf = calloc(1, DIR_CHUNKSIZE)) == NULL) {
			abort();
		}
		dh->d = dh->dbuf;
		dh->reslen = dh->bufsize = DIR_CHUNKSIZE;
	}

	if (puffs_nextdent(&dh->d, name, dino, dtype, &dh->reslen)) {
		return 0;
	}

	/* try to increase buffer space */
	dh->dbuf = realloc(dh->dbuf, dh->bufsize + DIR_CHUNKSIZE);
	if (dh->dbuf == NULL) {
		abort();
	}
	dh->d = (void *)((uint8_t *)dh->dbuf + (dh->bufsize - dh->reslen));
	dh->reslen += DIR_CHUNKSIZE;
	dh->bufsize += DIR_CHUNKSIZE;

	return !puffs_nextdent(&dh->d, name, dino, dtype, &dh->reslen);
}

/* ARGSUSED3 */
/* XXX: I have no idea how "off" is supposed to be used */
static int
puffs_fuse_fill_dir(void *buf, const char *name,
	const struct stat *stbuf, off_t off, enum fuse_fill_dir_flags flags)
{
	struct puffs_fuse_dirh *deh = buf;
	ino_t dino;
	uint8_t dtype;

	if (stbuf == NULL) {
		dtype = DT_UNKNOWN;
		dino = fakeino++;
	} else {
		dtype = (uint8_t)puffs_vtype2dt(puffs_mode2vt(stbuf->st_mode));
		dino = stbuf->st_ino;

		/*
		 * Some FUSE file systems like to always use 0 as the
		 * inode number.   Our readdir() doesn't like to show
		 * directory entries with inode number 0 ==> workaround.
		 */
		if (dino == 0) {
			dino = fakeino++;
		}
	}

	return fill_dirbuf(deh, name, dino, dtype);
}

/* ARGSUSED1 */
static int
fuse_getattr(struct fuse *fuse, struct puffs_node *pn, const char *path,
	struct vattr *va)
{
	struct refusenode	*rn = pn->pn_data;
	struct fuse_file_info	*fi = rn->opencount > 0 ? &rn->file_info : NULL;
	struct stat		 st;
	int			ret;

	/* wrap up return code */
	memset(&st, 0, sizeof(st));
	ret = fuse_fs_getattr_v30(fuse->fs, path, &st, fi);

	if (ret == 0) {
		if (st.st_blksize == 0)
			st.st_blksize = DEV_BSIZE;
		puffs_stat2vattr(va, &st);
	}

	return -ret;
}

/* utility function to set various elements of the attribute */
static int
fuse_setattr(struct fuse *fuse, struct puffs_node *pn, const char *path,
	const struct vattr *va)
{
	struct refusenode	*rn = pn->pn_data;
	struct fuse_file_info	*fi = rn->opencount > 0 ? &rn->file_info : NULL;
	mode_t			mode;
	uid_t			uid;
	gid_t			gid;
	int			error, ret;

	error = 0;

	mode = va->va_mode;
	uid = va->va_uid;
	gid = va->va_gid;

	if (mode != (mode_t)PUFFS_VNOVAL) {
		ret = fuse_fs_chmod_v30(fuse->fs, path, mode, fi);
		if (ret)
			error = ret;
	}
	if (uid != (uid_t)PUFFS_VNOVAL || gid != (gid_t)PUFFS_VNOVAL) {
		ret = fuse_fs_chown_v30(fuse->fs, path, uid, gid, fi);
		if (ret)
			error = ret;
	}
	if (va->va_atime.tv_sec != (time_t)PUFFS_VNOVAL
		|| va->va_mtime.tv_sec != (long)PUFFS_VNOVAL) {

		struct timespec	tv[2];

		tv[0].tv_sec	= va->va_atime.tv_sec;
		tv[0].tv_nsec	= va->va_atime.tv_nsec;
		tv[1].tv_sec	= va->va_mtime.tv_sec;
		tv[1].tv_nsec	= va->va_mtime.tv_nsec;

		ret = fuse_fs_utimens_v30(fuse->fs, path, tv, fi);
		if (ret)
			error = ret;
	}
	if (va->va_size != (u_quad_t)PUFFS_VNOVAL) {
		ret = fuse_fs_truncate_v30(fuse->fs, path, (off_t)va->va_size, fi);
		if (ret)
			error = ret;
	}
	/* XXX: no reflection with reality */
	puffs_setvattr(&pn->pn_va, va);

	return -error;

}

static int
fuse_newnode(struct puffs_usermount *pu, const char *path,
	const struct vattr *va, struct fuse_file_info *fi,
	struct puffs_newinfo *pni, struct puffs_node **pn_new)
{
	struct puffs_node	*pn;
	struct refusenode	*rn;
	struct vattr		 newva;
	struct fuse		*fuse;

	fuse = puffs_getspecific(pu);

	/* fix up nodes */
	pn = newrn(pu);
	if (pn == NULL) {
		if (va->va_type == VDIR) {
			fuse_fs_rmdir(fuse->fs, path);
		} else {
			fuse_fs_unlink(fuse->fs, path);
		}
		return ENOMEM;
	}
	fuse_setattr(fuse, pn, path, va);
	if (fuse_getattr(fuse, pn, path, &newva) == 0)
		puffs_setvattr(&pn->pn_va, &newva);

	rn = pn->pn_data;
	if (fi)
		memcpy(&rn->file_info, fi, sizeof(struct fuse_file_info));

	puffs_newinfo_setcookie(pni, pn);
	if (pn_new)
		*pn_new = pn;

	return 0;
}


/* operation wrappers start here */

/* lookup the path */
/* ARGSUSED1 */
static int
puffs_fuse_node_lookup(struct puffs_usermount *pu, void *opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn)
{
	struct puffs_node	*pn_res;
	struct stat		 st;
	struct fuse		*fuse;
	const char		*path = PCNPATH(pcn);
	int			 ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcn->pcn_cred);

	ret = fuse_fs_getattr_v30(fuse->fs, path, &st, NULL);
	if (ret != 0) {
		return -ret;
	}

	/* XXX: fiXXXme unconst */
	pn_res = puffs_pn_nodewalk(pu, puffs_path_walkcmp,
	    __UNCONST(&pcn->pcn_po_full));
	if (pn_res == NULL) {
		pn_res = newrn(pu);
		if (pn_res == NULL)
			return errno;
		puffs_stat2vattr(&pn_res->pn_va, &st);
	}

	puffs_newinfo_setcookie(pni, pn_res);
	puffs_newinfo_setvtype(pni, pn_res->pn_va.va_type);
	puffs_newinfo_setsize(pni, (voff_t)pn_res->pn_va.va_size);
	puffs_newinfo_setrdev(pni, pn_res->pn_va.va_rdev);

	return 0;
}

/* get attributes for the path name */
/* ARGSUSED3 */
static int
puffs_fuse_node_getattr(struct puffs_usermount *pu, void *opc, struct vattr *va,
	const struct puffs_cred *pcr) 
{
	struct puffs_node	*pn = opc;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcr);

	return fuse_getattr(fuse, pn, path, va);
}

/* read the contents of the symbolic link */
/* ARGSUSED2 */
static int
puffs_fuse_node_readlink(struct puffs_usermount *pu, void *opc,
	const struct puffs_cred *cred, char *linkname, size_t *linklen)
{
	struct puffs_node	*pn = opc;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn), *p;
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(cred);

	/* wrap up return code */
	ret = fuse_fs_readlink(fuse->fs, path, linkname, *linklen);

	if (ret == 0) {
		p = memchr(linkname, '\0', *linklen);
		if (!p)
			return EINVAL;

		*linklen = (size_t)(p - linkname);
	}

	return -ret;
}

/* make the special node */
/* ARGSUSED1 */
static int
puffs_fuse_node_mknod(struct puffs_usermount *pu, void *opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn,
	const struct vattr *va)
{
	struct fuse		*fuse;
	mode_t			 mode;
	const char		*path = PCNPATH(pcn);
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcn->pcn_cred);

	/* wrap up return code */
	mode = puffs_addvtype2mode(va->va_mode, va->va_type);
	ret = fuse_fs_mknod(fuse->fs, path, mode, va->va_rdev);

	if (ret == 0) {
		ret = fuse_newnode(pu, path, va, NULL, pni, NULL);
	}

	return -ret;
}

/* make a directory */
/* ARGSUSED1 */
static int
puffs_fuse_node_mkdir(struct puffs_usermount *pu, void *opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn,
	const struct vattr *va)
{
	struct fuse		*fuse;
	mode_t			 mode = va->va_mode;
	const char		*path = PCNPATH(pcn);
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcn->pcn_cred);

	/* wrap up return code */
	ret = fuse_fs_mkdir(fuse->fs, path, mode);

	if (ret == 0) {
		ret = fuse_newnode(pu, path, va, NULL, pni, NULL);
	}

	return -ret;
}

/*
 * create a regular file
 *
 * since linux/fuse sports using mknod for creating regular files
 * instead of having a separate call for it in some versions, if
 * we don't have create, just jump to op->mknod.
 */
/*ARGSUSED1*/
static int
puffs_fuse_node_create(struct puffs_usermount *pu, void *opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn,
	const struct vattr *va)
{
	struct fuse		*fuse;
	struct fuse_file_info	fi;
	struct puffs_node	*pn;
	mode_t			mode = va->va_mode;
	const char		*path = PCNPATH(pcn);
	int			ret, created;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcn->pcn_cred);

	memset(&fi, 0, sizeof(fi));
	/* In puffs "create" and "open" are two separate operations
	 * with atomicity achieved by locking the parent vnode. In
	 * fuse, on the other hand, "create" is actually a
	 * create-and-open-atomically and the open flags (O_RDWR,
	 * O_APPEND, ...) are passed via fi.flags. So the only way to
	 * emulate the fuse semantics is to open the file with dummy
	 * flags and then immediately close it.
	 *
	 * You might think that we could simply use fuse->op.mknod all
	 * the time but no, that's not possible because most file
	 * systems nowadays expect op.mknod to be called only for
	 * non-regular files and many don't even support it. */
	created = 0;
	fi.flags = O_WRONLY | O_CREAT | O_EXCL;
	ret = fuse_fs_create(fuse->fs, path, mode | S_IFREG, &fi);
	if (ret == 0) {
		created = 1;
	}
	else if (ret == -ENOSYS) {
		ret = fuse_fs_mknod(fuse->fs, path, mode | S_IFREG, 0);
	}

	if (ret == 0) {
		ret = fuse_newnode(pu, path, va, &fi, pni, &pn);

		/* sweet..  create also open the file */
		if (created) {
			struct refusenode *rn = pn->pn_data;
			/* The return value of op.release is expected to be
			 * discarded. */
			(void)fuse_fs_release(fuse->fs, path, &rn->file_info);
		}
	}

	return -ret;
}

/* remove the directory entry */
/* ARGSUSED1 */
static int
puffs_fuse_node_remove(struct puffs_usermount *pu, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node	*pn_targ = targ;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn_targ);
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcn->pcn_cred);

	/* wrap up return code */
	ret = fuse_fs_unlink(fuse->fs, path);

	return -ret;
}

/* remove the directory */
/* ARGSUSED1 */
static int
puffs_fuse_node_rmdir(struct puffs_usermount *pu, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node	*pn_targ = targ;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn_targ);
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcn->pcn_cred);

	/* wrap up return code */
	ret = fuse_fs_rmdir(fuse->fs, path);

	return -ret;
}

/* create a symbolic link */
/* ARGSUSED1 */
static int
puffs_fuse_node_symlink(struct puffs_usermount *pu, void *opc,
	struct puffs_newinfo *pni, const struct puffs_cn *pcn_src,
	const struct vattr *va, const char *link_target)
{
	struct fuse		*fuse;
	const char		*path = PCNPATH(pcn_src);
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcn_src->pcn_cred);

	/* wrap up return code */
	ret = fuse_fs_symlink(fuse->fs, link_target, path);

	if (ret == 0) {
		ret = fuse_newnode(pu, path, va, NULL, pni, NULL);
	}

	return -ret;
}

/* rename a directory entry */
/* ARGSUSED1 */
static int
puffs_fuse_node_rename(struct puffs_usermount *pu, void *opc, void *src,
	const struct puffs_cn *pcn_src, void *targ_dir, void *targ,
	const struct puffs_cn *pcn_targ)
{
	struct fuse		*fuse;
	const char		*path_src = PCNPATH(pcn_src);
	const char		*path_dest = PCNPATH(pcn_targ);
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcn_targ->pcn_cred);

	ret = fuse_fs_rename_v30(fuse->fs, path_src, path_dest, 0);

	return -ret;
}

/* create a link in the file system */
/* ARGSUSED1 */
static int
puffs_fuse_node_link(struct puffs_usermount *pu, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_node	*pn = targ;
	struct fuse		*fuse;
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcn->pcn_cred);

	/* wrap up return code */
	ret = fuse_fs_link(fuse->fs, PNPATH(pn), PCNPATH(pcn));

	return -ret;
}

/*
 * fuse's regular interface provides chmod(), chown(), utimes()
 * and truncate() + some variations, so try to fit the square block
 * in the circle hole and the circle block .... something like that
 */
/* ARGSUSED3 */
static int
puffs_fuse_node_setattr(struct puffs_usermount *pu, void *opc,
	const struct vattr *va, const struct puffs_cred *pcr)
{
	struct puffs_node	*pn = opc;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcr);

	return fuse_setattr(fuse, pn, path, va);
}

static int
puffs_fuse_node_pathconf(struct puffs_usermount *pu, void *opc,
	int name, __register_t *retval)
{
	/* Returning EINVAL for pathconf(2) means that this filesystem
	 * does not support an association of the given name with the
	 * file. This is necessary because the default error code
	 * returned by the puffs kernel module (ENOTSUPP) is not
	 * suitable for an errno from pathconf(2), and "ls -l"
	 * complains about it. */
	return EINVAL;
}

/* ARGSUSED2 */
static int
puffs_fuse_node_open(struct puffs_usermount *pu, void *opc, int mode,
	const struct puffs_cred *cred)
{
	struct puffs_node	*pn = opc;
	struct refusenode	*rn = pn->pn_data;
	struct fuse_file_info	*fi = &rn->file_info;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(cred);

	/* if open, don't open again, lest risk nuking file private info */
	if (rn->flags & RN_OPEN) {
		rn->opencount++;
		return 0;
	}

	/* OFLAGS(), need to convert FREAD/FWRITE to O_RD/WR */
	fi->flags = (mode & ~(O_CREAT | O_EXCL | O_TRUNC)) - 1;

	if (pn->pn_va.va_type == VDIR) {
		ret = fuse_fs_opendir(fuse->fs, path, fi);
	} else {
		ret = fuse_fs_open(fuse->fs, path, fi);
	}

	if (ret == 0) {
		rn->flags |= RN_OPEN;
		rn->opencount++;
	}

	return -ret;
}

/* ARGSUSED2 */
static int
puffs_fuse_node_close(struct puffs_usermount *pu, void *opc, int fflag,
	const struct puffs_cred *pcr)
{
	struct puffs_node	*pn = opc;
	struct refusenode	*rn = pn->pn_data;
	struct fuse		*fuse;
	struct fuse_file_info	*fi;
	const char		*path = PNPATH(pn);
	int			ret;

	fuse = puffs_getspecific(pu);
	fi = &rn->file_info;
	ret = 0;

	set_fuse_context_uid_gid(pcr);

	if (rn->flags & RN_OPEN) {
		if (pn->pn_va.va_type == VDIR) {
			ret = fuse_fs_releasedir(fuse->fs, path, fi);
		} else {
			ret = fuse_fs_release(fuse->fs, path, fi);
		}
	}
	rn->flags &= ~RN_OPEN;
	rn->opencount--;

	return ret;
}

/* read some more from the file */
/* ARGSUSED5 */
static int
puffs_fuse_node_read(struct puffs_usermount *pu, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr,
	int ioflag)
{
	struct puffs_node	*pn = opc;
	struct refusenode	*rn = pn->pn_data;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);
	size_t			maxread;
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcr);

	maxread = *resid;
	if (maxread > (size_t)((off_t)pn->pn_va.va_size - offset)) {
		/*LINTED*/
		maxread = (size_t)((off_t)pn->pn_va.va_size - offset);
	}
	if (maxread == 0)
		return 0;

	ret = fuse_fs_read(fuse->fs, path, (char *)buf, maxread, offset,
			   &rn->file_info);

	if (ret > 0) {
		*resid -= (size_t)ret;
		ret = 0;
	}

	return -ret;
}

/* write to the file */
/* ARGSUSED0 */
static int
puffs_fuse_node_write(struct puffs_usermount *pu, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr,
	int ioflag)
{
	struct puffs_node	*pn = opc;
	struct refusenode	*rn = pn->pn_data;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcr);

	if (ioflag & PUFFS_IO_APPEND)
		offset = (off_t)pn->pn_va.va_size;

	ret = fuse_fs_write(fuse->fs, path, (char *)buf, *resid, offset,
			    &rn->file_info);

	if (ret >= 0) {
		if ((uint64_t)(offset + ret) > pn->pn_va.va_size)
			pn->pn_va.va_size = (u_quad_t)(offset + ret);
		*resid -= (size_t)ret;
		ret = (*resid == 0) ? 0 : ENOSPC;
	} else {
		ret = -ret;
	}

	return ret;
}


/* ARGSUSED3 */
static int
puffs_fuse_node_readdir(struct puffs_usermount *pu, void *opc,
	struct dirent *dent, off_t *readoff, size_t *reslen,
	const struct puffs_cred *pcr, int *eofflag,
	off_t *cookies, size_t *ncookies)
{
	struct puffs_node	*pn = opc;
	struct refusenode	*rn = pn->pn_data;
	struct puffs_fuse_dirh	*dirh;
	struct fuse		*fuse;
	struct dirent		*fromdent;
	const char		*path = PNPATH(pn);
	int			ret;

	fuse = puffs_getspecific(pu);

	set_fuse_context_uid_gid(pcr);

	if (pn->pn_va.va_type != VDIR)
		return ENOTDIR;

	dirh = &rn->dirh;

	/*
	 * if we are starting from the beginning, slurp entire directory
	 * into our buffers
	 */
	if (*readoff == 0) {
		/* free old buffers */
		free(dirh->dbuf);
		memset(dirh, 0, sizeof(struct puffs_fuse_dirh));

		ret = fuse_fs_readdir_v30(
			fuse->fs, path, dirh, puffs_fuse_fill_dir,
			0, &rn->file_info, (enum fuse_readdir_flags)0);

		if (ret)
			return -ret;
	}

        /* Both op.readdir and op.getdir read full directory */
        *eofflag = 1;

	/* now, stuff results into the kernel buffers */
	while (*readoff < (off_t)(dirh->bufsize - dirh->reslen)) {
		/*LINTED*/
		fromdent = (struct dirent *)((uint8_t *)dirh->dbuf + *readoff);

		if (*reslen < _DIRENT_SIZE(fromdent))
			break;

		memcpy(dent, fromdent, _DIRENT_SIZE(fromdent));
		*readoff += (off_t)_DIRENT_SIZE(fromdent);
		*reslen -= _DIRENT_SIZE(fromdent);

		dent = _DIRENT_NEXT(dent);
	}

	return 0;
}

/* ARGSUSED */
static int
puffs_fuse_node_reclaim(struct puffs_usermount *pu, void *opc)
{
	struct puffs_node	*pn = opc;

	nukern(pn);
	return 0;
}

/* ARGSUSED1 */
static int
puffs_fuse_fs_unmount(struct puffs_usermount *pu, int flags)
{
	struct fuse		*fuse;

	fuse = puffs_getspecific(pu);
	fuse_fs_destroy(fuse->fs);
        return 0;
}

/* ARGSUSED0 */
static int
puffs_fuse_fs_sync(struct puffs_usermount *pu, int flags,
            const struct puffs_cred *cr)
{
	set_fuse_context_uid_gid(cr);
        return 0;
}

/* ARGSUSED2 */
static int
puffs_fuse_fs_statvfs(struct puffs_usermount *pu, struct puffs_statvfs *svfsb)
{
	struct fuse		*fuse;
	int			ret;
	struct statvfs		sb;

	/* fuse_fs_statfs() is special: it returns 0 even if the
	 * filesystem doesn't support statfs. So clear the struct
	 * before calling it. */
	memset(&sb, 0, sizeof(sb));

	fuse = puffs_getspecific(pu);
	ret = fuse_fs_statfs(fuse->fs, PNPATH(puffs_getroot(pu)), &sb);

	if (ret == 0)
		statvfs_to_puffs_statvfs(&sb, svfsb);

        return -ret;
}

/* End of puffs_fuse operations */

struct fuse *
__fuse_setup(int argc, char* argv[],
	     const void* op, int op_version, void* user_data,
	     struct fuse_cmdline_opts* opts)
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse *fuse = NULL;

	/* parse low-level options */
	if (fuse_parse_cmdline_v30(&args, opts) != 0)
		return NULL;

	if (opts->show_version) {
		fuse_lowlevel_version();
		goto free_args;
	}

	if (opts->show_help) {
		switch (opts->show_help) {
		case REFUSE_SHOW_HELP_FULL:
			if (args.argv[0] != NULL && args.argv[0][0] != '\0') {
				/* argv[0] being empty means that the application doesn't
				 * want us to print the usage string.
				 */
				printf("Usage: %s [options] mountpoint\n\n", args.argv[0]);
			}
			break;
		case REFUSE_SHOW_HELP_NO_HEADER:
			break;
		}
		fuse_cmdline_help();
		goto free_args;
	}

	if (opts->mountpoint == NULL) {
		fprintf(stderr, "fuse: no mountpoint specified\n");
		goto free_args;
	}

	if (opts->debug) {
		if (fuse_opt_add_arg(&args, "-odebug") != 0)
			goto free_args;
	}

	fuse = __fuse_new(&args, op, op_version, user_data);
	if (fuse == NULL)
		goto free_args;

	if (fuse_daemonize(opts->foreground) != 0)
		goto destroy;

	if (fuse_mount_v30(fuse, opts->mountpoint) != 0)
		goto destroy;

	if (__fuse_set_signal_handlers(fuse) != 0) {
		warn("%s: Failed to set signal handlers", __func__);
		goto destroy;
	}

	goto done;

destroy:
	fuse_destroy_v30(fuse);
	fuse = NULL;
free_args:
	free(opts->mountpoint);
done:
	fuse_opt_free_args(&args);
	return fuse;
}

void
__fuse_teardown(struct fuse* fuse)
{
	if (__fuse_remove_signal_handlers(fuse) != 0)
		warn("%s: Failed to restore signal handlers", __func__);

	fuse_unmount_v30(fuse);
}

/* ARGSUSED3 */
int
__fuse_main(int argc, char **argv, const void *op,
	int op_version, void *user_data)
{
	struct fuse_cmdline_opts opts;
	struct fuse *fuse;
	int rv;

	fuse = __fuse_setup(argc, argv, op, op_version, user_data, &opts);
	if (fuse == NULL)
		return -1;

	rv = fuse_loop(fuse);

	__fuse_teardown(fuse);

	free(opts.mountpoint);
	return rv;
}

int __fuse_mount(struct fuse *fuse, const char *mountpoint)
{
	struct puffs_pathobj	*po_root;
	struct puffs_node	*pn_root;
	struct refusenode	*rn_root;
	struct puffs_statvfs	 svfsb;

	pn_root = newrn(fuse->pu);
	puffs_setroot(fuse->pu, pn_root);
	rn_root = pn_root->pn_data;
	rn_root->flags |= RN_ROOT;

	po_root = puffs_getrootpathobj(fuse->pu);
	if ((po_root->po_path = strdup("/")) == NULL)
		err(1, "fuse_mount");
	po_root->po_len = 1;
	puffs_path_buildhash(fuse->pu, po_root);

	/* sane defaults */
	puffs_vattr_null(&pn_root->pn_va);
	pn_root->pn_va.va_type = VDIR;
	pn_root->pn_va.va_mode = 0755;
	/* It might be tempting to call op.getattr("/") here to
	 * populate pn_root->pa_va, but that would mean invoking an
	 * operation callback without initializing the filesystem. We
	 * cannot call op.init() either, because that is supposed to
	 * be called right before entering the main loop. */

	puffs_set_prepost(fuse->pu, set_fuse_context_pid, NULL);

	puffs_zerostatvfs(&svfsb);
	if (puffs_mount(fuse->pu, mountpoint, MNT_NODEV | MNT_NOSUID, pn_root) == -1) {
		err(EXIT_FAILURE, "puffs_mount: directory \"%s\"", mountpoint);
	}

	return 0;
}

int fuse_daemonize(int foreground)
{
	/* There is an impedance mismatch here: FUSE wants to
	 * daemonize the process without any contexts but puffs wants
	 * one. */
	struct fuse *fuse = fuse_get_context()->fuse;

	if (!fuse)
		/* FUSE would probably allow this, but we cannot. */
		errx(EXIT_FAILURE,
		     "%s: librefuse doesn't allow calling"
		     " this function before fuse_new().", __func__);

	if (!foreground)
		return puffs_daemon(fuse->pu, 0, 0);

	return 0;
}

struct fuse *
__fuse_new(struct fuse_args *args, const void *op, int op_version, void* user_data)
{
	struct refuse_config	config;
	struct puffs_usermount	*pu;
	struct fuse_context	*fusectx;
	struct puffs_ops	*pops;
	struct fuse		*fuse;
	uint32_t		puffs_flags;

	/* parse refuse options */
	memset(&config, 0, sizeof(config));
	if (fuse_opt_parse(args, &config, refuse_opts, NULL) == -1)
		return NULL;

	if ((fuse = calloc(1, sizeof(*fuse))) == NULL) {
		err(EXIT_FAILURE, "fuse_new");
	}

	/* grab the pthread context key */
	if (!create_context_key()) {
		free(config.fsname);
		free(fuse);
		return NULL;
	}

	/* Create the base filesystem layer. */
	fuse->fs = __fuse_fs_new(op, op_version, user_data);

	fusectx = fuse_get_context();
	fusectx->fuse = fuse;
	fusectx->uid = 0;
	fusectx->gid = 0;
	fusectx->pid = 0;
	fusectx->private_data = user_data;

	/* initialise the puffs operations structure */
        PUFFSOP_INIT(pops);

        PUFFSOP_SET(pops, puffs_fuse, fs, sync);
        PUFFSOP_SET(pops, puffs_fuse, fs, statvfs);
        PUFFSOP_SET(pops, puffs_fuse, fs, unmount);

	/*
	 * XXX: all of these don't possibly need to be
	 * unconditionally set
	 */
        PUFFSOP_SET(pops, puffs_fuse, node, lookup);
        PUFFSOP_SET(pops, puffs_fuse, node, getattr);
        PUFFSOP_SET(pops, puffs_fuse, node, setattr);
	PUFFSOP_SET(pops, puffs_fuse, node, pathconf);
        PUFFSOP_SET(pops, puffs_fuse, node, readdir);
        PUFFSOP_SET(pops, puffs_fuse, node, readlink);
        PUFFSOP_SET(pops, puffs_fuse, node, mknod);
        PUFFSOP_SET(pops, puffs_fuse, node, create);
        PUFFSOP_SET(pops, puffs_fuse, node, remove);
        PUFFSOP_SET(pops, puffs_fuse, node, mkdir);
        PUFFSOP_SET(pops, puffs_fuse, node, rmdir);
        PUFFSOP_SET(pops, puffs_fuse, node, symlink);
        PUFFSOP_SET(pops, puffs_fuse, node, rename);
        PUFFSOP_SET(pops, puffs_fuse, node, link);
        PUFFSOP_SET(pops, puffs_fuse, node, open);
        PUFFSOP_SET(pops, puffs_fuse, node, close);
        PUFFSOP_SET(pops, puffs_fuse, node, read);
        PUFFSOP_SET(pops, puffs_fuse, node, write);
        PUFFSOP_SET(pops, puffs_fuse, node, reclaim);

	puffs_flags = PUFFS_FLAG_BUILDPATH
		| PUFFS_FLAG_HASHPATH
		| PUFFS_KFLAG_NOCACHE;
	if (config.debug)
		puffs_flags |= PUFFS_FLAG_OPDUMP;

	pu = puffs_init(pops, _PATH_PUFFS, config.fsname, fuse, puffs_flags);
	if (pu == NULL) {
		err(EXIT_FAILURE, "puffs_init");
	}
	fuse->pu = pu;

	free(config.fsname);
	return fuse;
}

int
fuse_loop(struct fuse *fuse)
{
	struct fuse_conn_info conn;
	struct fuse_config cfg;

	/* struct fuse_conn_info is a part of the FUSE API so we must
	 * expose it to users, but we currently don't use them at
	 * all. The same goes for struct fuse_config. */
	memset(&conn, 0, sizeof(conn));
	memset(&cfg, 0, sizeof(cfg));

	fuse_fs_init_v30(fuse->fs, &conn, &cfg);

	return puffs_mainloop(fuse->pu);
}

int
__fuse_loop_mt(struct fuse *fuse,
	       struct fuse_loop_config *config __attribute__((__unused__)))
{
	/* TODO: Implement a proper multi-threaded loop. */
	return fuse_loop(fuse);
}

void
__fuse_destroy(struct fuse *fuse)
{

	/*
	 * TODO: needs to assert the fs is quiescent, i.e. no other
	 * threads exist
	 */

	clear_context();
	delete_context_key();
	/* XXXXXX: missing stuff */
	free(fuse);
}

void
fuse_exit(struct fuse *fuse)
{
	/* XXX: puffs_exit() is WRONG */
	if (fuse->dead == 0)
		puffs_exit(fuse->pu, 1);
	fuse->dead = 1;
}

/*
 * XXX: obviously not the most perfect of functions, but needs some
 * puffs tweaking for a better tomorrow
 */
void
__fuse_unmount(struct fuse *fuse)
{
	/* XXX: puffs_exit() is WRONG */
	if (fuse->dead == 0)
		puffs_exit(fuse->pu, 1);
	fuse->dead = 1;
}

void
fuse_lib_help(struct fuse_args *args __attribute__((__unused__)))
{
	fuse_cmdline_help();
}

int
fuse_interrupted(void)
{
	/* ReFUSE doesn't support request interruption at the
	 * moment. */
	return 0;
}

int
fuse_invalidate_path(struct fuse *fuse __attribute__((__unused__)),
		     const char *path __attribute__((__unused__)))
{
    /* ReFUSE doesn't cache anything at the moment. No need to do
     * anything. */
    return -ENOENT;
}

int
fuse_version(void)
{
	return _REFUSE_VERSION_;
}

const char *
fuse_pkgversion(void)
{
	return "ReFUSE " ___STRING(_REFUSE_MAJOR_VERSION_)
		"." ___STRING(_REFUSE_MINOR_VERSION_);
}

int
fuse_getgroups(int size, gid_t list[])
{
	/* XXX: In order to implement this, we need to save a pointer
	 * to struct puffs_cred in struct fuse upon entering a puffs
	 * callback, and set it back to NULL upon leaving it. Then we
	 * can use puffs_cred_getgroups(3) here. */
	return -ENOSYS;
}

int
fuse_start_cleanup_thread(struct fuse *fuse)
{
	/* XXX: ReFUSE doesn't support -oremember at the moment. */
	return 0;
}

void
fuse_stop_cleanup_thread(struct fuse *fuse) {
	/* XXX: ReFUSE doesn't support -oremember at the moment. */
}

int
fuse_clean_cache(struct fuse *fuse) {
	/* XXX: ReFUSE doesn't support -oremember at the moment. */
	return 3600;
}

/* This is a legacy function that has been removed from the FUSE API,
 * but is defined here because it needs to access refuse_opts. */
int
fuse_is_lib_option(const char *opt)
{
	return fuse_opt_match(refuse_opts, opt);
}
