/*	$NetBSD: dev_net.h,v 1.5.28.1 2009/01/19 13:19:57 skrll Exp $	*/

int	net_open(struct open_file *, ...);
int	net_close(struct open_file *);
int	net_ioctl(struct open_file *, u_long, void *);
int	net_strategy(void *, int , daddr_t , size_t, void *, size_t *);

#ifdef SUPPORT_BOOTP
extern int try_bootp;
#endif
