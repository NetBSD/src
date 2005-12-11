/*	$NetBSD: rawfs.h,v 1.4 2005/12/11 12:17:19 christos Exp $	*/

/*
 * Raw file system - for stream devices like tapes.
 * No random access, only sequential read allowed.
 */

int rawfs_open(const char *, struct open_file *);
int rawfs_close(struct open_file *);
int rawfs_read(struct open_file *, void *, u_int, u_int *);
int rawfs_write(struct open_file *, void *, u_int, u_int *);
off_t rawfs_seek(struct open_file *, off_t, int);
int rawfs_stat(struct open_file *, struct stat *);
