/*	$NetBSD: genfs.h,v 1.5 1998/03/01 02:22:03 fvdl Exp $	*/

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
