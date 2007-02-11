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
#include <err.h>
#include <errno.h>
#include <fuse.h>
#include <ucontext.h>
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
	int			compat;		/* compat level - not used in puffs_fuse */
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

static ino_t fakeino = 3;

/* XXX: redome */
struct feh {
	struct dirent *dent;
	size_t reslen;
};

/* ARGSUSED2 */
static int
puffs_fuse_dirfiller(void *buf, const char *name,
	const struct stat *stbuf, off_t off)
{
	struct feh *feh = buf;
	uint8_t dtype;

	/* XXX: this is hacked *purely* for hellofs, so fiXXXme */
	if (*name == '.')
		dtype = DT_DIR;
	else
		dtype = DT_REG;

	return puffs_nextdent(&feh->dent, name, fakeino++, dtype, &feh->reslen);
}

int
fuse_opt_add_arg(struct fuse_args *args, const char *arg)
{
	char	**oldargv;
	int	oldargc;

	if (args->allocated) {
		RENEW(char *, args->argv, args->argc + 1, "fuse_opt_add_arg1", return 0);
	} else {
		oldargv = args->argv;
		oldargc = args->argc;
		NEWARRAY(char *, args->argv, oldargc + 1, "fuse_opt_add_arg2", return 0);
		(void) memcpy(args->argv, oldargv, oldargc * sizeof(char *));
		args->allocated = 1;
	}
	args->argv[args->argc++] = strdup(arg);
	return 1;
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
	struct stat		st;
	struct fuse		*fuse;
	const char		*path = PCNPATH(pcn);
	int			ret;

	/* XXX: THIS IS VERY WRONG */
	fuse = (struct fuse *)pu->pu_privdata;
	ret = fuse->op.getattr(path, &st);
	ret = -ret; /* linux foo */
	if (ret != 0) {
		return ret;
	}
	*newnode = puffs_pn_new(pu, NULL);
	*newtype = (S_ISDIR(st.st_mode)) ? VDIR : VREG;
	*newsize = st.st_size;
	return ret;
}

/* get attributes for the path name */
/* ARGSUSED3 */
static int
puffs_fuse_node_getattr(struct puffs_cc *pcc, void *opc, struct vattr *va,
	const struct puffs_cred *pcr, pid_t pid) 
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = opc;
	struct stat		 st;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.getattr == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.getattr)(path, &st);

	if (ret == 0) {
		/* fill in va from st */
		va->va_mode = st.st_mode;
		va->va_nlink = st.st_nlink;
		va->va_uid = st.st_uid;
		va->va_gid = st.st_gid;
		va->va_fsid = st.st_rdev;
		va->va_fileid = st.st_ino;
		va->va_size = st.st_size;
		va->va_blocksize = st.st_blksize;
		va->va_atime = st.st_atimespec;
		va->va_mtime = st.st_mtimespec;
		va->va_ctime = st.st_ctimespec;
		va->va_birthtime = st.st_birthtimespec;
		va->va_gen = st.st_gen;
		va->va_flags = st.st_flags;
		va->va_rdev = st.st_rdev;
		va->va_bytes = st.st_size;
		va->va_filerev = st.st_gen;
		va->va_vaflags = st.st_flags;

	}

	return ret;
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
	const char		*path = PNPATH(pn);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.readlink == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.readlink)(path, linkname, *linklen);

	if (ret == 0) {
	}

	return ret;
}

/* make the special node */
/* ARGSUSED1 */
static int
puffs_fuse_node_mknod(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn;
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
		/* fix up nodes */
		pn = puffs_pn_new(pu, NULL);
		if (pn == NULL) {
			unlink(PCNPATH(pcn));
			return ENOMEM;
		}
		puffs_setvattr(&pn->pn_va, va);

		*newnode = pn;
	}

	return ret;
}

/* make a directory */
/* ARGSUSED1 */
static int
puffs_fuse_node_mkdir(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn, const struct vattr *va)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn;
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
		/* fix up nodes */
		pn = puffs_pn_new(pu, NULL);
		if (pn == NULL) {
			rmdir(PCNPATH(pcn));
			return ENOMEM;
		}
		puffs_setvattr(&pn->pn_va, va);

		*newnode = pn;
	}

	return ret;
}

/* remove the directory entry */
/* ARGSUSED1 */
static int
puffs_fuse_node_remove(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse		*fuse;
	const char		*path = PCNPATH(pcn);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.unlink == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.unlink)(path);

	return ret;
}

