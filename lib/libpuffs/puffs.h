/*	$NetBSD: puffs.h,v 1.1 2006/10/22 22:52:21 pooka Exp $	*/

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

#ifndef _PUFFS_USER_H_
#define _PUFFS_USER_H_

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
	struct vattr	pn_va;

	struct puffs_usermount *pn_mnt;

	LIST_ENTRY(puffs_node) pn_entries;
};

/* callbacks for operations */
struct puffs_vfsops {
	int (*puffs_start)(struct puffs_usermount *,
	    struct puffs_vfsreq_start *);
	int (*puffs_unmount)(struct puffs_usermount *, int, pid_t);
	int (*puffs_statvfs)(struct puffs_usermount *,
	    struct statvfs *, pid_t);
	int (*puffs_sync)(struct puffs_usermount *, int,
	    const struct puffs_cred *, pid_t);
};

struct puffs_vnops {
	int (*puffs_lookup)(struct puffs_usermount *,
	    void *, void **, enum vtype *, const struct puffs_cn *);
	int (*puffs_create)(struct puffs_usermount *,
	    void *, void **, const struct puffs_cn *, const struct vattr *);
	int (*puffs_mknod)(struct puffs_usermount *,
	    void *, void **, const struct puffs_cn *, const struct vattr *);
	int (*puffs_open)(struct puffs_usermount *,
	    void *, int, const struct puffs_cred *, pid_t);
	int (*puffs_close)(struct puffs_usermount *,
	    void *, int, const struct puffs_cred *, pid_t);
	int (*puffs_access)(struct puffs_usermount *,
	    void *, int, const struct puffs_cred *, pid_t);
	int (*puffs_getattr)(struct puffs_usermount *,
	    void *, struct vattr *, const struct puffs_cred *, pid_t);
	int (*puffs_setattr)(struct puffs_usermount *,
	    void *, const struct vattr *, const struct puffs_cred *, pid_t);
	int (*puffs_poll)(struct puffs_usermount *,
	    void *, struct puffs_vnreq_poll *);
	int (*puffs_revoke)(struct puffs_usermount *, void *, int);
	int (*puffs_mmap)(struct puffs_usermount *,
	    void *, struct puffs_vnreq_mmap *);
	int (*puffs_fsync)(struct puffs_usermount *,
	    void *, const struct puffs_cred *, int, off_t, off_t, pid_t);
	int (*puffs_seek)(struct puffs_usermount *,
	    void *, off_t, off_t, const struct puffs_cred *);
	int (*puffs_remove)(struct puffs_usermount *,
	    void *, void *, const struct puffs_cn *);
	int (*puffs_link)(struct puffs_usermount *,
	    void *, void *, const struct puffs_cn *);
	int (*puffs_rename)(struct puffs_usermount *,
	    void *, void *, const struct puffs_cn *, void *, void *,
	    const struct puffs_cn *);
	int (*puffs_mkdir)(struct puffs_usermount *,
	    void *, void **, const struct puffs_cn *, const struct vattr *);
	int (*puffs_rmdir)(struct puffs_usermount *,
	    void *, void *, const struct puffs_cn *);
	int (*puffs_symlink)(struct puffs_usermount *,
	    void *, void **, const struct puffs_cn *, const struct vattr *,
	    const char *);
	int (*puffs_readdir)(struct puffs_usermount *,
	    void *, struct dirent *, const struct puffs_cred *,
	    off_t *, size_t *);
	int (*puffs_readlink)(struct puffs_usermount *,
	    void *, const struct puffs_cred *, char *, size_t *);
	int (*puffs_reclaim)(struct puffs_usermount *,
	    void *, pid_t);
	int (*puffs_print)(struct puffs_usermount *,
	    void *);
	int (*puffs_pathconf)(struct puffs_usermount *,
	    void *, int, int *);
	int (*puffs_advlock)(struct puffs_usermount *,
	    void *, void *, int, struct flock *, int);
	int (*puffs_getpages)(struct puffs_usermount *,
	    void *, struct puffs_vnreq_getpages *);
	int (*puffs_putpages)(struct puffs_usermount *,
	    void *, struct puffs_vnreq_putpages *);
	int (*puffs_getextattr)(struct puffs_usermount *,
	    void *, struct puffs_vnreq_getextattr *);
	int (*puffs_setextattr)(struct puffs_usermount *,
	    void *, struct puffs_vnreq_setextattr *);
	int (*puffs_listextattr)(struct puffs_usermount *,
	    void *, struct puffs_vnreq_listextattr *);
	int (*puffs_read)(struct puffs_usermount *, void *,
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);
	int (*puffs_write)(struct puffs_usermount *, void *,
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);

	int (*puffs_ioctl1)(struct puffs_usermount *, void *,
	    struct puffs_vnreq_fcnioctl *, struct puffs_sizeop *);
	int (*puffs_ioctl2)(struct puffs_usermount *, void *,
	    struct puffs_vnreq_fcnioctl *, struct puffs_sizeop *);
	int (*puffs_fcntl1)(struct puffs_usermount *, void *,
	    struct puffs_vnreq_fcnioctl *, struct puffs_sizeop *);
	int (*puffs_fcntl2)(struct puffs_usermount *, void *,
	    struct puffs_vnreq_fcnioctl *, struct puffs_sizeop *);
};

struct puffs_usermount {
	struct puffs_vfsops	pu_pvfs;
	struct puffs_vnops	pu_pvn;

	int			pu_fd;
	uint32_t		pu_flags;
	size_t			pu_maxreqlen;

	struct puffs_node	*pu_rootnode;
	const char *		pu_rootpath;
	LIST_HEAD(, puffs_node)	pu_pnodelst;

