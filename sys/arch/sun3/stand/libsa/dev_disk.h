/*	$NetBSD: dev_disk.h,v 1.3 1998/01/05 07:03:24 perry Exp $	*/


int	disk_open __P((struct open_file *, ...));
int	disk_close __P((struct open_file *));
int	disk_strategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	disk_ioctl();

