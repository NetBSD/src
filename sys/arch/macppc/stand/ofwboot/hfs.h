/*	$NetBSD: hfs.h,v 1.1.6.2 2001/04/24 22:56:37 he Exp $	*/

int hfs_open(char *, struct open_file *);
int hfs_close(struct open_file *);
int hfs_read(struct open_file *, void *, size_t, size_t *);
int hfs_write(struct open_file *, void *, size_t, size_t *);
off_t hfs_seek(struct open_file *, off_t, int);
int hfs_stat(struct open_file *, struct stat *);
