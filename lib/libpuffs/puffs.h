/*	$NetBSD: puffs.h,v 1.48 2007/04/22 18:02:44 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
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

#ifndef _PUFFS_H_
#define _PUFFS_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/vnode.h>

#include <fs/puffs/puffs_msgif.h>

#include <mntopts.h>
#include <string.h>

/* forwards */
struct puffs_cc;

struct puffs_getreq;
struct puffs_putreq;

/* paths */
struct puffs_pathobj {
	void 	*po_path;
	size_t	po_len;
};

/* for prefix rename */
struct puffs_pathinfo {
	struct puffs_pathobj *pi_old;
	struct puffs_pathobj *pi_new;
};

/* describes one segment cached in the kernel */
struct puffs_kcache {
	off_t	pkc_start;
	off_t	pkc_end;

	LIST_ENTRY(puffs_kcache) pkc_entries;
};

/* XXX: might disappear from here into a private header */
struct puffs_node {
	off_t			pn_size;
	int			pn_flag;
	struct vattr		pn_va;

	void			*pn_data;	/* private data		*/

	struct puffs_pathobj	pn_po;		/* PUFFS_FLAG_BUILDPATH */

	struct puffs_usermount 	*pn_mnt;
	LIST_ENTRY(puffs_node)	pn_entries;

	LIST_HEAD(,puffs_kcache)pn_cacheinfo;	/* PUFFS_KFLAG_CACHE	*/
};


struct puffs_usermount;

/*
 * megaXXX: these are values from inside _KERNEL
 * need to work on the translation for ALL the necessary values.
 */
#define PUFFS_VNOVAL (-1)
#define PUFFS_IO_APPEND 0x20
#define PUFFS_VEXEC	01
#define PUFFS_VWRITE	02
#define PUFFS_VREAD	04

#define PUFFS_FSYNC_DATAONLY 0x0002
#define PUFFS_FSYNC_CACHE    0x0100

/*
 * Magic constants
 */
#define PUFFS_CC_STACKSIZE_DEFAULT (1024*1024)

struct puffs_cn {
	struct puffs_kcn	*pcn_pkcnp;	/* kernel input */
	struct puffs_pathobj	pcn_po_full;	/* PUFFS_FLAG_BUILDPATH */
};
#define pcn_nameiop	pcn_pkcnp->pkcn_nameiop
#define pcn_flags	pcn_pkcnp->pkcn_flags
#define pcn_pid		pcn_pkcnp->pkcn_pid
#define pcn_cred	pcn_pkcnp->pkcn_cred
#define pcn_name	pcn_pkcnp->pkcn_name
#define pcn_namelen	pcn_pkcnp->pkcn_namelen

/*
 * Puffs options to mount
 */
/* kernel */
#define	PUFFSMOPT_CACHE		{ "cache", 1, PUFFS_KFLAG_NOCACHE, 1 }
#define PUFFSMOPT_ALLOPS	{ "allops", 0, PUFFS_KFLAG_ALLOPS, 1 }

/* libpuffs */
#define PUFFSMOPT_DUMP		{ "dump", 0, PUFFS_FLAG_OPDUMP, 1 }

#define PUFFSMOPT_STD							\
	PUFFSMOPT_CACHE,						\
	PUFFSMOPT_ALLOPS,						\
	PUFFSMOPT_DUMP

extern const struct mntopt puffsmopts[]; /* puffs.c */

/* callbacks for operations */
struct puffs_ops {
	int (*puffs_fs_unmount)(struct puffs_cc *, int, pid_t);
	int (*puffs_fs_statvfs)(struct puffs_cc *,
	    struct statvfs *, pid_t);
	int (*puffs_fs_sync)(struct puffs_cc *, int,
	    const struct puffs_cred *, pid_t);
	int (*puffs_fs_fhtonode)(struct puffs_cc *, void *, size_t,
	    void **, enum vtype *, voff_t *, dev_t *);
	int (*puffs_fs_nodetofh)(struct puffs_cc *, void *cookie,
	    void *, size_t *);
	void (*puffs_fs_suspend)(struct puffs_cc *, int);

