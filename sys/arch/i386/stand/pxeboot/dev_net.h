/*	$NetBSD: dev_net.h,v 1.1.142.2 2009/11/01 13:58:44 jym Exp $	*/

int	net_open(struct open_file *, ...);
int	net_close(struct open_file *);
int	net_ioctl(struct open_file *, u_long, void *);
int	net_strategy(void *, int , daddr_t , size_t, void *, size_t *);
