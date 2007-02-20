/*	$NetBSD: refuse.c,v 1.32 2007/02/20 19:13:28 pooka Exp $	*/

/*
 * Copyright © 2007 Alistair Crooks.  All rights reserved.
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
__RCSID("$NetBSD: refuse.c,v 1.32 2007/02/20 19:13:28 pooka Exp $");
#endif /* !lint */

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fuse.h>
#include <unistd.h>

#include "defs.h"

typedef uint64_t	 fuse_ino_t;

struct fuse_config {
	uid_t		uid;
	gid_t		gid;
	mode_t		umask;
	double		entry_timeout;
	double		negative_timeout;
	double		attr_timeout;
	double		ac_attr_timeout;
	int		ac_attr_timeout_set;
	int		debug;
	int		hard_remove;
	int		use_ino;
	int		readdir_ino;
	int		set_mode;
	int		set_uid;
	int		set_gid;
	int		direct_io;
	int		kernel_cache;
	int		auto_cache;
	int		intr;
	int		intr_signal;
};

/* this is the private fuse structure */
struct fuse {
	struct fuse_session	*se;		/* fuse session pointer */
	struct fuse_operations	op;		/* switch table of operations */
	int			compat;		/* compat level -
						 * not used in puffs_fuse */
	struct node		**name_table;
	size_t			name_table_size;
	struct node		**id_table;
	size_t			id_table_size;
	fuse_ino_t		ctr;
	unsigned int		generation;
	unsigned int		hidectr;
	pthread_mutex_t		lock;
	pthread_rwlock_t	tree_lock;
	void			*user_data;
	struct fuse_config	conf;
	int			intr_installed;
	struct puffs_usermount	*pu;
};

struct refusenode {
	struct fuse_file_info	 file_info;
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

	rn = malloc(sizeof(struct refusenode));
	if (!rn)
		abort(); /*XXX*/

	memset(rn, 0, sizeof(struct refusenode));
	pn = puffs_pn_new(pu, rn);

	return pn;
}

static void
nukern(struct puffs_node *pn)
{

	free(pn->pn_data);
	puffs_pn_put(pn);
}

static ino_t fakeino = 3;

/*
 * XXX: do this otherwise if/when we grow thread support
 *
 * XXX2: does not consistently supply uid, gid or pid currently
 */
static struct fuse_context fcon;


/* XXX: rethinkme */
struct fuse_dirh {
	struct dirent *dent;
	size_t reslen;
	off_t readoff;
};

/* ARGSUSED2 */
static int
puffs_fuse_fill_dir(void *buf, const char *name,
	const struct stat *stbuf, off_t off)
{
	struct fuse_dirh *deh = buf;
	ino_t dino;
	uint8_t dtype;

	if (stbuf == NULL) {
		dtype = DT_UNKNOWN;
		dino = fakeino++;
	} else {
		dtype = puffs_vtype2dt(puffs_mode2vt(stbuf->st_mode));
		dino = stbuf->st_ino;
	}

	return !puffs_nextdent(&deh->dent, name, dino, dtype, &deh->reslen);
}

static int
puffs_fuse_dirfil(fuse_dirh_t h, const char *name, int type, ino_t ino)
{
	ino_t dino;
	int dtype;

	if (type == 0)
		dtype = DT_UNKNOWN;
	else
		dtype = type;

	if (ino)
		dino = ino;
	else
		dino = fakeino++;

	return !puffs_nextdent(&h->dent, name, dino, dtype, &h->reslen);
}

int
fuse_opt_add_arg(struct fuse_args *args, const char *arg)
{
	char	**oldargv;
	int	oldargc;

	if (args->allocated) {
		RENEW(char *, args->argv, args->argc + 1,
		    "fuse_opt_add_arg1", return 0);
	} else {
		oldargv = args->argv;
		oldargc = args->argc;
		NEWARRAY(char *, args->argv, oldargc + 1,
		    "fuse_opt_add_arg2", return 0);
		(void) memcpy(args->argv, oldargv, oldargc * sizeof(char *));
		args->allocated = 1;
	}
	args->argv[args->argc++] = strdup(arg);
	return 1;
}

