/*	$NetBSD: puffs_msgif.h,v 1.1 2006/10/22 22:43:23 pooka Exp $	*/

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

#ifndef _PUFFS_MSGIF_H_
#define _PUFFS_MSGIF_H_

#include <sys/param.h>
#include <sys/time.h>
#include <sys/ioccom.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/ucred.h>
#include <sys/statvfs.h>
#include <sys/dirent.h>
#include <sys/fcntl.h>

#define PUFFSOP_VFS	1
#define PUFFSOP_VN	2

enum {
	PUFFS_VFS_MOUNT,	PUFFS_VFS_START,	PUFFS_VFS_UNMOUNT,
	PUFFS_VFS_ROOT,		PUFFS_VFS_STATVFS,	PUFFS_VFS_SYNC,
	PUFFS_VFS_VGET,		PUFFS_VFS_FHTOVP,	PUFFS_VFS_VPTOFH,
	PUFFS_VFS_INIT,		PUFFS_VFS_DONE,		PUFFS_VFS_SNAPSHOT,
	PUFFS_VFS_EXTATTCTL
};
#define PUFFS_VFS_MAX PUFFS_VFS_EXTATTCTL

enum {
	PUFFS_VN_LOOKUP,	PUFFS_VN_CREATE,	PUFFS_VN_MKNOD,
	PUFFS_VN_OPEN,		PUFFS_VN_CLOSE,		PUFFS_VN_ACCESS,
	PUFFS_VN_GETATTR,	PUFFS_VN_SETATTR,	PUFFS_VN_READ,
	PUFFS_VN_WRITE,		PUFFS_VN_IOCTL,		PUFFS_VN_FCNTL,
	PUFFS_VN_POLL,		PUFFS_VN_KQFILTER,	PUFFS_VN_REVOKE,
	PUFFS_VN_MMAP,		PUFFS_VN_FSYNC,		PUFFS_VN_SEEK,
	PUFFS_VN_REMOVE,	PUFFS_VN_LINK,		PUFFS_VN_RENAME,
	PUFFS_VN_MKDIR,		PUFFS_VN_RMDIR,		PUFFS_VN_SYMLINK,
	PUFFS_VN_READDIR,	PUFFS_VN_READLINK,	PUFFS_VN_ABORTOP,
	PUFFS_VN_INACTIVE,	PUFFS_VN_RECLAIM,	PUFFS_VN_LOCK,
	PUFFS_VN_UNLOCK,	PUFFS_VN_BMAP,		PUFFS_VN_STRATEGY,
	PUFFS_VN_PRINT,		PUFFS_VN_ISLOCKED,	PUFFS_VN_PATHCONF,
	PUFFS_VN_ADVLOCK,	PUFFS_VN_LEASE,		PUFFS_VN_WHITEOUT,
	PUFFS_VN_GETPAGES,	PUFFS_VN_PUTPAGES,	PUFFS_VN_GETEXTATTR,
	PUFFS_VN_LISTEXTATTR,	PUFFS_VN_OPENEXTATTR,	PUFFS_VN_DELETEEXTATTR,
	PUFFS_VN_SETEXTATTR
};
#define PUFFS_VN_MAX PUFFS_VN_SETEXTATTR

#define PUFFSVERSION	0	/* meaning: *NO* versioning yet */
#define PUFFSNAMESIZE	32
struct puffs_args {
	int		pa_vers;
	int		pa_fd;
	unsigned int	pa_flags;
	size_t		pa_maxreqlen;
	char		pa_name[PUFFSNAMESIZE];	/* name for puffs type	*/
};
#define PUFFS_FLAG_ALLOWCTL	0x01	/* ioctl/fcntl commands allowed */
#define PUFFS_FLAG_MASK		0x01

/*
 * This is the device minor number for the cloning device.  Make it
 * a high number "just in case", even though we don't want to open
 * any specific devices currently.
 */
