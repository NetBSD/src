/*	$NetBSD: dev_net.h,v 1.2.26.1 2002/11/11 21:56:03 nathanw Exp $	*/


int	net_open __P((struct open_file *, ...));
int	net_close __P((struct open_file *));
int	net_ioctl __P((struct open_file *, u_long, void *));
int	net_strategy __P((void *, int , daddr_t , size_t, void *, size_t *));