void
fuse_opt_free_args(struct fuse_args *args)
{
	if (args && args->argv) {
		int i;
		for (i = 0; i < args->argc; i++)
			FREE(args->argv[i]);
		FREE(args->argv);
	}
}

#define FUSE_ERR_UNLINK(fuse, file) if (fuse->op.unlink) fuse->op.unlink(file)
#define FUSE_ERR_RMDIR(fuse, dir) if (fuse->op.rmdir) fuse->op.rmdir(dir)

/* ARGSUSED1 */
static int
fuse_getattr(struct fuse *fuse, struct puffs_node *pn, const char *path,
	struct vattr *va)
{
	struct stat		 st;
	int			ret;

	if (fuse->op.getattr == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.getattr)(path, &st);

	if (ret == 0) {
		puffs_stat2vattr(va, &st);
	}

	return -ret;
}

static int
fuse_setattr(struct fuse *fuse, struct puffs_node *pn, const char *path,
	const struct vattr *va)
{
	struct refusenode	*rn = pn->pn_data;
	mode_t			mode;
	uid_t			uid;
	gid_t			gid;
	int			error, ret;

	error = 0;

	mode = va->va_mode;
	uid = va->va_uid;
	gid = va->va_gid;

	if (mode != (mode_t)PUFFS_VNOVAL) {
		ret = 0;

		if (fuse->op.chmod == NULL) {
			error = -ENOSYS;
		} else {
			ret = fuse->op.chmod(path, mode);
			if (ret)
				error = ret;
		}
	}
	if (uid != (uid_t)PUFFS_VNOVAL || gid != (gid_t)PUFFS_VNOVAL) {
		ret = 0;

		if (fuse->op.chown == NULL) {
			error = -ENOSYS;
		} else {
			ret = fuse->op.chown(path, uid, gid);
			if (ret)
				error = ret;
		}
	}
	if (va->va_atime.tv_sec != (time_t)PUFFS_VNOVAL
	    || va->va_mtime.tv_sec != (long)PUFFS_VNOVAL) {
		ret = 0;

		if (fuse->op.utimens) {
			struct timespec tv[2];

			tv[0].tv_sec = va->va_atime.tv_sec;
			tv[0].tv_nsec = va->va_atime.tv_nsec;
			tv[1].tv_sec = va->va_mtime.tv_sec;
			tv[1].tv_nsec = va->va_mtime.tv_nsec;

			ret = fuse->op.utimens(path, tv);
		} else if (fuse->op.utime) {
			struct utimbuf timbuf;

			timbuf.actime = va->va_atime.tv_sec;
			timbuf.modtime = va->va_mtime.tv_sec;

			ret = fuse->op.utime(path, &timbuf);
		} else {
			error = -ENOSYS;
		}

		if (ret)
			error = ret;
	}
	if (va->va_size != (u_quad_t)PUFFS_VNOVAL) {
		ret = 0;

		if (fuse->op.truncate) {
			ret = fuse->op.truncate(path, (off_t)va->va_size);
		} else if (fuse->op.ftruncate) {
			ret = fuse->op.ftruncate(path, (off_t)va->va_size,
			    &rn->file_info);
		} else {
			error = -ENOSYS;
		}

		if (ret)
			error = ret;
	}
	/* XXX: no reflection with reality */
	puffs_setvattr(&pn->pn_va, va);

	return -error;

}

