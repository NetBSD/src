/* $NetBSD: exfatfs_vnops.h,v 1.1.2.1 2024/06/29 19:43:26 perseant Exp $ */
#ifndef EXFATFS_VNOPS_H_
#define EXFATFS_VNOPS_H_

int exfatfs_access(void *v);
int exfatfs_activate(struct xfinode *xip, bool rekey);
int exfatfs_advlock(void *v);
int exfatfs_bmap(void *v);
int exfatfs_close(void *v);
int exfatfs_create(void *v);
int exfatfs_deactivate(struct xfinode *xip, bool rekey);
int exfatfs_findempty(struct vnode *, struct xfinode *);
int exfatfs_fsync(void *v);
int exfatfs_getattr(void *v);
int exfatfs_inactive(void *v);
void exfatfs_itimes(struct xfinode *dep, const struct timespec *acc,
		    const struct timespec *mod, const struct timespec *cre);
int exfatfs_lookup(void *);
int exfatfs_mkdir(void *v);
int exfatfs_pathconf(void *v);
int exfatfs_print(void *v);
int exfatfs_read(void *v);
int exfatfs_readdir(void *v);
int exfatfs_readlink(void *v);
int exfatfs_reclaim(void *v);
int exfatfs_rekey(struct vnode *vp, struct exfatfs_dirent_key *nkeyp);
int exfatfs_remove(void *v);
int exfatfs_rename(void *v);
int exfatfs_rmdir(void *v);
int exfatfs_setattr(void *v);
int exfatfs_strategy(void *v);
int exfatfs_symlink(void *v);
int exfatfs_update(struct vnode *vp, const struct timespec *acc,
		   const struct timespec *mod, int flags);
int exfatfs_write(void *v);
int exfatfs_writeback(struct xfinode *xip);
int exfatfs_writeback2(struct xfinode *xip, struct xfinode *rxip);

#endif /* EXFATFS_VNOPS_H_ */
