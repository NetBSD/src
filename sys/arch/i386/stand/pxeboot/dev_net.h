/*	$NetBSD: dev_net.h,v 1.2 2009/03/14 14:46:00 dsl Exp $	*/

int	net_open(struct open_file *, ...);
int	net_close(struct open_file *);
int	net_ioctl(struct open_file *, u_long, void *);
int	net_strategy(void *, int , daddr_t , size_t, void *, size_t *);
