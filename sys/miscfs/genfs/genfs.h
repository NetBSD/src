/*	$NetBSD: genfs.h,v 1.8.8.1 1999/07/04 01:44:43 chs Exp $	*/

int	genfs_badop	__P((void *));
int	genfs_nullop	__P((void *));
int	genfs_enoioctl	__P((void *));
int	genfs_einval	__P((void *));
int	genfs_eopnotsupp __P((void *));
int	genfs_ebadf	__P((void *));
int	genfs_nolock	__P((void *));
int	genfs_noislocked __P((void *));
int	genfs_nounlock	__P((void *));

int	genfs_poll	__P((void *));
int	genfs_fsync	__P((void *));
int	genfs_seek	__P((void *));
int	genfs_abortop	__P((void *));
int	genfs_revoke	__P((void *));
int	genfs_lease_check __P((void *));

int	genfs_getpages __P((void *));
int	genfs_putpages __P((void *));
