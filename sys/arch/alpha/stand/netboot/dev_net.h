/*	$NetBSD: dev_net.h,v 1.3.126.1 2009/05/13 17:16:07 jym Exp $	*/


int	net_open(struct open_file *, ...);
int	net_close(struct open_file *);
int	net_ioctl(struct open_file *, u_long, void *);
int	net_strategy(void *, int , daddr_t , size_t, void *, size_t *);

