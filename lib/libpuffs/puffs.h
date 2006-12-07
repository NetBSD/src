/*	$NetBSD: puffs.h,v 1.16 2006/12/07 23:15:20 pooka Exp $	*/

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
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/vnode.h>

#include <fs/puffs/puffs_msgif.h>

/*
 * megaXXX: these are values from inside _KERNEL
 * need to work on the translation for ALL the necessary values.
 */
#define PUFFS_VNOVAL (-1)
#define PUFFS_ISDOTDOT 0x2000
#define PUFFS_IO_APPEND 0x20

/*
 * puffs vnode, i.e. puffs_node
 *
 * This represents all the relevant fields from struct vnode.
 * This may some day become struct vnode, but let's keep it separate
 * for now.
 *
 * XXX: this is not finished
 */
struct puffs_node {
	off_t		pn_size;
	int		pn_flag;		/* struct vnode	flags	*/
	void		*pn_data;		/* private data		*/
	enum vtype	pn_type;
	struct vattr	pn_va;			/* XXX: doesn't belong here*/

	struct puffs_usermount *pn_mnt;

	LIST_ENTRY(puffs_node) pn_entries;
};

/* callbacks for operations */
struct puffs_ops {
	int (*puffs_fs_mount)(struct puffs_usermount *, void **);
	int (*puffs_fs_unmount)(struct puffs_usermount *, int, pid_t);
	int (*puffs_fs_statvfs)(struct puffs_usermount *,
	    struct statvfs *, pid_t);
	int (*puffs_fs_sync)(struct puffs_usermount *, int,
	    const struct puffs_cred *, pid_t);

	int (*puffs_node_lookup)(struct puffs_usermount *,
	    void *, void **, enum vtype *, voff_t *, dev_t *,
	    const struct puffs_cn *);
	int (*puffs_node_create)(struct puffs_usermount *,
	    void *, void **, const struct puffs_cn *, const struct vattr *);
	int (*puffs_node_mknod)(struct puffs_usermount *,
	    void *, void **, const struct puffs_cn *, const struct vattr *);
	int (*puffs_node_open)(struct puffs_usermount *,
	    void *, int, const struct puffs_cred *, pid_t);
	int (*puffs_node_close)(struct puffs_usermount *,
	    void *, int, const struct puffs_cred *, pid_t);
	int (*puffs_node_access)(struct puffs_usermount *,
	    void *, int, const struct puffs_cred *, pid_t);
	int (*puffs_node_getattr)(struct puffs_usermount *,
	    void *, struct vattr *, const struct puffs_cred *, pid_t);
	int (*puffs_node_setattr)(struct puffs_usermount *,
	    void *, const struct vattr *, const struct puffs_cred *, pid_t);
	int (*puffs_node_poll)(struct puffs_usermount *,
	    void *, struct puffs_vnreq_poll *);
	int (*puffs_node_revoke)(struct puffs_usermount *, void *, int);
	int (*puffs_node_mmap)(struct puffs_usermount *,
	    void *, int, const struct puffs_cred *, pid_t);
	int (*puffs_node_fsync)(struct puffs_usermount *,
	    void *, const struct puffs_cred *, int, off_t, off_t, pid_t);
	int (*puffs_node_seek)(struct puffs_usermount *,
	    void *, off_t, off_t, const struct puffs_cred *);
	int (*puffs_node_remove)(struct puffs_usermount *,
	    void *, void *, const struct puffs_cn *);
	int (*puffs_node_link)(struct puffs_usermount *,
	    void *, void *, const struct puffs_cn *);
	int (*puffs_node_rename)(struct puffs_usermount *,
	    void *, void *, const struct puffs_cn *, void *, void *,
	    const struct puffs_cn *);
	int (*puffs_node_mkdir)(struct puffs_usermount *,
	    void *, void **, const struct puffs_cn *, const struct vattr *);
	int (*puffs_node_rmdir)(struct puffs_usermount *,
	    void *, void *, const struct puffs_cn *);
	int (*puffs_node_symlink)(struct puffs_usermount *,
	    void *, void **, const struct puffs_cn *, const struct vattr *,
	    const char *);
	int (*puffs_node_readdir)(struct puffs_usermount *,
	    void *, struct dirent *, const struct puffs_cred *,
	    off_t *, size_t *);
	int (*puffs_node_readlink)(struct puffs_usermount *,
	    void *, const struct puffs_cred *, char *, size_t *);
	int (*puffs_node_reclaim)(struct puffs_usermount *,
	    void *, pid_t);
	int (*puffs_node_inactive)(struct puffs_usermount *,
	    void *, pid_t, int *);
	int (*puffs_node_print)(struct puffs_usermount *,
	    void *);
	int (*puffs_node_pathconf)(struct puffs_usermount *,
	    void *, int, int *);
	int (*puffs_node_advlock)(struct puffs_usermount *,
	    void *, void *, int, struct flock *, int);
	int (*puffs_node_getextattr)(struct puffs_usermount *,
	    void *, struct puffs_vnreq_getextattr *);
	int (*puffs_node_setextattr)(struct puffs_usermount *,
	    void *, struct puffs_vnreq_setextattr *);
	int (*puffs_node_listextattr)(struct puffs_usermount *,
	    void *, struct puffs_vnreq_listextattr *);
	int (*puffs_node_read)(struct puffs_usermount *, void *,
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);
	int (*puffs_node_write)(struct puffs_usermount *, void *,
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);

