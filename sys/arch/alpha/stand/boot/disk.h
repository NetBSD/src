/* $NetBSD: disk.h,v 1.2.6.2 1997/04/06 08:40:35 cgd Exp $ */

int	diskstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));
/* int     diskopen __P((struct open_file *, int, int, int)); */
int     diskclose __P((struct open_file *));

#define	diskioctl	noioctl
