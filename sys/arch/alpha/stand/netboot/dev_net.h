/*	$NetBSD: dev_net.h,v 1.3 2002/11/09 06:34:38 thorpej Exp $	*/


int	net_open __P((struct open_file *, ...));
int	net_close __P((struct open_file *));
int	net_ioctl __P((struct open_file *, u_long, void *));
int	net_strategy __P((void *, int , daddr_t , size_t, void *, size_t *));