	int (*puffs_node_lookup)(struct puffs_cc *,
	    void *, void **, enum vtype *, voff_t *, dev_t *,
	    const struct puffs_cn *);
	int (*puffs_node_create)(struct puffs_cc *,
	    void *, void **, const struct puffs_cn *, const struct vattr *);
	int (*puffs_node_mknod)(struct puffs_cc *,
	    void *, void **, const struct puffs_cn *, const struct vattr *);
	int (*puffs_node_open)(struct puffs_cc *,
	    void *, int, const struct puffs_cred *, pid_t);
	int (*puffs_node_close)(struct puffs_cc *,
	    void *, int, const struct puffs_cred *, pid_t);
	int (*puffs_node_access)(struct puffs_cc *,
	    void *, int, const struct puffs_cred *, pid_t);
	int (*puffs_node_getattr)(struct puffs_cc *,
	    void *, struct vattr *, const struct puffs_cred *, pid_t);
	int (*puffs_node_setattr)(struct puffs_cc *,
	    void *, const struct vattr *, const struct puffs_cred *, pid_t);
	int (*puffs_node_poll)(struct puffs_cc *,
	    void *, struct puffs_vnreq_poll *);
	int (*puffs_node_mmap)(struct puffs_cc *,
	    void *, int, const struct puffs_cred *, pid_t);
	int (*puffs_node_fsync)(struct puffs_cc *,
	    void *, const struct puffs_cred *, int, off_t, off_t, pid_t);
	int (*puffs_node_seek)(struct puffs_cc *,
	    void *, off_t, off_t, const struct puffs_cred *);
	int (*puffs_node_remove)(struct puffs_cc *,
	    void *, void *, const struct puffs_cn *);
	int (*puffs_node_link)(struct puffs_cc *,
	    void *, void *, const struct puffs_cn *);
	int (*puffs_node_rename)(struct puffs_cc *,
	    void *, void *, const struct puffs_cn *, void *, void *,
	    const struct puffs_cn *);
	int (*puffs_node_mkdir)(struct puffs_cc *,
	    void *, void **, const struct puffs_cn *, const struct vattr *);
	int (*puffs_node_rmdir)(struct puffs_cc *,
	    void *, void *, const struct puffs_cn *);
	int (*puffs_node_symlink)(struct puffs_cc *,
	    void *, void **, const struct puffs_cn *, const struct vattr *,
	    const char *);
	int (*puffs_node_readdir)(struct puffs_cc *,
	    void *, struct dirent *, off_t *, size_t *,
	    const struct puffs_cred *, int *, off_t *, size_t *);
	int (*puffs_node_readlink)(struct puffs_cc *,
	    void *, const struct puffs_cred *, char *, size_t *);
	int (*puffs_node_reclaim)(struct puffs_cc *,
	    void *, pid_t);
	int (*puffs_node_inactive)(struct puffs_cc *,
	    void *, pid_t, int *);
	int (*puffs_node_print)(struct puffs_cc *,
	    void *);
	int (*puffs_node_pathconf)(struct puffs_cc *,
	    void *, int, int *);
	int (*puffs_node_advlock)(struct puffs_cc *,
	    void *, void *, int, struct flock *, int);
	int (*puffs_node_getextattr)(struct puffs_cc *,
	    void *, struct puffs_vnreq_getextattr *);
	int (*puffs_node_setextattr)(struct puffs_cc *,
	    void *, struct puffs_vnreq_setextattr *);
	int (*puffs_node_listextattr)(struct puffs_cc *,
	    void *, struct puffs_vnreq_listextattr *);
	int (*puffs_node_read)(struct puffs_cc *, void *,
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);
	int (*puffs_node_write)(struct puffs_cc *, void *,
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);

	int (*puffs_cache_write)(struct puffs_usermount *,
		void *, size_t, struct puffs_cacherun *);
};

