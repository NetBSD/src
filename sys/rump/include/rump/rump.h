/*	$NetBSD: rump.h,v 1.1 2008/11/17 08:53:12 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _RUMP_RUMP_H_
#define _RUMP_RUMP_H_

/*
 * NOTE: do not #include anything from <sys> here.  Otherwise this
 * has no chance of working on non-NetBSD platforms.
 */

struct mount;
struct vnode;
struct vattr;
struct componentname;
struct vfsops;
struct fid;
struct statvfs;

#if !defined(_RUMPKERNEL) && !defined(__NetBSD__)
struct kauth_cred;
typedef struct kauth_cred *kauth_cred_t;
#endif

struct lwp;
struct modinfo;

#include <rump/rumpvnode_if.h>
#include <rump/rumpdefs.h>

#ifndef curlwp
#define curlwp rump_get_curlwp()
#endif

/*
 * Something like rump capabilities would be nicer, but let's
 * do this for a start.
 */
#define RUMP_VERSION	01
#define rump_init()	_rump_init(RUMP_VERSION)

int		_rump_init(int);
struct mount	*rump_mnt_init(struct vfsops *, int);
int		rump_mnt_mount(struct mount *, const char *, void *, size_t *);
void		rump_mnt_destroy(struct mount *);

struct componentname	*rump_makecn(u_long, u_long, const char *, size_t,
				     kauth_cred_t, struct lwp *);
void			rump_freecn(struct componentname *, int);
#define RUMPCN_ISLOOKUP 0x01
#define RUMPCN_FREECRED 0x02
#define RUMPCN_HASNTBUF 0x04
int			rump_namei(uint32_t, uint32_t, const char *,
				   struct vnode **, struct vnode **,
				   struct componentname **);

void 	rump_getvninfo(struct vnode *, enum vtype *, off_t * /*XXX*/, dev_t *);

int	rump_fakeblk_register(const char *);
int	rump_fakeblk_find(const char *);
void	rump_fakeblk_deregister(const char *);

struct vfsops	*rump_vfslist_iterate(struct vfsops *);
struct vfsops	*rump_vfs_getopsbyname(const char *);

struct vattr	*rump_vattr_init(void);
void		rump_vattr_settype(struct vattr *, enum vtype);
void		rump_vattr_setmode(struct vattr *, mode_t);
void		rump_vattr_setrdev(struct vattr *, dev_t);
void		rump_vattr_free(struct vattr *);

void		rump_vp_incref(struct vnode *);
int		rump_vp_getref(struct vnode *);
void		rump_vp_decref(struct vnode *);
void		rump_vp_recycle_nokidding(struct vnode *);
void		rump_vp_rele(struct vnode *);

enum rump_uiorw { RUMPUIO_READ, RUMPUIO_WRITE };
struct uio	*rump_uio_setup(void *, size_t, off_t, enum rump_uiorw);
size_t		rump_uio_getresid(struct uio *);
off_t		rump_uio_getoff(struct uio *);
size_t		rump_uio_free(struct uio *);

void	rump_vp_lock_exclusive(struct vnode *);
void	rump_vp_lock_shared(struct vnode *);
void	rump_vp_unlock(struct vnode *);
int	rump_vp_islocked(struct vnode *);
void	rump_vp_interlock(struct vnode *);

kauth_cred_t	rump_cred_create(uid_t, gid_t, size_t, gid_t *);
kauth_cred_t	rump_cred_suserget(void);
void		rump_cred_destroy(kauth_cred_t);

#define rump_cred_suserput(c)	rump_cred_destroy(c)
/* COMPAT_NETHACK */
#define WizardMode()		rump_cred_suserget()
#define YASD(cred)		rump_cred_suserput(cred)

int	rump_vfs_unmount(struct mount *, int);
int	rump_vfs_root(struct mount *, struct vnode **, int);
int	rump_vfs_statvfs(struct mount *, struct statvfs *);
int	rump_vfs_sync(struct mount *, int, kauth_cred_t);
int	rump_vfs_fhtovp(struct mount *, struct fid *, struct vnode **);
int	rump_vfs_vptofh(struct vnode *, struct fid *, size_t *);
void	rump_vfs_syncwait(struct mount *);
int	rump_vfs_load(struct modinfo **);

void	rump_bioops_sync(void);

struct lwp	*rump_setup_curlwp(pid_t, lwpid_t, int);
struct lwp	*rump_get_curlwp(void);
void		rump_clear_curlwp(void);

void		rump_rcvp_set(struct vnode *, struct vnode *);
struct vnode 	*rump_cdir_get(void);

int	rump_splfoo(void);
void	rump_splx(int);

/* I picked the wrong header to stop sniffin' glue */
int syspuffs_glueinit(int, int *);

#endif /* _RUMP_RUMP_H_ */
