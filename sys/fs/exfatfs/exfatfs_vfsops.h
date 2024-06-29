/* $NetBSD: exfatfs_vfsops.h,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $ */
#ifndef EXFATFS_VFSOPS_H_
#define EXFATFS_VFSOPS_H_

int exfatfs_getnewvnode(struct exfatfs *fs, struct vnode *dvp,
			uint32_t clust, uint32_t off, unsigned type,
			void *xip, struct vnode **vpp);
#ifdef _KERNEL
void exfatfs_init(void);
int exfatfs_loadvnode(struct mount *mp, struct vnode *vp, const void *key, size_t key_len, const void **new_key);
int exfatfs_mount(struct mount *mp, const char *path, void *data, size_t *data_len);
int exfatfs_mountfs(struct vnode *, struct mount *, struct lwp *, struct exfatfs_args *);
int exfatfs_finish_mountfs(struct exfatfs *fs);
int exfatfs_mountroot(void);
/* void exfatfs_reinit(void); */
int exfatfs_root(struct mount *mp, int unused, struct vnode **vpp);
int exfatfs_start(struct mount *mp, int flags);
int exfatfs_statvfs(struct mount *mp, struct statvfs *sbp);
int exfatfs_sync(struct mount *mp, int waitfor, kauth_cred_t cred);
int exfatfs_unmount(struct mount *mp, int mntflags);
int exfatfs_vget(struct mount *mp, ino_t ino, int unused, struct vnode **vpp);
#endif /* _KERNEL */

#endif /* EXFATFS_VFSOPS_H_ */