typedef	int (*pu_pathbuild_fn)(struct puffs_usermount *,
			       const struct puffs_pathobj *,
			       const struct puffs_pathobj *, size_t,
			       struct puffs_pathobj *);
typedef int (*pu_pathtransform_fn)(struct puffs_usermount *,
				   const struct puffs_pathobj *,
				   const struct puffs_cn *,
				   struct puffs_pathobj *);
typedef int (*pu_pathcmp_fn)(struct puffs_usermount *, struct puffs_pathobj *,
			  struct puffs_pathobj *, size_t, int);
typedef void (*pu_pathfree_fn)(struct puffs_usermount *,
			       struct puffs_pathobj *);
typedef int (*pu_namemod_fn)(struct puffs_usermount *,
			     struct puffs_pathobj *, struct puffs_cn *);

enum {
	PUFFS_STATE_BEFOREMOUNT,
	PUFFS_STATE_MOUNTING,		PUFFS_STATE_RUNNING,
	PUFFS_STATE_UNMOUNTING,		PUFFS_STATE_UNMOUNTED
};

#define PUFFS_FLAG_BUILDPATH	0x80000000	/* node paths in pnode */
#define PUFFS_FLAG_OPDUMP	0x40000000	/* dump all operations */
#define PUFFS_FLAG_MASK		0xc0000000

#define PUFFS_FLAG_KERN(a)	((a) & PUFFS_KFLAG_MASK)
#define PUFFS_FLAG_LIB(a)	((a) & PUFFS_FLAG_MASK)

/* blocking mode argument */
#define PUFFSDEV_BLOCK 0
#define PUFFSDEV_NONBLOCK 1

/* mainloop flags */
#define PUFFSLOOP_NODAEMON 0x01

#define		DENT_DOT	0
#define		DENT_DOTDOT	1
#define		DENT_ADJ(a)	((a)-2)	/* nth request means dir's n-2th */


/*
 * protos
 */

