/*	$NetBSD: dev_disk.h,v 1.1 2001/06/14 12:57:14 fredette Exp $	*/


int	disk_open __P((struct open_file *, ...));
int	disk_close __P((struct open_file *));
int	disk_strategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	disk_ioctl __P((struct open_file *, u_long, void *));

