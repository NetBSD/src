/*	$NetBSD: dev_disk.h,v 1.3.16.1 2001/03/12 13:29:38 bouyer Exp $	*/


int	disk_open __P((struct open_file *, ...));
int	disk_close __P((struct open_file *));
int	disk_strategy __P((void *, int, daddr_t, size_t, void *, size_t *));
int	disk_ioctl __P((struct open_file *, u_long, void *));

