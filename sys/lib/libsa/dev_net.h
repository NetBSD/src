/*	$NetBSD: dev_net.h,v 1.3 1997/03/15 18:12:14 is Exp $	*/

int	net_open __P((struct open_file *, ...));
int	net_close __P((struct open_file *));
int	net_ioctl();
int	net_strategy();

