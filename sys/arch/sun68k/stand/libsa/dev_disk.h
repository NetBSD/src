/*	$NetBSD: dev_disk.h,v 1.1.32.1 2005/04/29 11:28:27 kent Exp $	*/


int	disk_open(struct open_file *, ...);
int	disk_close(struct open_file *);
int	disk_strategy(void *, int, daddr_t, size_t, void *, size_t *);
int	disk_ioctl(struct open_file *, u_long, void *);