	int (*puffs_node_ioctl1)(struct puffs_usermount *, void *,
	    struct puffs_vnreq_fcnioctl *, struct puffs_sizeop *);
	int (*puffs_node_ioctl2)(struct puffs_usermount *, void *,
	    struct puffs_vnreq_fcnioctl *, struct puffs_sizeop *);
	int (*puffs_node_fcntl1)(struct puffs_usermount *, void *,
	    struct puffs_vnreq_fcnioctl *, struct puffs_sizeop *);
	int (*puffs_node_fcntl2)(struct puffs_usermount *, void *,
	    struct puffs_vnreq_fcnioctl *, struct puffs_sizeop *);
};

struct puffs_usermount {
	struct puffs_ops	pu_ops;

	int			pu_fd;
	uint32_t		pu_flags;
	size_t			pu_maxreqlen;

	struct puffs_node	*pu_rootnode;
	const char *		pu_rootpath;
	fsid_t			pu_fsidx;
	LIST_HEAD(, puffs_node)	pu_pnodelst;

	int	pu_wcnt;
	void	*pu_privdata;
};

#define PUFFS_FLAG_KERN(a)	((a) & 0x0000ffff)
#define PUFFS_FLAG_LIB(a)	((a) & 0xffff0000)

#define PUFFS_FLAG_OPDUMP	0x80000000	/* dump all operations */

struct puffs_usermount *puffs_mount(struct puffs_ops *, const char *, int,
				    const char *, uint32_t, size_t);
int		puffs_mainloop(struct puffs_usermount *, int);
int		puffs_oneop(struct puffs_usermount *, uint8_t *, size_t);
int		puffs_getselectable(struct puffs_usermount *);
int		puffs_setblockingmode(struct puffs_usermount *, int);
void		puffs_dummyops(struct puffs_ops *);

/* blocking mode argument */
#define PUFFSDEV_BLOCK 0
#define PUFFSDEV_NONBLOCK 1

/* mainloop flags */
#define PUFFSLOOP_NODAEMON 0x01

struct puffs_node *	puffs_newpnode(struct puffs_usermount *, void *,
				       enum vtype);
void		puffs_putpnode(struct puffs_node *);
void		puffs_setvattr(struct vattr *, const struct vattr *);

int puffs_fsnop_unmount(struct puffs_usermount *, int, pid_t);
int puffs_fsnop_statvfs(struct puffs_usermount *, struct statvfs *, pid_t);
int puffs_fsnop_sync(struct puffs_usermount *, int waitfor,
		      const struct puffs_cred *, pid_t);

#define		DENT_DOT	0
#define		DENT_DOTDOT	1
#define		DENT_ADJ(a)	((a)-2)	/* nth request means dir's n-2th */
int		puffs_gendotdent(struct dirent **, ino_t, int, size_t *);
int		puffs_nextdent(struct dirent **, const char *, ino_t,
			       uint8_t, size_t *);
int		puffs_vtype2dt(enum vtype);
enum vtype	puffs_mode2vt(mode_t);


/*
 * Operation credentials
 */

/* Credential fetch */
int	puffs_cred_getuid(const struct puffs_cred *pcr, uid_t *);
int	puffs_cred_getgid(const struct puffs_cred *pcr, gid_t *);
int	puffs_cred_getgroups(const struct puffs_cred *pcr, gid_t *, short *);

/* Credential check */
int	puffs_cred_isuid(const struct puffs_cred *pcr, uid_t);
int	puffs_cred_hasgroup(const struct puffs_cred *pcr, gid_t);
/* kernel internal NOCRED */
int	puffs_cred_iskernel(const struct puffs_cred *pcr);
/* kernel internal FSCRED */
int	puffs_cred_isfs(const struct puffs_cred *pcr);
/* root || NOCRED || FSCRED */
int	puffs_cred_isjuggernaut(const struct puffs_cred *pcr);


/*
 * Requests
 */

struct puffs_getreq;
struct puffs_putreq;

struct puffs_getreq	*puffs_makegetreq(struct puffs_usermount *,
					  uint8_t *, size_t, int);
struct puffs_req	*puffs_getreq(struct puffs_getreq *);
int			puffs_remaininggetreq(struct puffs_getreq *);
void			puffs_destroygetreq(struct puffs_getreq *);

