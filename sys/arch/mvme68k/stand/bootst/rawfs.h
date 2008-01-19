/*	$NetBSD: rawfs.h,v 1.3.64.1 2008/01/19 12:14:32 bouyer Exp $	*/

/*
 * Raw file system - for stream devices like tapes.
 * No random access, only sequential read allowed.
 */

int	rawfs_open(const char *path, struct open_file *f);
int	rawfs_close(struct open_file *f);
int	rawfs_read(struct open_file *f, void *buf, u_int size, u_int *resid);
int	rawfs_write(struct open_file *f, void *buf, u_int size, u_int *resid);
off_t	rawfs_seek(struct open_file *f, off_t offset, int where);
int	rawfs_stat(struct open_file *f, struct stat *sb);
