/*	$NetBSD: rump.h,v 1.7.2.2 2009/07/23 23:32:53 jym Exp $	*/

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
struct stat;

/* yetch */
#if !defined(_RUMPKERNEL) && !defined(__NetBSD__)
struct kauth_cred;
typedef struct kauth_cred *kauth_cred_t;
#endif
#if defined(__NetBSD__)
#include <prop/proplib.h>
#else
struct prop_dictionary;
typedef struct prop_dictionary *prop_dictionary_t;
#endif /* __NetBSD__ */

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
#define rump_init()	rump__init(RUMP_VERSION)
int	rump_module_init(struct modinfo *, prop_dictionary_t props);
int	rump_module_fini(struct modinfo *);

int		rump__init(int);
int		rump_getversion(void);

struct mount	*rump_mnt_init(struct vfsops *, int);
int		rump_mnt_mount(struct mount *, const char *, void *, size_t *);
void		rump_mnt_destroy(struct mount *);

struct componentname	*rump_makecn(u_long, u_long, const char *, size_t,
				     kauth_cred_t, struct lwp *);
void			rump_freecn(struct componentname *, int);
#define RUMPCN_ISLOOKUP 0x01
#define RUMPCN_FREECRED 0x02
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

void	rump_vp_interlock(struct vnode *);

kauth_cred_t	rump_cred_create(uid_t, gid_t, size_t, gid_t *);
kauth_cred_t	rump_cred_suserget(void);
void		rump_cred_put(kauth_cred_t);

#define rump_cred_suserput(c)	rump_cred_put(c)
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
int	rump_vfs_getmp(const char *, struct mount **);

struct lwp	*rump_newproc_switch(void);
struct lwp	*rump_setup_curlwp(pid_t, lwpid_t, int);
struct lwp	*rump_get_curlwp(void);
void		rump_set_curlwp(struct lwp *);
void		rump_clear_curlwp(void);

void		rump_rcvp_set(struct vnode *, struct vnode *);
struct vnode 	*rump_cdir_get(void);

/* I picked the wrong header to stop sniffin' glue */
int rump_syspuffs_glueinit(int, int *);

int rump_virtif_create(int);

typedef int (*rump_sysproxy_t)(int, void *, uint8_t *, size_t, register_t *);
int		rump_sysproxy_set(rump_sysproxy_t, void *);
int		rump_sysproxy_socket_setup_client(int);
int		rump_sysproxy_socket_setup_server(int);

/*
 * compat syscalls.  these are currently hand-"generated"
 */
int		rump_sys___stat30(const char *, struct stat *);
int		rump_sys___lstat30(const char *, struct stat *);

/*
 * Other compat glue (for sniffing purposes)
 * XXX: (lack of) types
 */
void		rump_vattr50_to_vattr(const struct vattr *, struct vattr *);
void		rump_vattr_to_vattr50(const struct vattr *, struct vattr *);

/*
 * Begin rump syscall conditionals.  Yes, something a little better
 * is required here.
 */
#ifdef RUMP_SYS_NETWORKING
#define socket(a,b,c) rump_sys_socket(a,b,c)
#define accept(a,b,c) rump_sys_accept(a,b,c)
#define bind(a,b,c) rump_sys_bind(a,b,c)
#define connect(a,b,c) rump_sys_connect(a,b,c)
#define getpeername(a,b,c) rump_sys_getpeername(a,b,c)
#define getsockname(a,b,c) rump_sys_getsockname(a,b,c)
#define listen(a,b) rump_sys_listen(a,b)
#define recvfrom(a,b,c,d,e,f) rump_sys_recvfrom(a,b,c,d,e,f)
#define sendto(a,b,c,d,e,f) rump_sys_sendto(a,b,c,d,e,f)
#define getsockopt(a,b,c,d,e) rump_sys_getsockopt(a,b,c,d,e)
#define setsockopt(a,b,c,d,e) rump_sys_setsockopt(a,b,c,d,e)
#define shutdown(a,b) rump_sys_shutdown(a,b)
#endif /* RUMP_SYS_NETWORKING */

#ifdef RUMP_SYS_IOCTL
#define ioctl(...) rump_sys_ioctl(__VA_ARGS__)
#endif /* RUMP_SYS_IOCTL */

#ifdef RUMP_SYS_CLOSE
#define close(a) rump_sys_close(a)
#endif /* RUMP_SYS_CLOSE */

#ifdef RUMP_SYS_READWRITE
#define read(a,b,c) rump_sys_read(a,b,c)
#define readv(a,b,c) rump_sys_readv(a,b,c)
#define pread(a,b,c,d) rump_sys_pread(a,b,c,d)
#define preadv(a,b,c,d) rump_sys_preadv(a,b,c,d)
#define write(a,b,c) rump_sys_write(a,b,c)
#define writev(a,b,c) rump_sys_writev(a,b,c)
#define pwrite(a,b,c,d) rump_sys_pwrite(a,b,c,d)
#define pwritev(a,b,c,d) rump_sys_pwritev(a,b,c,d)
#endif /* RUMP_SYS_READWRITE */

#endif /* _RUMP_RUMP_H_ */
