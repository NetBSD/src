/*	$NetBSD: dev_disk.h,v 1.4 2001/02/22 07:11:10 chs Exp $	*/


int	disk_open __P((struct open_file *, ...));
int	disk_close __P((struct open_file *));
int	disk_strategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	disk_ioctl __P((struct open_file *, u_long, void *));

