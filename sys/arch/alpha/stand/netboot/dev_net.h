/*	$NetBSD: dev_net.h,v 1.2 1998/01/05 07:02:48 perry Exp $	*/


int	net_open __P((struct open_file *, ...));
int	net_close __P((struct open_file *));
int	net_ioctl();
int	net_strategy();