#define PUFFS_CLONER 0x7ffff

/*
 * This is the structure that travels on the user-kernel interface.
 * It is used to deliver ops to the user server and bring the results back
 */
struct puffs_req {
	uint64_t	preq_id;		/* OUT/IN	*/
	uint8_t		preq_opclass;		/* OUT   	*/
	uint8_t		preq_optype;		/* OUT		*/

	int		preq_rv;		/* IN		*/

	/*
	 * preq_cookie is the node cookie associated with the request.
	 * It always maps 1:1 to a vnode and should map to a userspace
	 * struct puffs_node.  The cookie usually describes the first
	 * vnode argument of the VOP_POP() in question.
	 */
	void	*preq_cookie;			/* OUT		*/

	/* these come in from userspace twice (getop and putop) */
	void	*preq_aux;			/* IN/IN	*/
	size_t	preq_auxlen;			/* IN/IN	*/
};

#define PUFFS_REQFLAG_ADJBUF	0x01

/*
 * Some operations have unknown size requirements.  So as the first
 * stab at handling it, do an extra bounce between the kernel and
 * userspace.
 */
struct puffs_sizeop {
	uint64_t	pso_reqid;

	uint8_t		*pso_userbuf;
	size_t		pso_bufsize;
};

/*
 * Credentials for an operation.  Can be either struct uucred for
 * ops called from a credential context or NOCRED/FSCRED for ops
 * called from within the kernel.  It is up to the implementation
 * if it makes a difference between these two and the super-user.
 */
struct puffs_cred {
	struct uucred	pcr_uuc;
	uint8_t		pcr_type;
	uint8_t		pcr_internal;
};
#define PUFFCRED_TYPE_UUC	1
#define PUFFCRED_TYPE_INTERNAL	2

#define PUFFCRED_CRED_NOCRED	1
#define PUFFCRED_CRED_FSCRED	2


#define PUFFSGETOP	_IOWR('p', 1, struct puffs_req)
#define PUFFSPUTOP	_IOWR('p', 2, struct puffs_req)
#define PUFFSSIZEOP	_IOWR('p', 3, struct puffs_sizeop)
#define PUFFSMOUNTOP	_IOWR('p', 4, struct puffs_vfsreq_start)

/*
 * 4x MAXPHYS is the max size the system will attempt to copy,
 * else treated as garbage
 */
#define PUFFS_REQ_MAXSIZE	4*MAXPHYS
#define PUFFS_REQSTRUCT_MAX	1024 /* XXX: approxkludge */

#define PUFFS_TOMOVE(a,b) (MIN((a), b->pmp_req_maxsize - PUFFS_REQSTRUCT_MAX))

/* puffs struct componentname, for userspace */
struct puffs_cn {
	/* args */
	u_long			pcn_nameiop;	/* namei operation	*/
	u_long			pcn_flags;	/* flags		*/
	pid_t			pcn_pid;	/* caller pid		*/
	struct puffs_cred	pcn_cred;	/* caller creds		*/

	/* shared */
	char pcn_name[MAXPATHLEN];		/* path to lookup	*/
	long pcn_namelen;			/* length of path	*/
};

/*
 * XXX: figure out what to do with these, copied from namei.h for now
 */
#define	PUFFSLOOKUP_LOOKUP	0	/* perform name lookup only */
#define PUFFSLOOKUP_CREATE	1	/* setup for file creation */
#define PUFFSLOOKUP_DELETE	2	/* setup for file deletion */
#define PUFFSLOOKUP_RENAME	3	/* setup for file renaming */
#define PUFFSLOOKUP_OPMASK	3	/* mask for operation */

#define PUFFSLOOKUP_FOLLOW	0x04	/* follow symlinks */
#define PUFFSLOOKUP_NOFOLLOW	0x08	/* don't follow symlinks */
#define PUFFSLOOKUP_OPTIONS	0x0c

