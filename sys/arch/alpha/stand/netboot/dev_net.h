/*	$NetBSD: dev_net.h,v 1.3.120.1 2009/04/28 07:33:37 skrll Exp $	*/


int	net_open(struct open_file *, ...);
int	net_close(struct open_file *);
int	net_ioctl(struct open_file *, u_long, void *);
int	net_strategy(void *, int , daddr_t , size_t, void *, size_t *);