static int
fuse_newnode(struct puffs_usermount *pu, const char *path,
	const struct vattr *va, struct fuse_file_info *fi, void **newnode)
{
	struct vattr		newva;
	struct fuse		*fuse;
	struct puffs_node	*pn;
	struct refusenode	*rn;

	fuse = (struct fuse *)pu->pu_privdata;

	/* fix up nodes */
	pn = newrn(pu);
	if (pn == NULL) {
		if (va->va_type == VDIR) {
			FUSE_ERR_RMDIR(fuse, path);
		} else {
			FUSE_ERR_UNLINK(fuse, path);
		}
		return ENOMEM;
	}
	fuse_setattr(fuse, pn, path, va);
	if (fuse_getattr(fuse, pn, path, &newva) == 0)
		puffs_setvattr(&pn->pn_va, &newva);

	rn = pn->pn_data;
	if (fi)
		memcpy(&rn->file_info, fi, sizeof(struct fuse_file_info));

	*newnode = pn;

	return 0;
}


/* operation wrappers start here */

/* lookup the path */
/* ARGSUSED1 */
static int
puffs_fuse_node_lookup(struct puffs_cc *pcc, void *opc, void **newnode,
	enum vtype *newtype, voff_t *newsize, dev_t *newrdev,
	const struct puffs_cn *pcn)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn_res;
	struct stat		st;
	struct fuse		*fuse;
	const char		*path = PCNPATH(pcn);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	ret = fuse->op.getattr(path, &st);

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

	*newnode = pn_res;
	*newtype = pn_res->pn_va.va_type;
	*newsize = pn_res->pn_va.va_size;
	*newrdev = pn_res->pn_va.va_rdev;

	return 0;
}

/* get attributes for the path name */
/* ARGSUSED3 */
static int
puffs_fuse_node_getattr(struct puffs_cc *pcc, void *opc, struct vattr *va,
	const struct puffs_cred *pcr, pid_t pid) 
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = opc;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);

	fuse = (struct fuse *)pu->pu_privdata;
	return fuse_getattr(fuse, pn, path, va);
}

/* read the contents of the symbolic link */
/* ARGSUSED2 */
static int
puffs_fuse_node_readlink(struct puffs_cc *pcc, void *opc,
	const struct puffs_cred *cred, char *linkname, size_t *linklen)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = opc;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn), *p;
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.readlink == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.readlink)(path, linkname, *linklen);

	if (ret == 0) {
		p = memchr(linkname, '\0', *linklen);
		if (!p)
			return EINVAL;

		*linklen = p - linkname;
	}

	return -ret;
}

/* make the special node */
/* ARGSUSED1 */
static int
puffs_fuse_node_mknod(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse		*fuse;
	mode_t			 mode = va->va_mode;
	const char		*path = PCNPATH(pcn);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.mknod == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.mknod)(path, mode, va->va_rdev);

	if (ret == 0) {
		ret = fuse_newnode(pu, path, va, NULL, newnode);
	}

	return -ret;
}

/* make a directory */
/* ARGSUSED1 */
static int
puffs_fuse_node_mkdir(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse		*fuse;
	mode_t			 mode = va->va_mode;
	const char		*path = PCNPATH(pcn);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.mkdir == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.mkdir)(path, mode);

	if (ret == 0) {
		ret = fuse_newnode(pu, path, va, NULL, newnode);
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
puffs_fuse_node_create(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse		*fuse;
	struct fuse_file_info	fi;
	mode_t			mode = va->va_mode;
	const char		*path = PCNPATH(pcn);
	int			ret, created;

	fuse = (struct fuse *)pu->pu_privdata;

	created = 0;
	if (fuse->op.create) {
		ret = fuse->op.create(path, mode, &fi);
		if (ret == 0)
			created = 1;

	} else if (fuse->op.mknod) {
		fcon.uid = va->va_uid; /*XXX*/
		fcon.gid = va->va_gid; /*XXX*/

		ret = fuse->op.mknod(path, mode | S_IFREG, 0);

	} else {
		ret = -ENOSYS;
	}

	if (ret == 0) {
		ret = fuse_newnode(pu, path, va, &fi, newnode);

		/* sweet..  create also open the file */
		if (created) {
			struct puffs_node *pn;
			struct refusenode *rn;

			pn = *newnode;
			rn = pn->pn_data;
			rn->flags |= RN_OPEN;
			rn->opencount++;
		}
	}

	return -ret;
}

/* remove the directory entry */
/* ARGSUSED1 */
static int
puffs_fuse_node_remove(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn_targ = targ;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn_targ);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.unlink == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.unlink)(path);

	return -ret;
}

