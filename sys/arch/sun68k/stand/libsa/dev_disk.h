/*	$NetBSD: dev_disk.h,v 1.1.24.1 2005/01/24 08:35:03 skrll Exp $	*/


int	disk_open(struct open_file *, ...);
int	disk_close(struct open_file *);
int	disk_strategy(void *, int, daddr_t, size_t, void *, size_t *);
int	disk_ioctl(struct open_file *, u_long, void *);