struct puffs_vfsreq_start {
	fsid_t	psr_fsidx;		/* fsid value */
	void	*psr_cookie;		/* root node cookie */
};

/*
 * aux structures for vfs operations.
 */
struct puffs_vfsreq_unmount {
	int			pvfsr_flags;
	pid_t			pvfsr_pid;
};

struct puffs_vfsreq_statvfs {
	struct statvfs		pvfsr_sb;
	pid_t			pvfsr_pid;
};

struct puffs_vfsreq_sync {
	struct puffs_cred	pvfsr_cred;
	pid_t			pvfsr_pid;
	int			pvfsr_waitfor;
};

/*
 * aux structures for vnode operations.
 */

struct puffs_vnreq_lookup {
	struct puffs_cn		pvnr_cn;		/* OUT	*/
	void			*pvnr_newnode;		/* IN	*/
	enum vtype		pvnr_vtype;		/* IN	*/
};

struct puffs_vnreq_create {
	struct puffs_cn		pvnr_cn;		/* OUT	*/
	struct vattr		pvnr_va;		/* OUT	*/
	void			*pvnr_newnode;		/* IN	*/
};

struct puffs_vnreq_mknod {
	struct puffs_cn		pvnr_cn;		/* OUT	*/
	struct vattr		pvnr_va;		/* OUT	*/
	void			*pvnr_newnode;		/* IN	*/
};

struct puffs_vnreq_open {
	struct puffs_cred	pvnr_cred;		/* OUT	*/
	pid_t			pvnr_pid;		/* OUT	*/
	int			pvnr_mode;		/* OUT	*/
};

struct puffs_vnreq_close {
	struct puffs_cred	pvnr_cred;		/* OUT	*/
	pid_t			pvnr_pid;		/* OUT	*/
	int			pvnr_fflag;		/* OUT	*/
};

struct puffs_vnreq_access {
	struct puffs_cred	pvnr_cred;		/* OUT	*/
	pid_t			pvnr_pid;		/* OUT	*/
	int			pvnr_mode;		/* OUT	*/
};

#define puffs_vnreq_setattr puffs_vnreq_setgetattr
#define puffs_vnreq_getattr puffs_vnreq_setgetattr
struct puffs_vnreq_setgetattr {
	struct puffs_cred	pvnr_cred;		/* OUT	*/
	struct vattr		pvnr_va;		/* IN/OUT (op depend) */
	pid_t			pvnr_pid;		/* OUT	*/
};

#define puffs_vnreq_read puffs_vnreq_readwrite
#define puffs_vnreq_write puffs_vnreq_readwrite
struct puffs_vnreq_readwrite {
	struct puffs_cred	pvnr_cred;		/* OUT	  */
	off_t			pvnr_offset;		/* OUT	  */
	size_t			pvnr_resid;		/* IN/OUT */
	int			pvnr_ioflag;		/* OUT	  */

	uint8_t			pvnr_data[0];		/* IN/OUT (wr/rd) */
};

#define puffs_vnreq_ioctl puffs_vnreq_fcnioctl
#define puffs_vnreq_fcntl puffs_vnreq_fcnioctl
struct puffs_vnreq_fcnioctl {
	struct puffs_cred	pvnr_cred;
	u_long			pvnr_command;
	pid_t			pvnr_pid;
	int			pvnr_fflag;

	void			*pvnr_data;
	size_t			pvnr_datalen;
	int			pvnr_copyback;
};

struct puffs_vnreq_poll {
	int			pvnr_events;		/* OUT	*/
	pid_t			pvnr_pid;		/* OUT	*/
};

struct puffs_vnreq_revoke {
	int			pvnr_flags;		/* OUT	*/
};

struct puffs_vnreq_fsync {
	struct puffs_cred	pvnr_cred;		/* OUT	*/
	off_t			pvnr_offlo;		/* OUT	*/
	off_t			pvnr_offhi;		/* OUT	*/
	pid_t			pvnr_pid;		/* OUT	*/
	int			pvnr_flags;		/* OUT	*/
};