	int	pu_wcnt;
	void	*pu_privdata;
};

#define PUFFSFLAG_OPDUMP	0x01		/* dump all operations */

struct puffs_usermount *puffs_mount(struct puffs_vfsops *, struct puffs_vnops *,
		    		    const char *, int, const char *,
				    uint32_t, size_t);
int		puffs_mainloop(struct puffs_usermount *);
int		puffs_oneop(struct puffs_usermount *, uint8_t *, size_t);
int		puffs_getselectable(struct puffs_usermount *);
int		puffs_setblockingmode(struct puffs_usermount *, int);
void		puffs_dummyops(struct puffs_vnops *);

struct puffs_node *	puffs_newpnode(struct puffs_usermount *, void *,
				       enum vtype);
void		puffs_putpnode(struct puffs_node *);
void		puffs_setvattr(struct vattr *, const struct vattr *);

#define		DENT_DOT	0
#define		DENT_DOTDOT	1
#define		DENT_ADJ(a)	((a)-2)	/* nth request means dir's n-2th */
int		puffs_gendotdent(struct dirent **, ino_t, int, size_t *);
int		puffs_nextdent(struct dirent **, const char *, ino_t,
			       uint8_t, size_t *);
int		puffs_vtype2dt(enum vtype);


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


#define PUFFSVFS_PROTOS(fsname)						\
	int fsname##_start(struct puffs_usermount *,			\
	    struct puffs_vfsreq_start *);				\
	int fsname##_unmount(struct puffs_usermount *, int, pid_t);	\
	int fsname##_statvfs(struct puffs_usermount *,			\
	    struct statvfs *, pid_t);					\
	int fsname##_sync(struct puffs_usermount *, int,		\
	    const struct puffs_cred *cred, pid_t);

#define PUFFSVN_PROTOS(fsname)						\
	int fsname##_lookup(struct puffs_usermount *,			\
	    void *, void **, enum vtype *, const struct puffs_cn *);	\
	int fsname##_create(struct puffs_usermount *,			\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *);					\
	int fsname##_mknod(struct puffs_usermount *,			\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *);					\
	int fsname##_open(struct puffs_usermount *,			\
	    void *, int, const struct puffs_cred *, pid_t);		\
	int fsname##_close(struct puffs_usermount *,			\
	    void *, int, struct puffs_cred *, pid_t);			\
	int fsname##_access(struct puffs_usermount *,			\
	    void *, int, struct puffs_cred *, pid_t);			\
	int fsname##_getattr(struct puffs_usermount *,			\
	    void *, struct vattr *, const struct puffs_cred *, pid_t);	\
	int fsname##_setattr(struct puffs_usermount *,			\
	    void *, const struct vattr *, const struct puffs_cred *,	\
	    pid_t);							\
	int fsname##_poll(struct puffs_usermount *,			\
	    void *, struct puffs_vnreq_poll *);				\
	int fsname##_revoke(struct puffs_usermount *, void *, int);	\
	int fsname##_mmap(struct puffs_usermount *,			\
	    void *, struct puffs_vnreq_mmap *);				\
	int fsname##_fsync(struct puffs_usermount *,			\
	    void *, const struct puffs_cred *, int, off_t, off_t,	\
	    pid_t);							\
	int fsname##_seek(struct puffs_usermount *,			\
	    void *, off_t, off_t, const struct puffs_cred *);		\
	int fsname##_remove(struct puffs_usermount *,			\
	    void *, void *, const struct puffs_cn *);			\
	int fsname##_link(struct puffs_usermount *,			\
	    void *, void *, const struct puffs_cn *);			\
	int fsname##_rename(struct puffs_usermount *,			\
	    void *, void *, const struct puffs_cn *, void *, void *,	\
	    const struct puffs_cn *);					\
	int fsname##_mkdir(struct puffs_usermount *,			\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *);					\
	int fsname##_rmdir(struct puffs_usermount *,			\
	    void *, void *, const struct puffs_cn *);			\
	int fsname##_symlink(struct puffs_usermount *,			\
	    void *, void **, const struct puffs_cn *,			\
	    const struct vattr *, const char *);			\
	int fsname##_readdir(struct puffs_usermount *,			\
	    void *, struct dirent *, const struct puffs_cred *,		\
	    off_t *, size_t *);						\
	int fsname##_readlink(struct puffs_usermount *,			\
	    void *, const struct puffs_cred *, char *, size_t *);	\
	int fsname##_reclaim(struct puffs_usermount *,			\
	    void *, pid_t);						\
	int fsname##_print(struct puffs_usermount *,			\
	    void *);							\
	int fsname##_pathconf(struct puffs_usermount *,			\
	    void *, int, int *);					\
	int fsname##_advlock(struct puffs_usermount *,			\
	    void *, void *, int, struct flock *, int);			\
	int fsname##_getpages(struct puffs_usermount *,			\
	    void *, struct puffs_vnreq_getpages *);			\
	int fsname##_putpages(struct puffs_usermount *,			\
	    void *, struct puffs_vnreq_putpages *);			\
	int fsname##_getextattr(struct puffs_usermount *,		\
	    void *, struct puffs_vnreq_getextattr *);			\
	int fsname##_setextattr(struct puffs_usermount *,		\
	    void *, struct puffs_vnreq_setextattr *);			\
	int fsname##_listextattr(struct puffs_usermount *,		\
	    void *, struct puffs_vnreq_listextattr *);			\
	int fsname##_read(struct puffs_usermount *, void *,		\
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);\
	int fsname##_write(struct puffs_usermount *, void *,		\
	    uint8_t *, off_t, size_t *, const struct puffs_cred *, int);

#endif /* _PUFFS_USER_H_ */
