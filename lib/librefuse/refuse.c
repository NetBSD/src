/*	$NetBSD: refuse.c,v 1.51 2007/05/03 21:02:54 agc Exp $	*/

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
__RCSID("$NetBSD: refuse.c,v 1.51 2007/05/03 21:02:54 agc Exp $");
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

struct fuse_chan {
	const char		*dir;
	struct fuse_args	*args;
	struct puffs_usermount	*pu;
};

/* this is the private fuse structure */
struct fuse {
	struct fuse_chan	*fc;		/* fuse channel pointer */
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

	NEW(struct refusenode, rn, "newrn", exit(EXIT_FAILURE));
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

static ino_t fakeino = 3;

/*
 * XXX: do this otherwise if/when we grow thread support
 *
 * XXX2: does not consistently supply uid, gid or pid currently
 */
static struct fuse_context fcon;

#define DIR_CHUNKSIZE 4096
static int
fill_dirbuf(struct puffs_fuse_dirh *dh, const char *name, ino_t dino,
	uint8_t dtype)
{

	/* initial? */
	if (dh->bufsize == 0) {
		dh->dbuf = malloc(DIR_CHUNKSIZE);
		if (dh->dbuf == NULL)
			err(1, "fill_dirbuf");
		dh->d = dh->dbuf;
		dh->reslen = dh->bufsize = DIR_CHUNKSIZE;
	}

	if (puffs_nextdent(&dh->d, name, dino, dtype, &dh->reslen))
		return 0;

	/* try to increase buffer space */
	dh->dbuf = realloc(dh->dbuf, dh->bufsize + DIR_CHUNKSIZE);
	if (dh->dbuf == NULL)
		err(1, "fill_dirbuf realloc");
	dh->d = (void *)((uint8_t *)dh->dbuf + (dh->bufsize - dh->reslen));
	dh->reslen += DIR_CHUNKSIZE;
	dh->bufsize += DIR_CHUNKSIZE;

	return !puffs_nextdent(&dh->d, name, dino, dtype, &dh->reslen);
}

/* ARGSUSED3 */
/* XXX: I have no idea how "off" is supposed to be used */
static int
puffs_fuse_fill_dir(void *buf, const char *name,
	const struct stat *stbuf, off_t off)
{
	struct puffs_fuse_dirh *deh = buf;
	ino_t dino;
	uint8_t dtype;

	if (stbuf == NULL) {
		dtype = DT_UNKNOWN;
		dino = fakeino++;
	} else {
		dtype = puffs_vtype2dt(puffs_mode2vt(stbuf->st_mode));
		dino = stbuf->st_ino;

		/*
		 * Some FUSE file systems like to always use 0 as the
		 * inode number.   Our readdir() doesn't like to show
		 * directory entries with inode number 0 ==> workaround.
		 */
		if (dino == 0)
			dino = fakeino++;
	}

	return fill_dirbuf(deh, name, dino, dtype);
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

	return fill_dirbuf(h, name, dino, dtype);
}

static struct fuse_args *
deep_copy_args(int argc, char **argv)
{
	struct fuse_args	*ap;
	int			 i;

	NEW(struct fuse_args, ap, "deep_copy_args", return NULL);
	/* deep copy args structure into channel args */
	ap->allocated = ((argc / 10) + 1) * 10;
	NEWARRAY(char *, ap->argv, ap->allocated, "deep_copy_args",
		return NULL);
	for (i = 0 ; i < argc ; i++) {
		ap->argv[i] = strdup(argv[i]);
	}
	return ap;
}

static void
free_args(struct fuse_args *ap)
{
	int	i;

	for (i = 0 ; i < ap->argc ; i++) {
		free(ap->argv[i]);
	}
	free(ap);
}

/* this function exposes struct fuse to userland */
struct fuse *
fuse_setup(int argc, char **argv, const struct fuse_operations *ops,
	size_t size, char **mountpoint, int *multithreaded, int *fd)
{
	struct fuse_chan	*fc;
	struct fuse_args	*args;
	struct fuse		*fuse;
	char			 name[64];
	char			*slash;

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
	if (argv == NULL || *argv == NULL) {
		(void) strlcpy(name, "refuse", sizeof(name));
	} else {
		if ((slash = strrchr(*argv, '/')) == NULL) {
			slash = *argv;
		} else {
			slash += 1;
		}
		(void) snprintf(name, sizeof(name), "refuse:%s", slash);
	}

	/* stuff name into fuse_args */
	args = deep_copy_args(argc, argv);
	if (args->argc > 0) {
		FREE(args->argv[0]);
	}
	args->argv[0] = strdup(name);

	fc = fuse_mount(*mountpoint = argv[argc - 1], args);
	fuse = fuse_new(fc, args, ops, size, NULL);

	free_args(args);

	/* XXX - wait for puffs to become multi-threaded */
	if (multithreaded) {
		*multithreaded = 0;
	}

	/* XXX - this is unused */
	if (fd) {
		*fd = 0;
	}

	return fuse;
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

	fuse = puffs_getspecific(pu);

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

	fuse = puffs_getspecific(pu);
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

	fuse = puffs_getspecific(pu);
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

	fuse = puffs_getspecific(pu);
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
	mode_t			 mode;
	const char		*path = PCNPATH(pcn);
	int			ret;

	fuse = puffs_getspecific(pu);
	if (fuse->op.mknod == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	mode = puffs_addvtype2mode(va->va_mode, va->va_type);
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

	fuse = puffs_getspecific(pu);
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

	fuse = puffs_getspecific(pu);

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

	fuse = puffs_getspecific(pu);
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

	fuse = puffs_getspecific(pu);
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

	fuse = puffs_getspecific(pu);
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

	fuse = puffs_getspecific(pu);
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

	fuse = puffs_getspecific(pu);
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

	fuse = puffs_getspecific(pu);

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

	fuse = puffs_getspecific(pu);

	/* if open, don't open again, lest risk nuking file private info */
	if (rn->flags & RN_OPEN) {
		rn->opencount++;
		return 0;
	}

	/* OFLAGS(), need to convert FREAD/FWRITE to O_RD/WR */
	fi->flags = (mode & ~(O_CREAT | O_EXCL | O_TRUNC)) - 1;

	if (pn->pn_va.va_type == VDIR) {
		if (fuse->op.opendir)
			fuse->op.opendir(path, fi);
	} else {
		if (fuse->op.open)
			fuse->op.open(path, fi);
	}

	rn->flags |= RN_OPEN;
	rn->opencount++;

	return 0;
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

	fuse = puffs_getspecific(pu);
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

	fuse = puffs_getspecific(pu);
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

	fuse = puffs_getspecific(pu);
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
puffs_fuse_node_readdir(struct puffs_cc *pcc, void *opc, struct dirent *dent,
	off_t *readoff, size_t *reslen, const struct puffs_cred *pcr,
	int *eofflag, off_t *cookies, size_t *ncookies)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = opc;
	struct refusenode	*rn = pn->pn_data;
	struct puffs_fuse_dirh	*dirh;
	struct fuse		*fuse;
	struct dirent		*fromdent;
	const char		*path = PNPATH(pn);
	int			ret;

	fuse = puffs_getspecific(pu);
	if (fuse->op.readdir == NULL && fuse->op.getdir == NULL) {
		return ENOSYS;
	}

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

		if (fuse->op.readdir)
			ret = fuse->op.readdir(path, dirh, puffs_fuse_fill_dir,
			    0, &rn->file_info);
		else
			ret = fuse->op.getdir(path, dirh, puffs_fuse_dirfil);
		if (ret)
			return -ret;
	}

	/* now, stuff results into the kernel buffers */
	while (*readoff < dirh->bufsize - dirh->reslen) {
		/*LINTED*/
		fromdent = (struct dirent *)((uint8_t *)dirh->dbuf + *readoff);

		if (*reslen < _DIRENT_SIZE(fromdent))
			break;

		memcpy(dent, fromdent, _DIRENT_SIZE(fromdent));
		*readoff += _DIRENT_SIZE(fromdent);
		*reslen -= _DIRENT_SIZE(fromdent);

		dent = _DIRENT_NEXT(dent);
	}

	return 0;
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

	fuse = puffs_getspecific(pu);
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

	fuse = puffs_getspecific(pu);
	if (fuse->op.statfs == NULL) {
		if ((ret = statvfs(PNPATH(puffs_getroot(pu)), svfsb)) == -1) {
			return errno;
		}
	} else {
		ret = fuse->op.statfs(PNPATH(puffs_getroot(pu)), svfsb);
	}

        return ret;
}


/* End of puffs_fuse operations */
/* ARGSUSED3 */
int
fuse_main_real(int argc, char **argv, const struct fuse_operations *ops,
	size_t size, void *userdata)
{
	struct fuse	*fuse;
	char		*mountpoint;
	int		 multithreaded;
	int		 fd;

	fuse = fuse_setup(argc, argv, ops, size, &mountpoint, &multithreaded,
			&fd);

	return fuse_loop(fuse);
}

/*
 * XXX: just defer the operation until fuse_new() when we have more
 * info on our hands.  The real beef is why's this separate in fuse in
 * the first place?
 */
/* ARGSUSED1 */
struct fuse_chan *
fuse_mount(const char *dir, struct fuse_args *args)
{
 	struct fuse_chan	*fc;

 	NEW(struct fuse_chan, fc, "fuse_mount", exit(EXIT_FAILURE));

 	fc->dir = strdup(dir);

	/*
	 * we need to deep copy the args struct - some fuse file
	 * systems "clean up" the argument vector for "security
	 * reasons"
	 */
	fc->args = deep_copy_args(args->argc, args->argv);
  
	return fc;
}

/* ARGSUSED1 */
struct fuse *
fuse_new(struct fuse_chan *fc, struct fuse_args *args,
	const struct fuse_operations *ops, size_t size, void *userdata)
{
	struct puffs_usermount	*pu;
	struct puffs_pathobj	*po_root;
	struct puffs_node	*pn_root;
	struct puffs_ops	*pops;
	struct refusenode	*rn_root;
	struct statvfs		svfsb;
	struct stat		st;
	struct fuse		*fuse;

	NEW(struct fuse, fuse, "fuse_new", exit(EXIT_FAILURE));

	/* copy fuse ops to their own stucture */
	(void) memcpy(&fuse->op, ops, sizeof(fuse->op));

	fcon.fuse = fuse;
	fcon.private_data = userdata;

	fuse->fc = fc;

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

	pu = puffs_mount(pops, fc->dir, MNT_NODEV | MNT_NOSUID,
			 args->argv[0], fuse,
			 PUFFS_FLAG_BUILDPATH
			   | PUFFS_FLAG_HASHPATH
			   | PUFFS_FLAG_OPDUMP
			   | PUFFS_KFLAG_NOCACHE);
	if (pu == NULL) {
		err(EXIT_FAILURE, "puffs_mount");
	}
	fc->pu = pu;

	pn_root = newrn(pu);
	puffs_setroot(pu, pn_root);
	rn_root = pn_root->pn_data;
	rn_root->flags |= RN_ROOT;

	po_root = puffs_getrootpathobj(pu);
	po_root->po_path = strdup("/");
	po_root->po_len = 1;

	/* sane defaults */
	puffs_vattr_null(&pn_root->pn_va);
	pn_root->pn_va.va_type = VDIR;
	pn_root->pn_va.va_mode = 0755;
	if (fuse->op.getattr)
		if (fuse->op.getattr(po_root->po_path, &st) == 0)
			puffs_stat2vattr(&pn_root->pn_va, &st);
	assert(pn_root->pn_va.va_type == VDIR);

	if (fuse->op.init)
		fcon.private_data = fuse->op.init(NULL); /* XXX */

	puffs_zerostatvfs(&svfsb);
	if (puffs_start(pu, pn_root, &svfsb) == -1) {
		err(EXIT_FAILURE, "puffs_start");
	}

	return fuse;
}

int
fuse_loop(struct fuse *fuse)
{

	return puffs_mainloop(fuse->fc->pu, PUFFSLOOP_NODAEMON);
}

void
fuse_destroy(struct fuse *fuse)
{


	/* XXXXXX: missing stuff */
	FREE(fuse);
}

/* XXX: threads */
struct fuse_context *
fuse_get_context()
{

	return &fcon;
}

void
fuse_exit(struct fuse *fuse)
{
	
	puffs_exit(fuse->fc->pu, 1);
}

/*
 * XXX: obviously not the most perfect of functions, but needs some
 * puffs tweaking for a better tomorrow
 */
/*ARGSUSED*/
void
fuse_unmount(const char *mp, struct fuse_chan *fc)
{

	puffs_exit(fc->pu, 1);
}

/*ARGSUSED*/
void
fuse_unmount_compat22(const char *mp)
{

	return;
}

/* The next function "exposes" struct fuse to userland.  Not much
* that we can do about this, as we're conforming to a defined
* interface.  */

void
fuse_teardown(struct fuse *fuse, char *mountpoint)
{
	fuse_unmount(mountpoint, fuse->fc);
	fuse_destroy(fuse);
}
