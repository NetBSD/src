/*	$NetBSD: rawfs.h,v 1.2 2003/08/21 23:28:27 he Exp $	*/

/*
 * Raw file system - for stream devices like tapes.
 * No random access, only sequential read allowed.
 */

int	rawfs_open __P((const char *path, struct open_file *f));
int	rawfs_close __P((struct open_file *f));
int	rawfs_read __P((struct open_file *f, void *buf,
		u_int size, u_int *resid));
int	rawfs_write __P((struct open_file *f, void *buf,
		u_int size, u_int *resid));
off_t	rawfs_seek __P((struct open_file *f, off_t offset, int where));
int	rawfs_stat __P((struct open_file *f, struct stat *sb));
