/*	$NetBSD: genfs.h,v 1.23.24.1 2009/05/13 17:22:16 jym Exp $	*/

#ifndef	_MISCFS_GENFS_GENFS_H_
#define	_MISCFS_GENFS_GENFS_H_

#include <sys/vnode.h>

int	genfs_badop(void *);
int	genfs_nullop(void *);
int	genfs_enoioctl(void *);
int	genfs_enoextops(void *);
int	genfs_einval(void *);
int	genfs_eopnotsupp(void *);
int	genfs_ebadf(void *);
int	genfs_nolock(void *);
int	genfs_noislocked(void *);
int	genfs_nounlock(void *);

int	genfs_poll(void *);
int	genfs_kqfilter(void *);
int	genfs_fcntl(void *);
int	genfs_seek(void *);
int	genfs_abortop(void *);
int	genfs_revoke(void *);
int	genfs_lock(void *);
int	genfs_islocked(void *);
int	genfs_unlock(void *);
int	genfs_mmap(void *);
int	genfs_getpages(void *);
int	genfs_putpages(void *);
int	genfs_null_putpages(void *);
int	genfs_compat_getpages(void *);

int	genfs_do_putpages(struct vnode *, off_t, off_t, int, struct vm_page **);

int	genfs_renamelock_enter(struct mount *);
void	genfs_renamelock_exit(struct mount *);

int	genfs_can_chmod(vnode_t *, kauth_cred_t, uid_t, gid_t, mode_t);
int	genfs_can_chown(vnode_t *, kauth_cred_t, uid_t, gid_t, uid_t, gid_t);
int	genfs_can_mount(vnode_t *, mode_t, kauth_cred_t);
int	genfs_can_chtimes(vnode_t *, u_int, uid_t, kauth_cred_t);

#endif /* !_MISCFS_GENFS_GENFS_H_ */
