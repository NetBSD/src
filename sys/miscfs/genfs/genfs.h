/*	$NetBSD: genfs.h,v 1.4 1998/01/05 19:20:10 perry Exp $	*/

int	genfs_badop	__P((void *));
int	genfs_nullop	__P((void *));
int	genfs_eopnotsupp __P((void *));
int	genfs_ebadf	__P((void *));

int	genfs_poll	__P((void *));
int	genfs_fsync	__P((void *));
int	genfs_seek	__P((void *));
int	genfs_abortop	__P((void *));
