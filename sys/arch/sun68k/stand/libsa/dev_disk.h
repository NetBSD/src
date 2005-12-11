/*	$NetBSD: dev_disk.h,v 1.3 2005/12/11 12:19:29 christos Exp $	*/


int	disk_open(struct open_file *, ...);
int	disk_close(struct open_file *);
int	disk_strategy(void *, int, daddr_t, size_t, void *, size_t *);
int	disk_ioctl(struct open_file *, u_long, void *);

