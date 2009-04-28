/*	$NetBSD: dev_net.h,v 1.1.136.1 2009/04/28 07:34:15 skrll Exp $	*/

int	net_open(struct open_file *, ...);
int	net_close(struct open_file *);
int	net_ioctl(struct open_file *, u_long, void *);
int	net_strategy(void *, int , daddr_t , size_t, void *, size_t *);
