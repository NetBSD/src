/*	$NetBSD: dev_net.h,v 1.2 1997/03/11 18:23:56 gwr Exp $	*/

int	net_open __P((struct open_file *, ...));
int	net_close __P((struct open_file *));
int	net_ioctl();
int	net_strategy();