struct puffs_vnreq_seek {
	struct puffs_cred	pvnr_cred;		/* OUT	*/
	off_t			pvnr_oldoff;		/* OUT	*/
	off_t			pvnr_newoff;		/* OUT	*/
};

struct puffs_vnreq_remove {
	struct puffs_cn		pvnr_cn;		/* OUT	*/
	void			*pvnr_cookie_targ;	/* OUT	*/
};

struct puffs_vnreq_mkdir {
	struct puffs_cn		pvnr_cn;		/* OUT	*/
	struct vattr		pvnr_va;		/* OUT	*/
	void			*pvnr_newnode;		/* IN	*/
};

struct puffs_vnreq_rmdir {
	struct puffs_cn		pvnr_cn;		/* OUT	*/
	void			*pvnr_cookie_targ;	/* OUT	*/
};

struct puffs_vnreq_link {
	struct puffs_cn		pvnr_cn;		/* OUT */
	void			*pvnr_cookie_targ;	/* OUT */
};

struct puffs_vnreq_rename {
	struct puffs_cn		pvnr_cn_src;		/* OUT	*/
	struct puffs_cn		pvnr_cn_targ;		/* OUT	*/
	void			*pvnr_cookie_src;	/* OUT	*/
	void			*pvnr_cookie_targ;	/* OUT	*/
	void			*pvnr_cookie_targdir;	/* OUT	*/
};

struct puffs_vnreq_symlink {
	struct puffs_cn		pvnr_cn;		/* OUT	*/
	struct vattr		pvnr_va;		/* OUT	*/
	void			*pvnr_newnode;		/* IN	*/
	char			pvnr_link[MAXPATHLEN];	/* OUT	*/
};

struct puffs_vnreq_readdir {
	struct puffs_cred	pvnr_cred;		/* OUT	  */
	off_t			pvnr_offset;		/* IN/OUT */
	size_t			pvnr_resid;		/* IN/OUT */

	struct dirent		pvnr_dent[0];		/* IN  	   */
};

struct puffs_vnreq_readlink {
	struct puffs_cred	pvnr_cred;		/* OUT */
	size_t			pvnr_linklen;		/* IN  */
	char			pvnr_link[MAXPATHLEN];	/* IN, XXX  */
};

struct puffs_vnreq_reclaim {
	pid_t			pvnr_pid;		/* OUT	*/
};

/* XXX: get rid of alltogether */
struct puffs_vnreq_print {
	/* empty */
};

struct puffs_vnreq_pathconf {
	int			pvnr_name;		/* OUT	*/
	int			pvnr_retval;		/* IN	*/
};

struct puffs_vnreq_advlock {
	struct flock		pvnr_fl;		/* OUT	*/
	void			*pvnr_id;		/* OUT	*/
	int			pvnr_op;		/* OUT	*/
	int			pvnr_flags;		/* OUT	*/
};

/* notyet */
#if 0
struct puffs_vnreq_kqfilter { };
struct puffs_vnreq_islocked { };
struct puffs_vnreq_lease { };
#endif
struct puffs_vnreq_getpages { };
struct puffs_vnreq_putpages { };
struct puffs_vnreq_mmap { };
struct puffs_vnreq_getextattr { };
struct puffs_vnreq_setextattr { };
struct puffs_vnreq_listextattr { };

#ifdef _KERNEL
#define PUFFS_VFSREQ(a)							\
	struct puffs_vfsreq_##a a##_arg;				\
	memset(&a##_arg, 0, sizeof(struct puffs_vfsreq_##a))

#define PUFFS_VNREQ(a)							\
	struct puffs_vnreq_##a a##_arg;					\
	memset(&a##_arg, 0, sizeof(struct puffs_vnreq_##a))
#endif

#endif /* _PUFFS_MSGIF_H_ */
