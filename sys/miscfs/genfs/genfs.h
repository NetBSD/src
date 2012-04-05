/*	$NetBSD: genfs.h,v 1.28.16.1 2012/04/05 21:33:42 mrg Exp $	*/

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

int	genfs_statvfs(struct mount *, struct statvfs *);

int	genfs_renamelock_enter(struct mount *);
void	genfs_renamelock_exit(struct mount *);

int	genfs_can_access(enum vtype, mode_t, uid_t, gid_t, mode_t,
	    kauth_cred_t);
int	genfs_can_chmod(enum vtype, kauth_cred_t, uid_t, gid_t, mode_t);
int	genfs_can_chown(kauth_cred_t, uid_t, gid_t, uid_t, gid_t);
int	genfs_can_chtimes(vnode_t *, u_int, uid_t, kauth_cred_t);
int	genfs_can_chflags(kauth_cred_t, enum vtype, uid_t, bool);
int	genfs_can_sticky(kauth_cred_t, uid_t, uid_t);
int	genfs_can_extattr(kauth_cred_t, int, vnode_t *, const char *);

#endif /* !_MISCFS_GENFS_GENFS_H_ */