struct puffs_putreq	*puffs_makeputreq(struct puffs_usermount *);
void			puffs_putreq(struct puffs_putreq *, struct puffs_req *);
int			puffs_putputreq(struct puffs_putreq *);
void			puffs_destroyputreq(struct puffs_putreq *);


#define PUFFSOP_PROTOS(fsname)						\
	int fsname##_fs_mount(struct puffs_usermount *, void **);	\
	int fsname##_fs_unmount(struct puffs_usermount *, int, pid_t);	\
	int fsname##_fs_statvfs(struct puffs_usermount *,		\
	    struct statvfs *, pid_t);					\
	int fsname##_fs_sync(struct puffs_usermount *, int,		\
	    const struct puffs_cred *cred, pid_t);			\
									\
	int fsname##_node_lookup(struct puffs_usermount *,		\
	    void *, void **, enum vtype *, voff_t *, dev_t *,		\
	    const struct puffs_cn *);					\
	int fsname##_node_create(struct puffs_usermount *,		\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *);					\
	int fsname##_node_mknod(struct puffs_usermount *,		\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *);					\
	int fsname##_node_open(struct puffs_usermount *,		\
	    void *, int, const struct puffs_cred *, pid_t);		\
	int fsname##_node_close(struct puffs_usermount *,		\
	    void *, int, const struct puffs_cred *, pid_t);		\
	int fsname##_node_access(struct puffs_usermount *,		\
	    void *, int, const struct puffs_cred *, pid_t);		\
	int fsname##_node_getattr(struct puffs_usermount *,		\
	    void *, struct vattr *, const struct puffs_cred *, pid_t);	\
	int fsname##_node_setattr(struct puffs_usermount *,		\
	    void *, const struct vattr *, const struct puffs_cred *,	\
	    pid_t);							\
	int fsname##_node_poll(struct puffs_usermount *,		\
	    void *, struct puffs_vnreq_poll *);				\
	int fsname##_node_revoke(struct puffs_usermount *, void *, int);\
	int fsname##_node_mmap(struct puffs_usermount *,		\
	    void *, int, const struct puffs_cred *, pid_t);		\
	int fsname##_node_fsync(struct puffs_usermount *,		\
	    void *, const struct puffs_cred *, int, off_t, off_t,	\
	    pid_t);							\
	int fsname##_node_seek(struct puffs_usermount *,		\
	    void *, off_t, off_t, const struct puffs_cred *);		\
	int fsname##_node_remove(struct puffs_usermount *,		\
	    void *, void *, const struct puffs_cn *);			\
	int fsname##_node_link(struct puffs_usermount *,		\
	    void *, void *, const struct puffs_cn *);			\
	int fsname##_node_rename(struct puffs_usermount *,		\
	    void *, void *, const struct puffs_cn *, void *, void *,	\
	    const struct puffs_cn *);					\
	int fsname##_node_mkdir(struct puffs_usermount *,		\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *);					\
	int fsname##_node_rmdir(struct puffs_usermount *,		\
	    void *, void *, const struct puffs_cn *);			\
	int fsname##_node_symlink(struct puffs_usermount *,		\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *, const char *);			\
	int fsname##_node_readdir(struct puffs_usermount *,		\
	    void *, struct dirent *, const struct puffs_cred *,		\
	    off_t *, size_t *);						\
	int fsname##_node_readlink(struct puffs_usermount *,		\
	    void *, const struct puffs_cred *, char *, size_t *);	\
	int fsname##_node_reclaim(struct puffs_usermount *,		\
	    void *, pid_t);						\
	int fsname##_node_inactive(struct puffs_usermount *,		\
	    void *, pid_t, int *);					\
	int fsname##_node_print(struct puffs_usermount *,		\
	    void *);							\
	int fsname##_node_pathconf(struct puffs_usermount *,		\
	    void *, int, int *);					\
	int fsname##_node_advlock(struct puffs_usermount *,		\
	    void *, void *, int, struct flock *, int);			\
	int fsname##_node_getextattr(struct puffs_usermount *,		\
	    void *, struct puffs_vnreq_getextattr *);			\
	int fsname##_node_setextattr(struct puffs_usermount *,		\
	    void *, struct puffs_vnreq_setextattr *);			\
	int fsname##_node_listextattr(struct puffs_usermount *,		\
	    void *, struct puffs_vnreq_listextattr *);			\
	int fsname##_node_read(struct puffs_usermount *, void *,	\
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);\
	int fsname##_node_write(struct puffs_usermount *, void *,	\
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);

#define PUFFSOP_INIT(ops)						\
    memset(ops, 0, sizeof(struct puffs_ops))
#define PUFFSOP_SET(ops, fsname, fsornode, opname)			\
    (ops)->puffs_##fsornode##_##opname = fsname##_##fsornode##_##opname
#define PUFFSOP_SETFSNOP(ops, opname)					\
    (ops)->puffs_fs_##opname = puffs_fsnop_##opname

#endif /* _PUFFS_H_ */