/* remove the directory */
/* ARGSUSED1 */
static int
puffs_fuse_node_rmdir(struct puffs_cc *pcc, void *opc, void *targ,
	const struct puffs_cn *pcn)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse		*fuse;
	const char		*path = PCNPATH(pcn);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.rmdir == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.rmdir)(path);

	return ret;
}

/* create a symbolic link */
/* ARGSUSED1 */
static int
puffs_fuse_node_symlink(struct puffs_cc *pcc, void *opc, void **newnode,
	const struct puffs_cn *pcn_src, const struct vattr *va,
	const char *link_target)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn;
	struct fuse		*fuse;
	const char		*path = PCNPATH(pcn_src);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.symlink == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.symlink)(path, link_target);
	/* XXX - check I haven't transposed these args */

	if (ret == 0) {
		/* fix up nodes */
		pn = puffs_pn_new(pu, NULL);
		if (pn == NULL) {
			unlink(link_target);
			return ENOMEM;
		}
		puffs_setvattr(&pn->pn_va, va);

		*newnode = pn;
	}

	return ret;
}

/* rename a directory entry */
/* ARGSUSED1 */
static int
puffs_fuse_node_rename(struct puffs_cc *pcc, void *opc, void *src,
	const struct puffs_cn *pcn_src, void *targ_dir, void *targ,
	const struct puffs_cn *pcn_targ)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct puffs_node	*pn = opc;
	struct vattr		va;
	struct fuse		*fuse;
	const char		*path = PCNPATH(pcn_src);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.rename == NULL) {
		return ENOSYS;
	}

	/* wrap up return code */
	ret = (*fuse->op.rename)(path, PCNPATH(pcn_targ));

	if (ret == 0) {
		(void) memcpy(&va, &pn->pn_va, sizeof(va));

		puffs_pn_put(pn);

		pn = puffs_pn_new(pu, NULL);
		if (pn == NULL) {
			return ENOMEM;
		}
		puffs_setvattr(&pn->pn_va, &va);

	}

	return ret;
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

	if (ret == 0) {
		/* fix up nodes */
		pn = puffs_pn_new(pu, NULL);
		if (pn == NULL) {
			unlink(PCNPATH(pcn));
			return ENOMEM;
		}
	}

	return ret;
}

/*
We run into a slight problemette here - puffs provides
setattr/getattr, whilst fuse provides all the usual chown/chmod/chgrp
functionality.  So that we don't miss out on anything when calling a
fuse operation, we have to get the vattr from the existing file,
find out what's changed, and then switch on that to call the fuse
function accordingly.
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
	mode_t			mode;
	uid_t			uid;
	gid_t			gid;
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;

	ret = -1;

	mode = va->va_mode;
	uid = va->va_uid;
	gid = va->va_gid;

	if (mode != 0) {
		if (fuse->op.chmod == NULL) {
			return ENOSYS;
		}
		ret = (*fuse->op.chmod)(path, mode);
	}
	if (uid != 0 || gid != 0) {
		if (fuse->op.chown == NULL) {
			return ENOSYS;
		}
		ret = (*fuse->op.chown)(path, uid, gid);
	}

	if (ret == 0) {
	}

	return ret;
}

/* ARGSUSED2 */
static int
puffs_fuse_node_open(struct puffs_cc *pcc, void *opc, int flags,
	const struct puffs_cred *cred, pid_t pid)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse_file_info	 file_info;
	struct puffs_node	*pn = opc;
	struct fuse		*fuse;
	struct stat		 st;
	const char		*path = PNPATH(pn);
	int			 ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.open == NULL) {
		return ENOSYS;
	}

	(void) memset(&file_info, 0x0, sizeof(file_info));

	/* examine type - if directory, return 0 rather than open */
	ret = (fuse->op.getattr == NULL) ?
		stat(path, &st) :
		(*fuse->op.getattr)(path, &st);
	if (ret == 0 && (st.st_mode & S_IFMT) == S_IFDIR) {
		return 0;
	}

	if (strcmp(path, "/") == 0) {
		return 0;
	}

	ret = (*fuse->op.open)(path, &file_info);

	if (ret == 0) {
	}

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
	struct fuse_file_info	file_info;
	struct puffs_node	*pn = opc;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.read == NULL) {
		return ENOSYS;
	}

	(void) memset(&file_info, 0x0, sizeof(file_info));
	/* XXX fill in file_info here */

	ret = (*fuse->op.read)(path, (char *)buf, *resid, offset, &file_info);

	if (ret > 0) {
		*resid -= ret;
	}

	return 0;
}