/* remove the directory */
/* ARGSUSED1 */
static int
puffs_fuse_node_rmdir(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn_targ = targ;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn_targ);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.rmdir == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.rmdir)(path);

	return -ret;
}

/* create a symbolic link */
/* ARGSUSED1 */
static int
puffs_fuse_node_symlink(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn_src, const struct vattr *va,
	const char *link_target)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse		*fuse;
	const char		*path = PCNPATH(pcn_src);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.symlink == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = fuse->op.symlink(link_target, path);

	if (ret == 0) {
		ret = fuse_newnode(pu, path, va, NULL, newnode);
	}

	return -ret;
}

/* rename a directory entry */
/* ARGSUSED1 */
static int
puffs_fuse_node_rename(struct puffs_cc *pcc, void *opc, void *src,
	const struct puffs_cn *pcn_src, void *targ_dir, void *targ,
	const struct puffs_cn *pcn_targ)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse		*fuse;
	const char		*path_src = PCNPATH(pcn_src);
	const char		*path_dest = PCNPATH(pcn_targ);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.rename == NULL) {
		return ENOSYS;
	}

	ret = fuse->op.rename(path_src, path_dest);

	if (ret == 0) {
	}

	return -ret;
}

/* create a link in the file system */
/* ARGSUSED1 */
static int
puffs_fuse_node_link(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = targ;
	struct fuse		*fuse;
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.link == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.link)(PNPATH(pn), PCNPATH(pcn));

	return -ret;
}

/*
 * fuse's regular interface provides chmod(), chown(), utimes()
 * and truncate() + some variations, so try to fit the square block
 * in the circle hole and the circle block .... something like that
 */
/* ARGSUSED3 */
static int
puffs_fuse_node_setattr(struct puffs_cc *pcc, void *opc,
	const struct vattr *va, const struct puffs_cred *pcr, pid_t pid)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = opc;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);

	fuse = (struct fuse *)pu->pu_privdata;

	return fuse_setattr(fuse, pn, path, va);
}

/* ARGSUSED2 */
static int
puffs_fuse_node_open(struct puffs_cc *pcc, void *opc, int mode,
	const struct puffs_cred *cred, pid_t pid)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = opc;
	struct refusenode	*rn = pn->pn_data;
	struct fuse_file_info	*fi = &rn->file_info;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);
	int			 ret;

	fuse = (struct fuse *)pu->pu_privdata;

	/* if open, don't open again, lest risk nuking file private info */
	if (rn->flags & RN_OPEN) {
		rn->opencount++;
		return 0;
	}

	fi->flags = mode & ~(O_CREAT | O_EXCL | O_TRUNC);
	if (pn->pn_va.va_type == VDIR) {
		if (fuse->op.opendir)
			ret = fuse->op.opendir(path, fi);
		else
			ret = ENOSYS;
	} else {
		if (fuse->op.open)
			ret = fuse->op.open(path, fi);
		else
			ret = ENOSYS;
	}

	if (ret == 0) {
		rn->flags |= RN_OPEN;
		rn->opencount++;
	}

	return -ret;
}

/* ARGSUSED2 */
static int
puffs_fuse_node_close(struct puffs_cc *pcc, void *opc, int fflag,
	const struct puffs_cred *pcr, pid_t pid)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = opc;
	struct refusenode	*rn = pn->pn_data;
	struct fuse		*fuse;
	struct fuse_file_info	*fi;
	const char		*path = PNPATH(pn);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	fi = &rn->file_info;
	ret = 0;

	if (rn->flags & RN_OPEN) {
		if (pn->pn_va.va_type == VDIR) {
			if (fuse->op.releasedir)
				ret = fuse->op.releasedir(path, fi);
		} else {
			if (fuse->op.release)
				ret = fuse->op.release(path, fi);
		}
	}
	rn->flags &= ~RN_OPEN;
	rn->opencount--;
	printf("opencount- %d\n", rn->opencount);

	return ret;
}

