/*	$NetBSD: genfs.h,v 1.6 1998/06/25 22:15:30 thorpej Exp $	*/

int	genfs_badop	__P((void *));
int	genfs_nullop	__P((void *));
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
