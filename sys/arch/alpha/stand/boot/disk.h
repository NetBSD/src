/* $NetBSD: disk.h,v 1.3 1998/10/15 00:49:34 ross Exp $ */

int	diskstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));
/* int     diskopen __P((struct open_file *, int, int, int)); */
int     diskclose __P((struct open_file *));

#define	diskioctl	noioctl

int	booted_dev_fd;