/* read some more from the file */
/* ARGSUSED5 */
static int
puffs_fuse_node_read(struct puffs_cc *pcc, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr,
	int ioflag)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = opc;
	struct refusenode	*rn = pn->pn_data;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);
	size_t			maxread;
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.read == NULL) {
		return ENOSYS;
	}

	maxread = *resid;
	if (maxread > pn->pn_va.va_size - offset) {
		/*LINTED*/
		maxread = pn->pn_va.va_size - offset;
	}
	if (maxread == 0)
		return 0;

	ret = (*fuse->op.read)(path, (char *)buf, maxread, offset,
	    &rn->file_info);

	if (ret > 0) {
		*resid -= ret;
		ret = 0;
	}

	return -ret;
}

/* write to the file */
/* ARGSUSED0 */
static int
puffs_fuse_node_write(struct puffs_cc *pcc, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr,
	int ioflag)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = opc;
	struct refusenode	*rn = pn->pn_data;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.write == NULL) {
		return ENOSYS;
	}

	if (ioflag & PUFFS_IO_APPEND)
		offset = pn->pn_va.va_size;

	ret = (*fuse->op.write)(path, (char *)buf, *resid, offset,
	    &rn->file_info);

	if (ret > 0) {
		if (offset + ret > pn->pn_va.va_size)
			pn->pn_va.va_size = offset + ret;
		*resid -= ret;
		ret = 0;
	}

	return -ret;
}


/* ARGSUSED3 */
static int
puffs_fuse_node_readdir(struct puffs_cc *pcc, void *opc,
	struct dirent *dent, const struct puffs_cred *pcr, off_t *readoff,
	size_t *reslen)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = opc;
	struct refusenode	*rn = pn->pn_data;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);
	struct fuse_dirh	deh;
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.readdir == NULL && fuse->op.getdir == NULL) {
		return ENOSYS;
	}

	/* XXX: how to handle this??? */
	if (*readoff != 0) {
		return 0;
	}

	deh.dent = dent;
	deh.reslen = *reslen;
	deh.readoff = *readoff;

	if (fuse->op.readdir)
		ret = fuse->op.readdir(path, &deh, puffs_fuse_fill_dir,
		    *readoff, &rn->file_info);
	else
		ret = fuse->op.getdir(path, &deh, puffs_fuse_dirfil);
	*reslen = deh.reslen;
	*readoff = 1;

	if (ret == 0) {
	}

	return -ret;
}

/* ARGSUSED */
static int
puffs_fuse_node_reclaim(struct puffs_cc *pcc, void *opc, pid_t pid)
{
	struct puffs_node	*pn = opc;

	nukern(pn);

	return 0;
}

/* ARGSUSED1 */
static int
puffs_fuse_fs_unmount(struct puffs_cc *pcc, int flags, pid_t pid)
{
        struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse		*fuse;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.destroy == NULL) {
		return 0;
	}
	(*fuse->op.destroy)(fuse);
        return 0;
}

/* ARGSUSED0 */
static int
puffs_fuse_fs_sync(struct puffs_cc *pcc, int flags,
            const struct puffs_cred *cr, pid_t pid)
{
        return 0;
}

/* ARGSUSED2 */
static int
puffs_fuse_fs_statvfs(struct puffs_cc *pcc, struct statvfs *svfsb, pid_t pid)
{
        struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse		*fuse;
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.statfs == NULL) {
		if ((ret = statvfs(PNPATH(pu->pu_pn_root), svfsb)) == -1) {
			return errno;
		}
	} else {
		ret = (*fuse->op.statfs)(PNPATH(pu->pu_pn_root), svfsb);
	}

        return ret;
}