#define PUFFSOP_PROTOS(fsname)						\
	int fsname##_fs_mount(struct puffs_usermount *, void **,	\
	    struct statvfs *);						\
	int fsname##_fs_unmount(struct puffs_cc *, int, pid_t);		\
	int fsname##_fs_statvfs(struct puffs_cc *,			\
	    struct statvfs *, pid_t);					\
	int fsname##_fs_sync(struct puffs_cc *, int,			\
	    const struct puffs_cred *cred, pid_t);			\
	int fsname##_fs_fhtonode(struct puffs_cc *, void *, size_t,	\
	    void **, enum vtype *, voff_t *, dev_t *);			\
	int fsname##_fs_nodetofh(struct puffs_cc *, void *cookie,	\
	    void *, size_t *);						\
	void fsname##_fs_suspend(struct puffs_cc *, int);		\
									\
	int fsname##_node_lookup(struct puffs_cc *,			\
	    void *, void **, enum vtype *, voff_t *, dev_t *,		\
	    const struct puffs_cn *);					\
	int fsname##_node_create(struct puffs_cc *,			\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *);					\
	int fsname##_node_mknod(struct puffs_cc *,			\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *);					\
	int fsname##_node_open(struct puffs_cc *,			\
	    void *, int, const struct puffs_cred *, pid_t);		\
	int fsname##_node_close(struct puffs_cc *,			\
	    void *, int, const struct puffs_cred *, pid_t);		\
	int fsname##_node_access(struct puffs_cc *,			\
	    void *, int, const struct puffs_cred *, pid_t);		\
	int fsname##_node_getattr(struct puffs_cc *,			\
	    void *, struct vattr *, const struct puffs_cred *, pid_t);	\
	int fsname##_node_setattr(struct puffs_cc *,			\
	    void *, const struct vattr *, const struct puffs_cred *,	\
	    pid_t);							\
	int fsname##_node_poll(struct puffs_cc *,			\
	    void *, struct puffs_vnreq_poll *);				\
	int fsname##_node_mmap(struct puffs_cc *,			\
	    void *, int, const struct puffs_cred *, pid_t);		\
	int fsname##_node_fsync(struct puffs_cc *,			\
	    void *, const struct puffs_cred *, int, off_t, off_t,	\
	    pid_t);							\
	int fsname##_node_seek(struct puffs_cc *,			\
	    void *, off_t, off_t, const struct puffs_cred *);		\
	int fsname##_node_remove(struct puffs_cc *,			\
	    void *, void *, const struct puffs_cn *);			\
	int fsname##_node_link(struct puffs_cc *,			\
	    void *, void *, const struct puffs_cn *);			\
	int fsname##_node_rename(struct puffs_cc *,			\
	    void *, void *, const struct puffs_cn *, void *, void *,	\
	    const struct puffs_cn *);					\
	int fsname##_node_mkdir(struct puffs_cc *,			\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *);					\
	int fsname##_node_rmdir(struct puffs_cc *,			\
	    void *, void *, const struct puffs_cn *);			\
	int fsname##_node_symlink(struct puffs_cc *,			\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *, const char *);			\
	int fsname##_node_readdir(struct puffs_cc *,			\
	    void *, struct dirent *, off_t *, size_t *,			\
	    const struct puffs_cred *, int *, off_t *, size_t *);	\
	int fsname##_node_readlink(struct puffs_cc *,			\
	    void *, const struct puffs_cred *, char *, size_t *);	\
	int fsname##_node_reclaim(struct puffs_cc *,			\
	    void *, pid_t);						\
	int fsname##_node_inactive(struct puffs_cc *,			\
	    void *, pid_t, int *);					\
	int fsname##_node_print(struct puffs_cc *,			\
	    void *);							\
	int fsname##_node_pathconf(struct puffs_cc *,			\
	    void *, int, int *);					\
	int fsname##_node_advlock(struct puffs_cc *,			\
	    void *, void *, int, struct flock *, int);			\
	int fsname##_node_getextattr(struct puffs_cc *,			\
	    void *, struct puffs_vnreq_getextattr *);			\
	int fsname##_node_setextattr(struct puffs_cc *,			\
	    void *, struct puffs_vnreq_setextattr *);			\
	int fsname##_node_listextattr(struct puffs_cc *,		\
	    void *, struct puffs_vnreq_listextattr *);			\
	int fsname##_node_read(struct puffs_cc *, void *,		\
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);\
	int fsname##_node_write(struct puffs_cc *, void *,		\
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);\
									\
	int fsname##_cache_write(struct puffs_usermount *, void *,	\
	    size_t, struct puffs_cacheinfo *);

#define PUFFSOP_INIT(ops)						\
    ops = malloc(sizeof(struct puffs_ops));				\
    memset(ops, 0, sizeof(struct puffs_ops))
#define PUFFSOP_SET(ops, fsname, fsornode, opname)			\
    (ops)->puffs_##fsornode##_##opname = fsname##_##fsornode##_##opname
#define PUFFSOP_SETFSNOP(ops, opname)					\
    (ops)->puffs_fs_##opname = puffs_fsnop_##opname

#define PUFFS_DEVEL_LIBVERSION 13
#define puffs_mount(a,b,c,d,e,f) \
    _puffs_mount(PUFFS_DEVEL_LIBVERSION,a,b,c,d,e,f)
#define puffs_init(a,b,c,d) \
    _puffs_init(PUFFS_DEVEL_LIBVERSION,a,b,c,d)


#define PNPATH(pnode)	((pnode)->pn_po.po_path)
#define PNPLEN(pnode)	((pnode)->pn_po.po_len)
#define PCNPATH(pcnode)	((pcnode)->pcn_po_full.po_path)
#define PCNPLEN(pcnode)	((pcnode)->pcn_po_full.po_len)
#define PCNISDOTDOT(pcnode) \
	((pcnode)->pcn_namelen == 2 && strcmp((pcnode)->pcn_name, "..") == 0)

#define PUFFS_STORE_DCOOKIE(cp, ncp, off)				\
if (cp)	{								\
	*((cp)++) = off;						\
	(*(ncp))++;							\
}

