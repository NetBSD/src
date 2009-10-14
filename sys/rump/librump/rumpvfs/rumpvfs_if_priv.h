/*	$NetBSD: rumpvfs_if_priv.h,v 1.1 2009/10/14 17:28:14 pooka Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpvfs.ifspec,v 1.1 2009/10/14 17:17:00 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.2 2009/10/14 17:26:09 pooka Exp 
 */

void rumppriv_getvninfo(struct vnode *, enum vtype *, off_t *, dev_t *);
struct vfsops * rumppriv_vfslist_iterate(struct vfsops *);
struct vfsops * rumppriv_vfs_getopsbyname(const char *);
struct vattr * rumppriv_vattr_init(void);
void rumppriv_vattr_settype(struct vattr *, enum vtype);
void rumppriv_vattr_setmode(struct vattr *, mode_t);
void rumppriv_vattr_setrdev(struct vattr *, dev_t);
void rumppriv_vattr_free(struct vattr *);
void rumppriv_vp_incref(struct vnode *);
int rumppriv_vp_getref(struct vnode *);
void rumppriv_vp_rele(struct vnode *);
void rumppriv_vp_interlock(struct vnode *);
int rumppriv_etfs_register(const char *, const char *, enum rump_etfs_type);
int rumppriv_etfs_register_withsize(const char *, const char *, enum rump_etfs_type, uint64_t, uint64_t);
int rumppriv_etfs_remove(const char *);
void rumppriv_freecn(struct componentname *, int);
int rumppriv_checksavecn(struct componentname *);
int rumppriv_namei(uint32_t, uint32_t, const char *, struct vnode **, struct vnode **, struct componentname **);
struct componentname * rumppriv_makecn(u_long, u_long, const char *, size_t, kauth_cred_t, struct lwp *);
int rumppriv_vfs_unmount(struct mount *, int);
int rumppriv_vfs_root(struct mount *, struct vnode **, int);
int rumppriv_vfs_statvfs(struct mount *, struct statvfs *);
int rumppriv_vfs_sync(struct mount *, int, kauth_cred_t);
int rumppriv_vfs_fhtovp(struct mount *, struct fid *, struct vnode **);
int rumppriv_vfs_vptofh(struct vnode *, struct fid *, size_t *);
void rumppriv_vfs_syncwait(struct mount *);
int rumppriv_vfs_getmp(const char *, struct mount **);
void rumppriv_rcvp_set(struct vnode *, struct vnode *);
struct vnode * rumppriv_cdir_get(void);
int rumppriv_syspuffs_glueinit(int, int *);
int rumppriv_sys___stat30(const char *, struct stat *);
int rumppriv_sys___lstat30(const char *, struct stat *);
void rumppriv_vattr50_to_vattr(const struct vattr *, struct vattr *);
void rumppriv_vattr_to_vattr50(const struct vattr *, struct vattr *);