/* End of puffs_fuse operations */

/* ARGSUSED3 */
int
fuse_main_real(int argc, char **argv, const struct fuse_operations *ops,
	size_t size, void *userdata)
{
	struct puffs_usermount	*pu;
	struct puffs_pathobj	*po_root;
	struct puffs_ops	*pops;
	struct refusenode	*rn_root;
	struct statvfs		svfsb;
	struct stat		st;
	struct fuse		*fuse;
	char			 name[64];
	char			*slash;
	int			 ret;

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

	NEW(struct fuse, fuse, "fuse_main_real", exit(EXIT_FAILURE));

	/* copy fuse ops to their own stucture */
	(void) memcpy(&fuse->op, ops, sizeof(fuse->op));

	fcon.fuse = fuse;
	fcon.private_data = userdata;

	/* whilst this (assigning the pu_privdata in the puffs
	 * usermount struct to be the fuse struct) might seem like
	 * we are chasing our tail here, the logic is as follows:
		+ the operation wrapper gets called with the puffs
		  calling conventions
		+ we need to fix up args first
		+ then call the fuse user-supplied operation
		+ then we fix up any values on return that we need to
		+ and fix up any nodes, etc
	 * so we need to be able to get at the fuse ops from within the
	 * puffs_usermount struct
	 */
	if ((slash = strrchr(*argv, '/')) == NULL) {
		slash = *argv;
	} else {
		slash += 1;
	}
	(void) snprintf(name, sizeof(name), "refuse:%s", slash);
	pu = puffs_mount(pops, argv[argc - 1], MNT_NODEV | MNT_NOSUID,
			name, fuse,
			PUFFS_FLAG_BUILDPATH
			  | PUFFS_FLAG_OPDUMP
			  | PUFFS_KFLAG_NOCACHE,
			0);
	if (pu == NULL) {
		err(EXIT_FAILURE, "puffs_mount");
	}

	fuse->pu = pu;
	pu->pu_pn_root = newrn(pu);
	rn_root = pu->pu_pn_root->pn_data;
	rn_root->flags |= RN_ROOT;

	po_root = puffs_getrootpathobj(pu);
	po_root->po_path = strdup("/");
	po_root->po_len = 1;

	/* sane defaults */
	puffs_vattr_null(&pu->pu_pn_root->pn_va);
	pu->pu_pn_root->pn_va.va_type = VDIR;
	pu->pu_pn_root->pn_va.va_mode = 0755;
	if (fuse->op.getattr)
		if (fuse->op.getattr(po_root->po_path, &st) == 0)
			puffs_stat2vattr(&pu->pu_pn_root->pn_va, &st);
	assert(pu->pu_pn_root->pn_va.va_type == VDIR);

	if (fuse->op.init)
		fcon.private_data = fuse->op.init(NULL); /* XXX */

	statvfs(argv[argc - 1], &svfsb); /* XXX - not really the correct dir */
	if (puffs_start(pu, pu->pu_pn_root, &svfsb) == -1) {
		err(EXIT_FAILURE, "puffs_start");
	}

	ret = puffs_mainloop(fuse->pu, PUFFSLOOP_NODAEMON);

	(void) free(po_root->po_path);
	FREE(fuse);
	return ret;
}

/* ARGSUSED0 */
int
fuse_opt_parse(struct fuse_args *args, void *data,
	const struct fuse_opt *opts, fuse_opt_proc_t proc)
{
	return 0;
}

/* XXX: threads */
struct fuse_context *
fuse_get_context()
{

	return &fcon;
}

void
fuse_exit(struct fuse *f)
{
	
	puffs_exit(f->pu, 1);
}

/*
 * XXX: obviously not the most perfect of functions, but needs some
 * puffs tweaking for a better tomorrow
 */
/*ARGSUSED*/
void
fuse_unmount(const char *mp)
{

	return;
}