__BEGIN_DECLS

struct puffs_usermount *_puffs_mount(int, struct puffs_ops *, const char *, int,
				    const char *, void *, uint32_t);
struct puffs_usermount *_puffs_init(int, struct puffs_ops *pops, const char *,
				    void *, uint32_t);
int		puffs_domount(struct puffs_usermount *, const char *, int);
int		puffs_start(struct puffs_usermount *, void *, struct statvfs *);
int		puffs_exit(struct puffs_usermount *, int);
int		puffs_mainloop(struct puffs_usermount *, int);


int	puffs_getselectable(struct puffs_usermount *);
int	puffs_setblockingmode(struct puffs_usermount *, int);
int	puffs_getstate(struct puffs_usermount *);
void	puffs_setstacksize(struct puffs_usermount *, size_t);

void			puffs_setroot(struct puffs_usermount *,
				      struct puffs_node *);
struct puffs_node 	*puffs_getroot(struct puffs_usermount *);
void			*puffs_getspecific(struct puffs_usermount *);
void			puffs_setmaxreqlen(struct puffs_usermount *, size_t);
size_t			puffs_getmaxreqlen(struct puffs_usermount *);
void			puffs_setfhsize(struct puffs_usermount *, size_t, int);

void			puffs_setncookiehash(struct puffs_usermount *, int);

struct puffs_pathobj	*puffs_getrootpathobj(struct puffs_usermount *);

struct puffs_node	*puffs_pn_new(struct puffs_usermount *, void *);
void			puffs_pn_put(struct puffs_node *);

void			*puffs_pn_getmntspecific(struct puffs_node *);

typedef		void *	(*puffs_nodewalk_fn)(struct puffs_usermount *,
					     struct puffs_node *, void *);
void			*puffs_pn_nodewalk(struct puffs_usermount *,
					   puffs_nodewalk_fn, void *);

void			puffs_setvattr(struct vattr *, const struct vattr *);
void			puffs_vattr_null(struct vattr *);

/*
 * generic/dummy routines applicable for some file systems
 */
int  puffs_fsnop_unmount(struct puffs_cc *, int, pid_t);
int  puffs_fsnop_statvfs(struct puffs_cc *, struct statvfs *, pid_t);
void puffs_zerostatvfs(struct statvfs *);
int  puffs_fsnop_sync(struct puffs_cc *, int waitfor,
		      const struct puffs_cred *, pid_t);
int  puffs_genfs_node_reclaim(struct puffs_cc *, void *, pid_t);

/*
 * Subroutine stuff
 */

int		puffs_gendotdent(struct dirent **, ino_t, int, size_t *);
int		puffs_nextdent(struct dirent **, const char *, ino_t,
			       uint8_t, size_t *);
int		puffs_vtype2dt(enum vtype);
enum vtype	puffs_mode2vt(mode_t);
void		puffs_stat2vattr(struct vattr *va, const struct stat *);
mode_t		puffs_addvtype2mode(mode_t, enum vtype);


/*
 * credentials & permissions
 */

/* Credential fetch */
int	puffs_cred_getuid(const struct puffs_cred *pcr, uid_t *);
int	puffs_cred_getgid(const struct puffs_cred *pcr, gid_t *);
int	puffs_cred_getgroups(const struct puffs_cred *pcr, gid_t *, short *);

/* Credential check */
int	puffs_cred_isuid(const struct puffs_cred *, uid_t);
int	puffs_cred_hasgroup(const struct puffs_cred *, gid_t);
int	puffs_cred_isregular(const struct puffs_cred *);
int	puffs_cred_iskernel(const struct puffs_cred *);
int	puffs_cred_isfs(const struct puffs_cred *);
int	puffs_cred_isjuggernaut(const struct puffs_cred *);

/* misc */
int	puffs_access(enum vtype, mode_t, uid_t, gid_t, mode_t,
		     const struct puffs_cred *);
int	puffs_access_chown(uid_t, gid_t, uid_t, gid_t,
			   const struct puffs_cred *);
