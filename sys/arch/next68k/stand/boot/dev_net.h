/*	$NetBSD: dev_net.h,v 1.1.1.1 1998/06/09 07:53:06 dbj Exp $	*/

int	net_open __P((struct open_file *, ...));
int	net_close __P((struct open_file *));
int	net_ioctl();
int	net_strategy();

