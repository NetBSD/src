/*	$NetBSD: dev_net.h,v 1.1.16.2 2002/06/23 17:37:35 jdolecek Exp $	*/

int	net_open __P((struct open_file *, ...));
int	net_close __P((struct open_file *));
int	net_ioctl __P((struct open_file *, u_long, void *));
int	net_strategy __P((void *, int , daddr_t , size_t, void *, size_t *));