/* write to the file */
/* ARGSUSED0 */
static int
puffs_fuse_node_write(struct puffs_cc *pcc, void *opc, uint8_t *buf,
	off_t offset, size_t *resid, const struct puffs_cred *pcr,
	int ioflag)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse_file_info	file_info;
	struct puffs_node	*pn = opc;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.write == NULL) {
		return ENOSYS;
	}

	(void) memset(&file_info, 0x0, sizeof(file_info));

	/* XXX fill in file_info here */

	ret = (*fuse->op.write)(path, (char *)buf, *resid, offset, &file_info);

	if (ret > 0) {
		*resid -= ret;
	}

	return ret;
}


/* ARGSUSED3 */
static int
puffs_fuse_node_readdir(struct puffs_cc *pcc, void *opc,
	struct dirent *dent, const struct puffs_cred *pcr, off_t *readoff,
	size_t *reslen)
{
	struct puffs_usermount	*pu = puffs_cc_getusermount(pcc);
	struct fuse_file_info	file_info;
	struct puffs_node	*pn = opc;
	struct fuse		*fuse;
	const char		*path = PNPATH(pn);
	struct feh		feh; /* XXX */
	int			ret;

	fuse = (struct fuse *)pu->pu_privdata;
	if (fuse->op.readdir == NULL) {
		return ENOSYS;
	}

	/* XXX: how to handle this??? */
	if (*readoff != 0) {
		return 0;
	}

	(void) memset(&file_info, 0x0, sizeof(file_info));
	/* XXX - fill in file_info here */

	feh.dent = dent;
	feh.reslen = *reslen;
	ret = (*fuse->op.readdir)(path, &feh, puffs_fuse_dirfiller, *readoff, &file_info);
	*reslen = feh.reslen;
	*readoff = 1;

	if (ret == 0) {
	}

	return ret;
}

/* ARGSUSED */
static int
puffs_fuse_node_reclaim(struct puffs_cc *pcc, void *opc, pid_t pid)
{
	struct puffs_node	*pn = opc;

	puffs_pn_put(pn);

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
	struct statvfs		svfsb;
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
        PUFFSOP_SET(pops, puffs_fuse, node, readdir);
        PUFFSOP_SET(pops, puffs_fuse, node, readlink);
        PUFFSOP_SET(pops, puffs_fuse, node, mknod);
        PUFFSOP_SET(pops, puffs_fuse, node, mkdir);
        PUFFSOP_SET(pops, puffs_fuse, node, remove);
        PUFFSOP_SET(pops, puffs_fuse, node, rmdir);
        PUFFSOP_SET(pops, puffs_fuse, node, symlink);
        PUFFSOP_SET(pops, puffs_fuse, node, rename);
        PUFFSOP_SET(pops, puffs_fuse, node, link);
        PUFFSOP_SET(pops, puffs_fuse, node, setattr);
        PUFFSOP_SET(pops, puffs_fuse, node, open);
        PUFFSOP_SET(pops, puffs_fuse, node, read);
        PUFFSOP_SET(pops, puffs_fuse, node, write);
        PUFFSOP_SET(pops, puffs_fuse, node, readdir);
        PUFFSOP_SET(pops, puffs_fuse, node, read);
        PUFFSOP_SET(pops, puffs_fuse, node, write);
        PUFFSOP_SET(pops, puffs_fuse, node, reclaim);

	NEW(struct fuse, fuse, "fuse_main_real", exit(EXIT_FAILURE));

	/* copy fuse ops to their own stucture */
	(void) memcpy(&fuse->op, ops, sizeof(fuse->op));

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
			PUFFS_FLAG_BUILDPATH | PUFFS_FLAG_OPDUMP, 0);
	if (pu == NULL) {
		err(EXIT_FAILURE, "puffs_mount");
	}

	fuse->pu = pu;
	pu->pu_pn_root = puffs_pn_new(pu, NULL);
	po_root = puffs_getrootpathobj(pu);
	po_root->po_path = strdup("/");
	po_root->po_len = 1;

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