int	puffs_access_chmod(uid_t, gid_t, enum vtype, mode_t,
			   const struct puffs_cred *);
int	puffs_access_times(uid_t, gid_t, mode_t, int,
			   const struct puffs_cred *);


/*
 * Requests
 */

struct puffs_getreq	*puffs_req_makeget(struct puffs_usermount *,
					   size_t, int);
int			puffs_req_loadget(struct puffs_getreq *);
struct puffs_req	*puffs_req_get(struct puffs_getreq *);
int			puffs_req_remainingget(struct puffs_getreq *);
void			puffs_req_setmaxget(struct puffs_getreq *, int);
void			puffs_req_destroyget(struct puffs_getreq *);

struct puffs_putreq	*puffs_req_makeput(struct puffs_usermount *);
void			puffs_req_put(struct puffs_putreq *,struct puffs_req *);
void			puffs_req_putcc(struct puffs_putreq *,struct puffs_cc*);
int			puffs_req_putput(struct puffs_putreq *);
void			puffs_req_resetput(struct puffs_putreq *);
void			puffs_req_destroyput(struct puffs_putreq *);

int			puffs_req_handle(struct puffs_usermount *,
					 struct puffs_getreq *,
					 struct puffs_putreq *, int);

/*
 * Call Context interfaces relevant for user.
 */

void			puffs_cc_yield(struct puffs_cc *);
void			puffs_cc_continue(struct puffs_cc *);
struct puffs_usermount	*puffs_cc_getusermount(struct puffs_cc *);
void 			*puffs_cc_getspecific(struct puffs_cc *);
struct puffs_cc 	*puffs_cc_create(struct puffs_usermount *);
void			puffs_cc_destroy(struct puffs_cc *);

/*
 * Execute or continue a request
 */

int	puffs_dopreq(struct puffs_usermount *, struct puffs_req *,
		     struct puffs_putreq *);
int	puffs_docc(struct puffs_cc *, struct puffs_putreq *);

/*
 * Flushing / invalidation routines
 */

int	puffs_inval_namecache_dir(struct puffs_usermount *, void *);
int	puffs_inval_namecache_all(struct puffs_usermount *);

int	puffs_inval_pagecache_node(struct puffs_usermount *, void *);
int	puffs_inval_pagecache_node_range(struct puffs_usermount *, void *,
					 off_t, off_t);
int	puffs_flush_pagecache_node(struct puffs_usermount *, void *);
int	puffs_flush_pagecache_node_range(struct puffs_usermount *, void *,
					 off_t, off_t);

/*
 * Path constructicons
 */

int	puffs_stdpath_buildpath(struct puffs_usermount *,
			     const struct puffs_pathobj *,
			     const struct puffs_pathobj *, size_t,
			     struct puffs_pathobj *);
int	puffs_stdpath_cmppath(struct puffs_usermount *, struct puffs_pathobj *,
			   struct puffs_pathobj *, size_t, int);
void	puffs_stdpath_freepath(struct puffs_usermount *,struct puffs_pathobj *);

void	*puffs_path_walkcmp(struct puffs_usermount *,
			    struct puffs_node *, void *);
void	*puffs_path_prefixadj(struct puffs_usermount *,
			      struct puffs_node *, void *);
int	puffs_path_pcnbuild(struct puffs_usermount *,
			    struct puffs_cn *, void *);

void	puffs_set_pathbuild(struct puffs_usermount *, pu_pathbuild_fn);
void	puffs_set_pathtransform(struct puffs_usermount *, pu_pathtransform_fn);
void	puffs_set_pathcmp(struct puffs_usermount *, pu_pathcmp_fn);
void	puffs_set_pathfree(struct puffs_usermount *, pu_pathfree_fn);
void	puffs_set_namemod(struct puffs_usermount *, pu_namemod_fn);

/*
 * Suspension
 */

int	puffs_fs_suspend(struct puffs_usermount *);

__END_DECLS

#endif /* _PUFFS_H_ */
